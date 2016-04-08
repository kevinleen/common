#include "cassandra.h"

namespace util {

Cassandra::Cassandra(const std::string& ip, uint16 port, uint32 timeout,
                     const std::string& keyspace,
                     const std::string& column_family)
    : _ip(ip), _port(port), _timeout(timeout), _keyspace(keyspace),
      _column_family(column_family) {
    this->connect();
}

Cassandra::~Cassandra() {
}

bool Cassandra::connect() {
    _client.reset(
        util::NewThriftClient<cass::CassandraClient>(_ip, _port, _timeout));
    _client->set_keyspace(_keyspace);

    return true;
}

bool Cassandra::get(const std::string& key, const std::string& column,
                    std::string* value) {
    cass::ColumnPath cp;
    cp.__set_column_family(_column_family);
    cp.__set_super_column(std::string());
    cp.__set_column(column);

    cass::ColumnOrSuperColumn sc;
    try {
        _client->get(sc, key, cp, cass::ConsistencyLevel::ONE);
    } catch (cass::NotFoundException& e) {
        return false;
    } catch (cass::TimedOutException& e) {
        ELOG << "cassandra::get timeout exception: " << e.what();
        return false;
    } catch (std::exception& e) {
        ELOG << "cassandra::get exception: " << e.what();
        return false;
    }

    *value = sc.column.value;
    return true;
}

bool Cassandra::set(const std::string& key, const std::string& column,
                    const std::string& value, int32 ttl) {
    cass::ColumnParent parent;
    parent.__set_column_family(_column_family);

    cass::Column c;
    c.__set_name(column);
    c.__set_value(value);
    c.__set_timestamp(sys::utc.ms());
    if (ttl != -1) c.__set_ttl(ttl);

    try {
        _client->insert(key, parent, c, cass::ConsistencyLevel::ONE);
    } catch (cass::TimedOutException& e) {
        ELOG << "cassandra::set timeout exception: " << e.what();
        return false;
    } catch (std::exception& e) {
        ELOG << "cassandra::set exception: " << e.what();
        return false;
    }

    return true;
}

void Cassandra::remove(const std::string& key, const std::string& column) {
    cass::ColumnPath cp;
    cp.__set_column_family(_column_family);
    cp.__set_super_column(std::string());
    cp.__set_column(column);

    try {
        _client->remove(key, cp, sys::utc.ms(), cass::ConsistencyLevel::ONE);
    } catch (cass::TimedOutException& e) {
        ELOG << "cassandra::remove timeout exception: " << e.what();
    } catch (std::exception& e) {
        ELOG << "cassandra::remove exception: " << e.what();
    }
}

std::shared_ptr<Cassandra> CassandraDelegate::create() {
    auto server = next_server();

    std::shared_ptr<Cassandra> cass(
        new Cassandra(server.first, server.second, _timeout,
                      _keyspace, _column_family));

    return cass;
}

} // namespace util
