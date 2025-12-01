#pragma once
#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include <memory>
#include <string>

// Forward declarations
namespace sw { namespace redis { class Redis; } }

class ApiController : public drogon::HttpController<ApiController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ApiController::createTenant, "/api/tenants", Post);
    ADD_METHOD_TO(ApiController::getTenant, "/api/tenants/{1}", Get);
    ADD_METHOD_TO(ApiController::createApiKey, "/api/apikeys", Post);
    ADD_METHOD_TO(ApiController::verifyApiKey, "/api/verify", Get);
    ADD_METHOD_TO(ApiController::revokeApiKey, "/api/apikeys/{1}", Delete);
    METHOD_LIST_END

    ApiController();

    void createTenant(const drogon::HttpRequestPtr &req, 
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getTenant(const drogon::HttpRequestPtr &req, 
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  const std::string &tenantId);
    void createApiKey(const drogon::HttpRequestPtr &req, 
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void verifyApiKey(const drogon::HttpRequestPtr &req, 
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void revokeApiKey(const drogon::HttpRequestPtr &req, 
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     const std::string &key);

private:
    drogon::orm::DbClientPtr db_;
    std::unique_ptr<sw::redis::Redis> redis_;
};