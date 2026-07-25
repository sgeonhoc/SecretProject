#pragma once

#include <stdexcept>

namespace vroid
{
    namespace authorization
    {
        class AccountFileParseFailed: public std::exception
        {
        };

        class Oauth final
        {
        public:
            struct Account
            {
                std::string access_token;
                std::string token_type;
                double expires_in;
                std::string refresh_token;
                std::string scope;
                double created_at;
                Account() = default;
            };

            Oauth(std::string application_id, std::string secret_key, std::string redirect_uri);
#if 0
            std::string generate_authorization_url(const std::string& redirect_uri, const std::string& scopes) const;
            Account authorize(const std::string& authorize_code) const;
#else
            std::string generate_authorization_url(const bool is_multiplay) const;
            std::string generate_authorize_token_url(const std::string& auth_code) const;
            std::string generate_refresh_url(const std::string& refresh_token) const;
#endif
            static Account load_account();
            bool introspect(const std::string& token) const;
            Account refresh(const std::string& refresh_token) const;

        private:
            std::string application_id_;
            std::string secret_key_;
            std::string redirect_uri_;
            std::string code_verifier_;
        };
    }
}
