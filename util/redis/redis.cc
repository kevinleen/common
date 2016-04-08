#include "redis.h"

#define run_cmd (redisReply*) redisCommand

namespace {

struct RedisReply {
    RedisReply(redisReply* reply)
        : _reply(reply) {
    }

    ~RedisReply() {
        if (_reply != NULL) freeReplyObject(_reply);
    }

    bool ok() const {
        return _reply != NULL;
    }

    bool is_error() const {
        return this->type() == REDIS_REPLY_ERROR;
    }

    bool is_string() const {
        return this->type() == REDIS_REPLY_STRING;
    }

    bool is_nil() const {
        return this->type() == REDIS_REPLY_NIL;
    }

    std::string value() const {
        return std::string(_reply->str, _reply->len);
    }

    int type() const {
        return _reply->type;
    }

  private:
    redisReply* _reply;
};

}

namespace util {

bool Redis::connect() {
    if (_ctx != NULL) return true;

    struct timeval tv = { 1, 0 };
    _ctx = redisConnectWithTimeout(_ip.c_str(), _port, tv);
    CHECK_NOTNULL(_ctx);

    if (_ctx->err == REDIS_OK) {
        tv.tv_sec = _timeout / 1000;
        tv.tv_usec = _timeout % 1000 * 1000;

        auto ret = redisSetTimeout(_ctx, tv);
        CHECK_EQ(ret, REDIS_OK);

        return true;
    }

    TLOG("redis") << "connect to redis |" << _ip << ":" << _port << " "
        << _timeout << "| failed: " << _ctx->errstr;
    return false;
}

bool Redis::get(const std::string& key, std::string* value,
                std::string* err) {
    if (_ctx == NULL && !this->connect()) return false;

    value->clear();

    RedisReply reply = run_cmd(_ctx, "GET %b", key.data(), key.size());

    if (reply.ok()) {
        if (reply.is_string()) {
            *value = reply.value();
            return true;
        }

        if (reply.is_nil()) return false;

        if (reply.is_error()) {
            if (err != NULL) *err = reply.value();
            TLOG("redis") << "redis get error: " << reply.value();
        }

        return false;
    }

    TLOG("redis") << "redis get error, key: " << key << ", err: "
        << this->on_error(_ctx->err);

    return false;
}

bool Redis::set(const std::string& key, const std::string& value,
                std::string* err) {
    if (_ctx == NULL && !this->connect()) return false;

    RedisReply reply = run_cmd(_ctx, "SET %b %b", key.data(), key.size(),
                               value.data(), value.size());

    if (reply.ok()) {
        if (reply.is_error()) {
            if (err != NULL) *err = reply.value();

            TLOG("redis") << "redis set error: " << reply.value();
            return false;
        }

        return true;
    }

    TLOG("redis") << "redis set error, key: " << key << ", err: "
        << this->on_error(_ctx->err);

    return false;
}

bool Redis::set(const std::string& key, const std::string& value,
                int32 expired, std::string* err) {
    if (_ctx == NULL && !this->connect()) return false;

    if (expired < 0) return this->set(key, value, err);

    RedisReply reply= run_cmd(_ctx, "SETEX %b %d %b", key.data(), key.size(),
                              expired, value.data(), value.size());

    if (reply.ok()) {
        if (reply.is_error()) {
            if (err != NULL) *err = reply.value();

            TLOG("redis") << "redis set REPLY_ERROR: " << reply.value();
            return false;
        }

        return true;
    }

    TLOG("redis") << "redis set error, key: " << key << ", err: "
        << this->on_error(_ctx->err);
    return false;
}

bool Redis::del(const std::string& key, std::string* err) {
    if (_ctx == NULL && !connect()) return false;

    RedisReply reply = run_cmd(_ctx, "DEL %b", key.data(), key.size());

    if (!reply.ok()) {
        TLOG("redis") << "redis del error, key: " << key << ", "
            << this->on_error(_ctx->err);
        return false;
    }

    if (reply.is_error()) {
        if (err != NULL) *err = reply.value();
        TLOG("redis") << "redis del error: " << reply.value();
        return false;
    }

    return true;
}

std::string Redis::cluster_nodes() {
    if (_ctx == NULL && !connect()) return std::string();

    RedisReply reply = run_cmd(_ctx, "CLUSTER NODES");
    if (!reply.ok()) return std::string();

    CHECK(reply.is_string());
    return reply.value();
}

const std::string& Redis::on_error(int err) {
    static std::vector<std::string> err_str {
        "", "ERR_IO", "ERR_OTHER",
        "ERR_EOF: server close the connection",
        "ERR_PROTO", "ERR_OOM: out of memory",
    };

    if (err < 0 || err >= err_str.size()) return err_str[0];

    if (err == REDIS_ERR_IO || err == REDIS_ERR_EOF ||
        err == REDIS_ERR_PROTOCOL) {
        this->disconnect();
    }

    return err_str[err];
}

}  // namespace util
