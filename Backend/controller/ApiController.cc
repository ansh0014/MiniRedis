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

string ApiController::generateApiKeyHex()
{
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dis;
    ostringstream oss;
    oss << "mk_" << hex << setfill('0');
    for (int i = 0; i < 4; ++i) {
        uint64_t v = dis(gen);
        oss << setw(8) << (v & 0xFFFFFFFF);
    }
    return oss.str();
}

string ApiController::generateUuid()
{
    static random_device rd;
    static mt19937_64 gen(rd());
    static uniform_int_distribution<uint64_t> dis;
    
    ostringstream oss;
    oss << hex << setfill('0');
    
    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);
    
    oss << setw(8) << (part1 & 0xFFFFFFFF) << "-";
    oss << setw(4) << ((part1 >> 32) & 0xFFFF) << "-";
    oss << setw(4) << ((part2 & 0x0FFF) | 0x4000) << "-";
    oss << setw(4) << ((part2 >> 16) & 0x3FFF | 0x8000) << "-";
    oss << setw(12) << ((part2 >> 32) & 0xFFFFFFFF);
    oss << setw(4) << (dis(gen) & 0xFFFF);
    
    return oss.str();
}

ApiController::ApiController()
{
    db_ = drogon::app().getDbClient("default");
    
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

int ApiController::findAvailablePort()
{
    int startPort = 6380;
    int maxPort = 6480; 
    
    auto sql = "SELECT node_port FROM tenants ORDER BY node_port";
    auto result = db_->execSqlSync(sql);
    
    std::set<int> usedPorts;
    for (size_t i = 0; i < result.size(); ++i) {
        usedPorts.insert(result[i]["node_port"].as<int>());
    }
    
    for (int port = startPort; port <= maxPort; ++port) {
        if (usedPorts.find(port) == usedPorts.end()) {
            return port;
        }
    }
    
    return -1; 
}

void ApiController::createTenant(const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    
    if (!json || !(*json).isMember("name")) {
        Json::Value error;
        error["error"] = "name is required";
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    string name = (*json)["name"].asString();
    string firebaseUid = (*json).get("firebase_uid", "").asString();
    int memoryMb = (*json).get("memory_limit_mb", 40).asInt();

    // Auto-assign available port
    int nodePort = findAvailablePort();
    if (nodePort == -1) {
        Json::Value error;
        error["error"] = "No available ports (max 100 tenants)";
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k507InsufficientStorage);
        callback(resp);
        return;
    }

    cout << "[Backend] Creating tenant: " << name << endl;
    cout << "[Backend]   Firebase UID: " << firebaseUid << endl;
    cout << "[Backend]   Port: " << nodePort << " (auto-assigned)" << endl;
    cout << "[Backend]   Memory: " << memoryMb << "MB" << endl;

    // Let PostgreSQL generate UUID using uuid_generate_v4()
    auto sql = "INSERT INTO tenants (id, name, node_port, firebase_uid, memory_limit_mb) "
               "VALUES (uuid_generate_v4(), $1, $2, $3, $4) RETURNING id::text";
    
    db_->execSqlAsync(sql,
        [callback, nodePort, memoryMb, name](const drogon::orm::Result &result) {
            
            string tenantId = result[0][0].as<string>();
            
            
            cout << "[Backend] Starting node via Node Manager..." << endl;
            
            auto nodeClient = HttpClient::newHttpClient("http://localhost:7000");
            Json::Value nodeJson;
            nodeJson["tenant_id"] = tenantId;
            nodeJson["port"] = nodePort;
            nodeJson["memory_limit_mb"] = memoryMb;
            
            auto nodeReq = HttpRequest::newHttpJsonRequest(nodeJson);
            nodeReq->setMethod(Post);
            nodeReq->setPath("/node/start");

            nodeClient->sendRequest(nodeReq, [callback, tenantId, nodePort](ReqResult result, const HttpResponsePtr& nodeResp) {
                if (result == ReqResult::Ok && nodeResp->getStatusCode() == k200OK) {
                    cout << "[Backend]  Node started successfully" << endl;
                    
                    Json::Value response;
                    response["tenant_id"] = tenantId;
                    response["port"] = nodePort;
                    response["status"] = "running";
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                } else {
                    cout << " Failed to start node" << endl;
                    if (result != ReqResult::Ok) {
                        cout << " Request failed with result code: "  << endl;
                    } else {
                        cout << " Node Manager returned status: "<< endl;
                    }
                    
                    Json::Value response;
                    response["error"] = "Failed to start node";
                    response["tenant_id"] = tenantId;
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                }
            });
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            cout << "[Backend]  Failed to create tenant: " << endl;
            
            Json::Value response;
            response["error"] = "Failed to create tenant in database";
            response["details"] = e.base().what();
            
            auto resp = HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        name, nodePort, firebaseUid, memoryMb);
}

void ApiController::getTenant(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &tenantId)
{
    auto sql = "SELECT id, name, node_port, firebase_uid, status FROM tenants WHERE id = $1";
    db_->execSqlAsync(sql,
        [callback](const drogon::orm::Result &r) {
            if (r.size() == 0) {
                Json::Value error;
                error["error"] = "tenant not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                callback(resp);
                return;
            }

            Json::Value out;
            out["tenant_id"] = r[0]["id"].as<string>();
            out["name"] = r[0]["name"].as<string>();
            out["node_port"] = r[0]["node_port"].as<int>();
            out["firebase_uid"] = r[0]["firebase_uid"].as<string>();
            out["status"] = r[0]["status"].as<string>();
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            Json::Value error;
            error["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        tenantId);
}

void ApiController::getUserTenants(const HttpRequestPtr &req,
                                   function<void(const HttpResponsePtr &)> &&callback,
                                   const string &firebaseUid)
{
    cout << "[Backend] Getting tenants for user: " << firebaseUid << endl;
    
    auto sql = "SELECT id, name, node_port, status, created_at FROM tenants WHERE firebase_uid = $1";
    db_->execSqlAsync(sql,
        [callback, firebaseUid](const drogon::orm::Result &r) {
            Json::Value tenants(Json::arrayValue);
            for (size_t i = 0; i < r.size(); ++i) {
                Json::Value tenant;
                tenant["tenant_id"] = r[i]["id"].as<string>();
                tenant["name"] = r[i]["name"].as<string>();
                tenant["port"] = r[i]["node_port"].as<int>();
                tenant["status"] = r[i]["status"].as<string>();
                tenant["created_at"] = r[i]["created_at"].as<string>();
                tenants.append(tenant);
            }
            
            cout << "[Backend]  Found " << tenants.size() << " tenants for user: " << endl;
            
            auto resp = HttpResponse::newHttpJsonResponse(tenants);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            cout << " Database error: " << e.base().what() << endl;
            
            Json::Value error;
            error["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        firebaseUid);
}

void ApiController::createApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json || !(*json).isMember("tenant_id")) {
        Json::Value error;
        error["error"] = "tenant_id required";
        auto r = HttpResponse::newHttpJsonResponse(error);
        r->setStatusCode(k400BadRequest);
        callback(r);
        return;
    }

    string tenantId = (*json)["tenant_id"].asString();
    string desc = (*json).isMember("description") ? (*json)["description"].asString() : "";
    string apiKey = generateApiKeyHex();

    auto sql = "INSERT INTO api_keys (key, tenant_id, description) VALUES ($1, $2, $3)";
    db_->execSqlAsync(sql,
        [apiKey, tenantId, callback, this](const drogon::orm::Result &) {
            cout << " API key created for tenant: "  << endl;
            
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
        [callback](const drogon::orm::DrogonDbException &e) {
            Json::Value error;
            error["error"] = "Failed to create API key";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
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
        Json::Value error;
        error["error"] = "no key provided";
        auto r = HttpResponse::newHttpJsonResponse(error);
        r->setStatusCode(k400BadRequest);
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
                Json::Value error;
                error["error"] = "invalid key";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k401Unauthorized);
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
        [callback](const drogon::orm::DrogonDbException &e) {
            Json::Value error;
            error["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        key);
}

void ApiController::revokeApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &key)
{
    auto sql = "DELETE FROM api_keys WHERE key = $1";
    db_->execSqlAsync(sql,
        [callback, key, this](const drogon::orm::Result &) {
            Json::Value success;
            success["status"] = "revoked";
            auto resp = HttpResponse::newHttpJsonResponse(success);
            callback(resp);
            
            if (redis_) {
                try {
                    redis_->del("apikey:" + key);
                } catch (const exception &ex) {
                    LOG_WARN << "Redis cache delete failed: " << ex.what();
                }
            }
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            Json::Value error;
            error["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        key);
}

void ApiController::getRateLimit(const HttpRequestPtr &req,
                                 function<void(const HttpResponsePtr &)> &&callback,
                                 const string &tenantId)
{
    auto sql = "SELECT * FROM rate_limits WHERE tenant_id = $1";
    db_->execSqlAsync(sql,
        [callback](const drogon::orm::Result &r) {
            if (r.size() == 0) {
                Json::Value error;
                error["error"] = "Rate limit not found";
                auto resp = HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(k404NotFound);
                callback(resp);
                return;
            }

            Json::Value out;
            out["tenant_id"] = r[0]["tenant_id"].as<string>();
            out["max_requests_per_second"] = r[0]["max_requests_per_second"].as<int>();
            out["max_requests_per_minute"] = r[0]["max_requests_per_minute"].as<int>();
            out["max_requests_per_hour"] = r[0]["max_requests_per_hour"].as<int>();
            out["max_requests_per_day"] = r[0]["max_requests_per_day"].as<int>();
            out["requests_current_second"] = r[0]["requests_current_second"].as<int>();
            out["requests_current_minute"] = r[0]["requests_current_minute"].as<int>();
            out["requests_current_hour"] = r[0]["requests_current_hour"].as<int>();
            out["requests_current_day"] = r[0]["requests_current_day"].as<int>();
            
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            Json::Value error;
            error["error"] = "database error";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        tenantId);
}

void ApiController::updateRateLimit(const HttpRequestPtr &req,
                                    function<void(const HttpResponsePtr &)> &&callback,
                                    const string &tenantId)
{
    auto json = req->getJsonObject();
    
    int maxPerSecond = (*json).get("max_requests_per_second", 100).asInt();
    int maxPerMinute = (*json).get("max_requests_per_minute", 1000).asInt();
    int maxPerHour = (*json).get("max_requests_per_hour", 10000).asInt();
    int maxPerDay = (*json).get("max_requests_per_day", 100000).asInt();

    auto sql = "UPDATE rate_limits SET "
               "max_requests_per_second = $1, "
               "max_requests_per_minute = $2, "
               "max_requests_per_hour = $3, "
               "max_requests_per_day = $4, "
               "updated_at = now() "
               "WHERE tenant_id = $5";
    
    db_->execSqlAsync(sql,
        [callback, tenantId](const drogon::orm::Result &) {
            Json::Value out;
            out["success"] = true;
            out["tenant_id"] = tenantId;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            Json::Value error;
            error["error"] = "Failed to update rate limit";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        maxPerSecond, maxPerMinute, maxPerHour, maxPerDay, tenantId);
}

void ApiController::checkRateLimit(const HttpRequestPtr &req,
                                   function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    string tenantId = (*json)["tenant_id"].asString();

    // Use Redis for fast rate limit checking
    if (redis_) {
        try {
            string key = "ratelimit:" + tenantId + ":second";
            auto count = redis_->incr(key);
            redis_->expire(key, chrono::seconds(1));

            // Get max limit from DB
            auto sql = "SELECT max_requests_per_second FROM rate_limits WHERE tenant_id = $1";
            db_->execSqlAsync(sql,
                [callback, count](const drogon::orm::Result &r) {
                    if (r.size() == 0) {
                        Json::Value error;
                        error["error"] = "Rate limit not configured";
                        auto resp = HttpResponse::newHttpJsonResponse(error);
                        resp->setStatusCode(k404NotFound);
                        callback(resp);
                        return;
                    }

                    int maxPerSecond = r[0]["max_requests_per_second"].as<int>();
                    bool allowed = count <= maxPerSecond;

                    Json::Value out;
                    out["allowed"] = allowed;
                    out["current_count"] = (long long)count;
                    out["max_allowed"] = maxPerSecond;
                    out["remaining"] = max(0, maxPerSecond - (int)count);

                    auto resp = HttpResponse::newHttpJsonResponse(out);
                    if (!allowed) {
                        resp->setStatusCode(k429TooManyRequests);
                    }
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    Json::Value error;
                    error["error"] = "database error";
                    auto resp = HttpResponse::newHttpJsonResponse(error);
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                },
                tenantId);

        } catch (const exception &ex) {
            LOG_ERROR << "Redis rate limit check failed: " << ex.what();
            Json::Value error;
            error["error"] = "Rate limit check failed";
            auto resp = HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        }
    } else {
        Json::Value error;
        error["error"] = "Rate limiting not available";
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k503ServiceUnavailable);
        callback(resp);
    }
}