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

    int type() const {
        return _reply->type;
    }

    std::string to_string() const {
        return std::string(_reply->str, _reply->len);
    }

    std::vector<std::string> cluster_slots() const;

    bool is_error() const {
        return this->type() == REDIS_REPLY_ERROR;
    }

    bool is_string() const {
        return this->type() == REDIS_REPLY_STRING;
    }

    bool is_nil() const {
        return this->type() == REDIS_REPLY_NIL;
    }

    bool is_array() const {
        return this->type() == REDIS_REPLY_ARRAY;
    }

    bool is_integer() const {
        return this->type() == REDIS_REPLY_INTEGER;
    }

    void reset(redisReply* reply = NULL) {
        _reply = reply;
    }

  public:
    static const int str = REDIS_REPLY_STRING;
    static const int array = REDIS_REPLY_ARRAY;
    static const int integer = REDIS_REPLY_INTEGER;
    static const int nil = REDIS_REPLY_NIL;
    static const int status = REDIS_REPLY_STATUS;
    static const int error = REDIS_REPLY_ERROR;

  private:
    redisReply* _reply;
};

std::vector<std::string> RedisReply::cluster_slots() const {
    std::vector<std::string> vs;

    for (size_t i = 0; i < _reply->elements; ++i) {
        std::string s;

        auto reply = _reply->element[i];

        if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 2) {
            const auto& e = reply->element;
            if (e[0]->type != REDIS_REPLY_INTEGER ||
                e[1]->type != REDIS_REPLY_INTEGER) {
                goto err;
            }

            s += util::to_string(e[0]->integer) + "~" +
                 util::to_string(e[1]->integer) + " ";

            for (auto k = 2; k < reply->elements; ++k) {
                if (e[k]->type != REDIS_REPLY_ARRAY) goto err;
                if (e[k]->elements != 2) goto err;

                const auto& ee = e[k]->element;

                if (ee[0]->type != REDIS_REPLY_STRING ||
                    ee[1]->type != REDIS_REPLY_INTEGER) {
                    goto err;
                }

                if (k != 2) s += ",";
                s += std::string(ee[0]->str, ee[0]->len);
                s += ":" + util::to_string(ee[1]->integer);
            }

        } else {
            goto err;
        }

        vs.push_back(s);
    }

    return vs;

    err:
    TLOG("redis") << "redis cluster slots error";
    return std::vector<std::string>();
}
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
    if (_ctx == NULL && !this->connect()) {
        if (err != NULL) *err = "connect error";
        return false;
    }

    value->clear();

    RedisReply reply = run_cmd(_ctx, "GET %b", key.data(), key.size());

    if (reply.ok()) {
        if (reply.is_string()) {
            *value = reply.to_string();
            return true;
        }

        if (reply.is_nil()) return false;

        if (reply.is_error()) {
            if (err != NULL) *err = reply.to_string();
            TLOG("redis") << "redis get error: " << reply.to_string();
        } else {
            if (err != NULL) *err = "unknown reply type";
        }

        return false;
    }

    TLOG("redis") << "redis get error, key: " << key << ", err: "
        << this->on_error(_ctx->err);

    if (err != NULL) *err = "connect error";
    return false;
}

bool Redis::set(const std::string& key, const std::string& value,
                std::string* err) {
    if (_ctx == NULL && !this->connect()) {
        if (err != NULL) *err = "connect error";
        return false;
    }

    RedisReply reply = run_cmd(_ctx, "SET %b %b", key.data(), key.size(),
                               value.data(), value.size());

    if (reply.ok()) {
        if (reply.is_error()) {
            if (err != NULL) *err = reply.to_string();

            TLOG("redis") << "redis set error: " << reply.to_string();
            return false;
        }

        return true;
    }

    TLOG("redis") << "redis set error, key: " << key << ", err: "
        << this->on_error(_ctx->err);

    if (err != NULL) *err = "connect error";
    return false;
}

bool Redis::set(const std::string& key, const std::string& value,
                int32 expired, std::string* err) {
    if (_ctx == NULL && !this->connect()) {
        if (err != NULL) *err = "connect error";
        return false;
    }

    if (expired < 0) return this->set(key, value, err);

    RedisReply reply= run_cmd(_ctx, "SETEX %b %d %b", key.data(), key.size(),
                              expired, value.data(), value.size());

    if (reply.ok()) {
        if (reply.is_error()) {
            if (err != NULL) *err = reply.to_string();

            TLOG("redis") << "redis set REPLY_ERROR: " << reply.to_string();
            return false;
        }

        return true;
    }

    TLOG("redis") << "redis set error, key: " << key << ", err: "
        << this->on_error(_ctx->err);

    if (err != NULL) *err = "connect error";
    return false;
}

bool Redis::del(const std::string& key, std::string* err) {
    if (_ctx == NULL && !connect()) {
        if (err != NULL) *err = "connect error";
        return false;
    }

    RedisReply reply = run_cmd(_ctx, "DEL %b", key.data(), key.size());

    if (!reply.ok()) {
        TLOG("redis") << "redis del error, key: " << key << ", "
            << this->on_error(_ctx->err);

        if (err != NULL) *err = "unknow error";
        return false;
    }

    if (reply.is_error()) {
        if (err != NULL) *err = reply.to_string();
        TLOG("redis") << "redis del error: " << reply.to_string();
        return false;
    }

    return true;
}

std::string Redis::cluster_nodes() {
    if (_ctx == NULL && !connect()) return std::string();

    RedisReply reply = run_cmd(_ctx, "CLUSTER NODES");
    if (!reply.ok()) return std::string();

    if (reply.is_string()) {
        return reply.to_string();
    }

    if (reply.is_error()) {
        TLOG("redis") << "CLUSTER NODES error: " << reply.to_string();
        return reply.to_string();

    } else if (reply.is_nil()) {
        TLOG("redis") << "CLUSTER NODES nil reply";
    }

    return std::string();
}

std::string Redis::cluster_slots() {
    if (_ctx == NULL && !connect()) return std::string();

    RedisReply reply = run_cmd(_ctx, "CLUSTER SLOTS");
    if (!reply.ok()) return std::string();

    if (!reply.is_array()) {
        TLOG("redis") << "cluster slots error, result type not array: "
            << reply.type();
        return std::string();
    }

    return util::to_string(reply.cluster_slots());
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
