#pragma once

#include "base/base.h"
#include "include/thread_safe.h"
#include <mysql/mysql.h>

namespace util {

class Mysql {
  public:
    typedef std::vector<std::map<std::string, std::string>> MysqlData;

    Mysql(const std::string& ip, uint32 port,
          const std::string& user, const std::string& password,
          const std::string& db, const std::string& charset = "");

    ~Mysql();


    bool connect();
    void disconnect();

    bool execute(const std::string& sql);

    MysqlData query_record(const std::string& sql);

    /*
     * insert_record("table", "(`int1`,`str2`) values(3,'hello')");
     */
    bool insert_record(const std::string& table, const std::string& sql);

    bool replace_record(const std::string& table, const std::string& sql);

    /*
     * update_record("table", "`int1`=5,`str2`='world'");
     */
    bool update_record(const std::string& table, const std::string& sql);

    bool del_record(const std::string& table, const std::string& cond);

    std::string escape_string(const std::string& s);

  private:
    MYSQL* _mysql;

    std::string _ip;
    uint32 _port;

    std::string _user;
    std::string _password;
    std::string _db;
    std::string _charset;

    DISALLOW_COPY_AND_ASSIGN(Mysql);
};

typedef ::ThreadSafeHandlerWrapper<util::Mysql> ThreadSafeMysql;

class ThreadSafeMysqlDelegate : public ThreadSafeMysql::Delegate {
  public:
    ThreadSafeMysqlDelegate(const std::string& ip, uint32 port,
                            const std::string& user, const std::string& password,
                            const std::string& db,
                            const std::string& charset)
        : _ip(ip), _port(port), _user(user), _password(password),
          _db(db), _charset(charset) {
    }

    virtual ~ThreadSafeMysqlDelegate() = default;

    virtual std::shared_ptr<util::Mysql> create() {
        std::shared_ptr<util::Mysql> mysql(
            new Mysql(_ip, _port, _user, _password, _db, _charset));
        return mysql;
    }

  private:
    const std::string _ip;
    uint32 _port;

    const std::string _user;
    const std::string _password;
    const std::string _db;
    const std::string _charset;

    DISALLOW_COPY_AND_ASSIGN(ThreadSafeMysqlDelegate);
};

struct MysqlClient : public ThreadSafeMysql {
    MysqlClient(const std::string& ip, uint32 port,
                const std::string& user, const std::string& password,
                const std::string& db, const std::string& charset = "")
        : ThreadSafeMysql(
            new util::ThreadSafeMysqlDelegate(ip, port, user, password,
                                              db, charset)) {
    }

    virtual ~MysqlClient() = default;
};

inline std::shared_ptr<MysqlClient> CreateMysqlClient(
    const std::string& ip, uint32 port, const std::string& user,
    const std::string& password, const std::string& db,
    const std::string& charset = "") {
    return std::shared_ptr<MysqlClient>(
        new MysqlClient(ip, port, user, password, db, charset));
}

} // namespace util
