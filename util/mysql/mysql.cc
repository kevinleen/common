#include "mysql.h"
#include <mysql/errmsg.h>

namespace util {

Mysql::Mysql(const std::string& ip, uint32 port,
             const std::string& user, const std::string& password,
             const std::string& db, const std::string& charset)
    : _ip(ip), _port(port), _user(user), _password(password),
      _db(db), _charset(charset), _mysql(NULL) {
    this->connect();
}

Mysql::~Mysql() {
    this->disconnect();
}

bool Mysql::connect() {
    if (_mysql != NULL) return true;

    _mysql = mysql_init(NULL);


    if(!_charset.empty())  {
        auto ret = mysql_options(_mysql, MYSQL_SET_CHARSET_NAME,
                                 _charset.c_str());
        if (ret != 0) {
            TLOG("mysql") << "failed to set charset: " << _charset;
            return false;
        }
    }

    auto ret = mysql_real_connect(_mysql, _ip.c_str(), _user.c_str(),
                                  _password.c_str(), _db.c_str(), _port,
                                  NULL, 0);
    if (ret == NULL) {
        TLOG("mysql") << "failed to connect mysql " << _ip << ":" << _port
            << ", error: " << mysql_error(_mysql);
        return false;
    }

    return true;
}

void Mysql::disconnect() {
    if (_mysql != NULL) {
        mysql_close(_mysql);
        _mysql = NULL;
    }
}

bool Mysql::execute(const std::string& sql) {
    if (_mysql == NULL && !this->connect()) return false;

    int ret = mysql_real_query(_mysql, sql.c_str(), sql.size());

    if(ret != 0) {
        int err = mysql_errno(_mysql);
        if (err == CR_SERVER_LOST || err == CR_SERVER_GONE_ERROR) {
            this->disconnect();
        }

        TLOG("mysql") << "execute " << sql << ", errno: " << err;
        return false;
    }

    return true;
}

Mysql::MysqlData Mysql::query_record(const std::string& sql) {
    if (_mysql == NULL && !this->connect()) return MysqlData();

    int ret = mysql_real_query(_mysql, sql.data(), sql.size());

    if(ret != 0) {
        int err = mysql_errno(_mysql);
        if (err == CR_SERVER_LOST || err == CR_SERVER_GONE_ERROR) {
            this->disconnect();
        }

        TLOG("mysql") << "query_record: " << sql << ", errno: " << err;
        return MysqlData();
    }

    MYSQL_RES* res = mysql_store_result(_mysql);
    if(res == NULL) {
        TLOG("mysql") << "query_record: " << sql << ", store_result error: "
            << mysql_error(_mysql);
        return MysqlData();
    }

    std::vector<std::string> fields;
    MYSQL_FIELD* field;

    while((field = mysql_fetch_field(res)) != NULL) {
         fields.push_back(field->name);
    }

    MysqlData data;
    std::map<std::string, std::string> row;
    MYSQL_ROW mysql_row;

    while((mysql_row = mysql_fetch_row(res)) != (MYSQL_ROW)NULL) {
        row.clear();
        unsigned long * lengths = mysql_fetch_lengths(res);

        for(size_t i = 0; i < fields.size(); ++i) {
            if(mysql_row[i] != NULL) {
                row[fields[i]] = std::string(mysql_row[i], lengths[i]);

            } else {
                row[fields[i]] = std::string();
            }
        }

        data.push_back(row);
    }

    mysql_free_result(res);
    return data;
}

bool Mysql::insert_record(const std::string& table, const std::string& sql) {
    std::string cmd = "insert into " + table + " " + sql;
    return this->execute(cmd);
}

bool Mysql::update_record(const std::string& table, const std::string& sql) {
    std::string cmd = "update " + table + " set " + sql;
    return this->execute(cmd);
}

bool Mysql::replace_record(const std::string& table, const std::string& sql) {
    std::string cmd = "replace into " + table + " " + sql;
    return this->execute(cmd);
}

bool Mysql::del_record(const std::string& table, const std::string& cond) {
    std::string cmd = "delete from " + table + " " + cond;
    return this->execute(cmd);
}

std::string Mysql::escape_string(const std::string& s) {
    while (_mysql == NULL) {
        ELOG << __FUNCTION__ << ": wait for connecting mysql...";
        sys::msleep(20);
    }

    std::string to;
    to.resize(s.size() * 2 + 1);

    auto len = mysql_real_escape_string(NULL, &to[0], s.data(), s.size());
    to.resize(len);

    return to;
}

} // namespace util
