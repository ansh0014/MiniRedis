#include "ApiController.h"
#include "../config/config.h"
#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <sw/redis++/redis++.h>
#include <random>
#include <iomanip>
#include <sstream>

using namespace drogon;
using namespace std;
using namespace sw::redis;

// Generate UUID v4 using standard C++
static string generateUuid()
{
    static random_device rd;
    static mt19937_64 gen(rd());
    static uniform_int_distribution<uint64_t> dis;
    
    ostringstream oss;
    oss << hex << setfill('0');
    
    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);
    
    // UUID format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    oss << setw(8) << (part1 & 0xFFFFFFFF) << "-";
    oss << setw(4) << ((part1 >> 32) & 0xFFFF) << "-";
    oss << setw(4) << ((part2 & 0x0FFF) | 0x4000) << "-";  // Version 4
    oss << setw(4) << ((part2 >> 16) & 0x3FFF | 0x8000) << "-";  // Variant
    oss << setw(12) << ((part2 >> 32) & 0xFFFFFFFF);
    oss << setw(4) << (dis(gen) & 0xFFFF);
    
    return oss.str();
}

// Generate 256-bit API key
static string generateApiKeyHex()
{
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dis;
    ostringstream oss;
    oss << hex << setfill('0');
    for (int i = 0; i < 4; ++i) {
        uint64_t v = dis(gen);
        oss << setw(16) << v;
    }
    return oss.str();
}

ApiController::ApiController()
{
    db_ = drogon::app().getDbClient("default");
    
    // Load Redis config from EnvLoader
    string redisHost = EnvLoader::get("REDIS_HOST", "localhost");
    int redisPort = EnvLoader::getInt("REDIS_PORT", 6379);
    
    try {
        ConnectionOptions opts;
        opts.host = redisHost;
        opts.port = redisPort;
        opts.socket_timeout = chrono::milliseconds(100);
        redis_ = make_unique<Redis>(opts);
        LOG_INFO << "Redis connected: " << redisHost << ":" << redisPort;
    } catch (const exception &ex) {
        LOG_ERROR << "Redis init failed: " << ex.what();
        redis_.reset();
    }
}

void ApiController::createTenant(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !(*json).isMember("name") || !(*json).isMember("node_port")) {
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k400BadRequest);
        r->setBody(R"({"error":"name and node_port required"})");
        callback(r);
        return;
    }

    string name = (*json)["name"].asString();
    int port = (*json)["node_port"].asInt();
    int mem = (*json).isMember("memory_limit_mb") ? (*json)["memory_limit_mb"].asInt() : 40;

    string uuid = generateUuid();

    auto sql = "INSERT INTO tenants (id, name, node_port, memory_limit_mb) VALUES ($1, $2, $3, $4)";
    db_->execSqlAsync(sql,
        [uuid, callback](const drogon::orm::Result &) {
            Json::Value out;
            out["tenant_id"] = uuid;
            auto resp = HttpResponse::newHttpJsonResponse(out);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        uuid, name, port, mem);
}

void ApiController::getTenant(const HttpRequestPtr &, function<void(const HttpResponsePtr &)> &&callback, const string &tenantId)
{
    auto sql = "SELECT id, name, node_port, memory_limit_mb FROM tenants WHERE id = $1";
    db_->execSqlAsync(sql,
        [callback](const drogon::orm::Result &r) {
            if (r.size() == 0) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("{\"error\":\"tenant not found\"}");
                callback(resp);
                return;
            }

            Json::Value out;
            out["tenant_id"] = r[0]["id"].as<string>();
            out["name"] = r[0]["name"].as<string>();
            out["node_port"] = r[0]["node_port"].as<int>();
            out["memory_limit_mb"] = r[0]["memory_limit_mb"].as<int>();
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        tenantId);
}

void ApiController::createApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !(*json).isMember("tenant_id")) {
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k400BadRequest);
        r->setBody(R"({"error":"tenant_id required"})");
        callback(r);
        return;
    }

    string tenantId = (*json)["tenant_id"].asString();
    string desc = (*json).isMember("description") ? (*json)["description"].asString() : "";
    string apiKey = generateApiKeyHex();

    auto sql = "INSERT INTO api_keys (key, tenant_id, description) VALUES ($1, $2, $3)";
    db_->execSqlAsync(sql,
        [apiKey, tenantId, callback, this](const drogon::orm::Result &) {
            Json::Value out;
            out["api_key"] = apiKey;
            out["tenant_id"] = tenantId;
            auto resp = HttpResponse::newHttpJsonResponse(out);
            callback(resp);
            
            if (redis_) {
                try {
                    redis_->set("apikey:" + apiKey, tenantId);
                } catch (const exception &ex) {
                    LOG_WARN << "Redis cache set failed: " << ex.what();
                }
            }
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        apiKey, tenantId, desc);
}

void ApiController::verifyApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
{
    string key = req->getParameter("key");
    if (key.empty()) {
        auto auth = req->getHeader("Authorization");
        if (auth.rfind("Bearer ", 0) == 0) key = auth.substr(7);
    }
    if (key.empty()) {
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k400BadRequest);
        r->setBody("{\"error\":\"no key provided\"}");
        callback(r);
        return;
    }

    if (redis_) {
        try {
            auto val = redis_->get("apikey:" + key);
            if (val) {
                Json::Value out;
                out["tenant_id"] = *val;
                callback(HttpResponse::newHttpJsonResponse(out));
                return;
            }
        } catch (const exception &ex) {
            LOG_WARN << "Redis cache get failed: " << ex.what();
        }
    }

    auto sql = "SELECT tenant_id FROM api_keys WHERE key = $1";
    db_->execSqlAsync(sql,
        [callback, key, this](const drogon::orm::Result &r) {
            if (r.size() == 0) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("{\"error\":\"invalid key\"}");
                callback(resp);
                return;
            }
            string tenantId = r[0]["tenant_id"].as<string>();

            Json::Value out;
            out["tenant_id"] = tenantId;
            callback(HttpResponse::newHttpJsonResponse(out));
            
            if (redis_) {
                try {
                    redis_->set("apikey:" + key, tenantId);
                } catch (const exception &ex) {
                    LOG_WARN << "Redis cache update failed: " << ex.what();
                }
            }
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        key);
}

void ApiController::revokeApiKey(const HttpRequestPtr &, function<void(const HttpResponsePtr &)> &&callback, const string &key)
{
    if (key.empty()) {
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k400BadRequest);
        r->setBody("{\"error\":\"key required\"}");
        callback(r);
        return;
    }

    auto sql = "DELETE FROM api_keys WHERE key = $1";
    db_->execSqlAsync(sql,
        [callback, key, this](const drogon::orm::Result &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("{\"status\":\"revoked\"}");
            callback(resp);
            
            if (redis_) {
                try {
                    redis_->del("apikey:" + key);
                } catch (const exception &ex) {
                    LOG_WARN << "Redis cache delete failed: " << ex.what();
                }
            }
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        key);
}