#include "ApiController.h"
#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <random>
#include <iomanip>
#include <sstream>
#include <exception>

// redis++
#include <sw/redis++/redis++.h>

using namespace drogon;
using namespace std;
using namespace sw::redis;

static string generateApiKeyHex()
{
    // 256-bit (32 bytes) hex token using CSPRNG
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << hex << setfill('0');
    for (int i = 0; i < 4; ++i) {
        uint64_t v = dis(gen);
        oss << setw(16) << v;
    }
    return oss.str();
}

ApiController::ApiController()
{
    // Get drogon DB client named "postgres" configured in config.json
    db_ = drogon::app().getDbClient("postgres");

    // Create redis client (adjust URI if needed)
    try {
        redis_ = make_unique<Redis>("tcp://127.0.0.1:6379");
    } catch (const std::exception &ex) {
        LOG_ERROR << "Redis init failed: " << ex.what();
        redis_.reset();
    }
}

void ApiController::createTenant(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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

    boost::uuids::uuid u = boost::uuids::random_generator()();
    string uuid = boost::uuids::to_string(u);

    auto sql = "INSERT INTO tenants (id, name, node_port, memory_limit_mb) VALUES ($1, $2, $3, $4)";
    db_->execSqlAsync(sql,
        [uuid, callback](const drogon::orm::Result &r) {
            Json::Value out;
            out["tenant_id"] = uuid;
            auto resp = HttpResponse::newHttpJsonResponse(out);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &err) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        uuid, name, port, mem);
}

void ApiController::createApiKey(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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
        [this, apiKey, tenantId, callback](const drogon::orm::Result &r) {
            // write to redis for fast lookup
            try {
                if (redis_) {
                    redis_->set("apikey:" + apiKey, tenantId);
                }
            } catch (const std::exception &ex) {
                LOG_WARN << "Redis SET failed: " << ex.what();
            }

            Json::Value out;
            out["api_key"] = apiKey;
            out["tenant_id"] = tenantId;
            auto resp = HttpResponse::newHttpJsonResponse(out);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &err) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        apiKey, tenantId, desc);
}

void ApiController::verifyApiKey(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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

    // Check Redis first
    try {
        if (redis_) {
            auto val = redis_->get("apikey:" + key);
            if (val) {
                Json::Value out;
                out["tenant_id"] = *val;
                callback(HttpResponse::newHttpJsonResponse(out));
                return;
            }
        }
    } catch (const std::exception &ex) {
        LOG_WARN << "Redis GET failed: " << ex.what();
    }

    // Fallback to Postgres
    auto sql = "SELECT tenant_id FROM api_keys WHERE key = $1";
    db_->execSqlAsync(sql,
        [this, key, callback](const drogon::orm::Result &r) {
            if (r.size() == 0) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("{\"error\":\"invalid key\"}");
                callback(resp);
                return;
            }
            string tenantId = r[0]["tenant_id"].as<string>();

            try {
                if (redis_) redis_->set("apikey:" + key, tenantId);
            } catch (...) {}

            Json::Value out;
            out["tenant_id"] = tenantId;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &err) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        key);
}

void ApiController::revokeApiKey(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    string key = req->getParameter("key");
    if (key.empty()) {
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k400BadRequest);
        r->setBody("{\"error\":\"key required\"}");
        callback(r);
        return;
    }

    auto sql = "DELETE FROM api_keys WHERE key = $1";
    db_->execSqlAsync(sql,
        [this, key, callback](const drogon::orm::Result &r) {
            try { if (redis_) redis_->del("apikey:" + key); } catch(...) {}
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("{\"status\":\"revoked\"}");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &err) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("{\"error\":\"db error\"}");
            callback(resp);
        },
        key);
}