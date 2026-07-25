#pragma once

#include "authorization/oauth.h"
#include "models/account.h"
#include "network/http_request.h"
#include "models/download_license.h"

namespace vroid
{
    namespace api
    {
        constexpr auto AUTH_HEADER = "Authorization";
        constexpr auto AUTH_SCHEME_PREFIX = "Bearer ";
        constexpr auto VERSION_HEADER = "X-Api-Version";
        constexpr auto VERSION = "11";
#if 0
        class ApiBase
        {
        public:
            explicit ApiBase(authorization::Oauth::Account account)
                : oauth_account_(std::move(account))
            {
            }
            virtual ~ApiBase() = default;

        protected:
            std::string request(const network::EVerb verb, const std::string& url,
                                const std::vector<network::HttpRequest::Param>& params = {},
                                std::vector<network::HttpRequest::Header>&& headers = {}) const;
            authorization::Oauth::Account oauth_account_;
        };

        class DefaultApi final : ApiBase
        {
        public:
            explicit DefaultApi(const authorization::Oauth::Account& account) : ApiBase(account)
            {
            }

            models::Account get_account() const;
#if 0
            models::PageResponse<models::CharacterModel> get_account_models(const std::string& max_id = "", const int32_t count = 20) const;
#endif
            models::DownloadLicense post_download_license(const std::string& character_model_id, bool is_multiplay) const;
            std::string get_download_license_download(const std::string& download_license_id) const;
        };
#else
        class CoreApi
        {
        public:
            explicit CoreApi(const std::string& in_access_token)
                : access_token_(in_access_token)
            {
            }
            virtual ~CoreApi() = default;

        private:
            std::string request(const network::EVerb verb, const std::string& url,
                                const std::vector<network::HttpRequest::Param>& params = {},
                                std::vector<network::HttpRequest::Header>&& headers = {}) const;

            std::string access_token_;
            
        public:
            void update_access_token(const std::string& access_token);
            models::Account get_account() const;
            models::DownloadLicense post_download_license(const std::string& character_model_id, bool is_multiplay) const;
            std::string get_download_license_download(const std::string& download_license_id) const;
        };
#endif
    }
}
