#pragma once
#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include <sw/redis++/redis++.h>
#include <memory>

using namespace drogon;

class ApiController : public HttpController<ApiController>
{
public:
    METHOD_LIST_BEGIN
    // Existing endpoints
    ADD_METHOD_TO(ApiController::createTenant, "/api/tenant", Post);
    ADD_METHOD_TO(ApiController::getTenant, "/api/tenant/{1}", Get);
    ADD_METHOD_TO(ApiController::createApiKey, "/api/apikey", Post);
    ADD_METHOD_TO(ApiController::verifyApiKey, "/api/apikey/verify", Get);
    ADD_METHOD_TO(ApiController::revokeApiKey, "/api/apikey/{1}", Delete);
    
    // NEW: Firebase-integrated endpoints
    ADD_METHOD_TO(ApiController::createUserInstance, "/api/user/instance", Post);
    ADD_METHOD_TO(ApiController::getUserInstance, "/api/user/{1}/instance", Get);
    ADD_METHOD_TO(ApiController::restartInstance, "/api/instance/{1}/restart", Post);
    ADD_METHOD_TO(ApiController::deleteInstance, "/api/instance/{1}", Delete);
    METHOD_LIST_END

    ApiController();

    // Existing methods
    void createTenant(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getTenant(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &tenantId);
    void createApiKey(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void verifyApiKey(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void revokeApiKey(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &key);

    // NEW: Firebase-integrated methods
    void createUserInstance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getUserInstance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &firebaseUid);
    void restartInstance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &instanceId);
    void deleteInstance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &instanceId);

private:
    drogon::orm::DbClientPtr db_;
    std::unique_ptr<sw::redis::Redis> redis_;
    
    std::string generateApiKeyHex();
    std::string generateUuid();
    std::string createDockerContainer(const std::string &firebaseUid);
};