#pragma once

namespace vroid
{
    namespace endpoint
    {
        std::string account();
        std::string art_works();
        std::string authorize(const std::string& client_id, const std::string& redirect_uri, const std::string& scopes,
                              const std::string& code_challenge, const std::string& state);
        std::string authorize_token(const std::string& client_id, const std::string& secret,
                                    const std::string& auth_code, const std::string& redirect_uri,
                                    const std::string& code_verifier);
        std::string refresh_token(const std::string& client_id, const std::string& secret,
                                  const std::string& redirect_uri, const std::string& refresh_token);
        std::string characters();
        std::string character_model();
        std::string character_model_property(const std::string& character_model_id);
        std::string hearts_model();
        std::string staff_picks_model();
        std::string download_license();
        std::string download_license_download(const std::string& download_license_id);
        std::string introspect();
        std::string token();
        std::string download_multiplay_license();
        std::string delete_download_licenses(const std::string& license_id);
    }
}
