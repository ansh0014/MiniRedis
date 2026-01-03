#pragma once
#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include <sw/redis++/redis++.h>
#include <memory>

using namespace drogon;
using namespace std;

class ApiController : public HttpController<ApiController>
{
public:
    ApiController();

    METHOD_LIST_BEGIN
    // Tenant management
    ADD_METHOD_TO(ApiController::createTenant, "/api/tenants", Post);
    ADD_METHOD_TO(ApiController::getTenant, "/api/tenants/{1}", Get);
    ADD_METHOD_TO(ApiController::getUserTenants, "/api/user/{1}/tenants", Get);
    
    // API Key management
    ADD_METHOD_TO(ApiController::createApiKey, "/api/apikeys", Post);
    ADD_METHOD_TO(ApiController::verifyApiKey, "/api/verify", Get);
    ADD_METHOD_TO(ApiController::revokeApiKey, "/api/apikeys/{1}", Delete);
    
    // Rate limit management
    ADD_METHOD_TO(ApiController::getRateLimit, "/api/tenants/{1}/ratelimit", Get);
    ADD_METHOD_TO(ApiController::updateRateLimit, "/api/tenants/{1}/ratelimit", Put);
    ADD_METHOD_TO(ApiController::checkRateLimit, "/api/ratelimit/check", Post);
    METHOD_LIST_END

    // Tenant methods
    void createTenant(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback);
    void getTenant(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &tenantId);
    void getUserTenants(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &firebaseUid);
    
    // API Key methods
    void createApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback);
    void verifyApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback);
    void revokeApiKey(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &key);
    
    // Rate limit methods
    void getRateLimit(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &tenantId);
    void updateRateLimit(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &tenantId);
    void checkRateLimit(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback);

private:
    drogon::orm::DbClientPtr db_;
    unique_ptr<sw::redis::Redis> redis_;
    
    string generateApiKeyHex();
    string generateUuid();
    int findAvailablePort();  // ADD THIS
};