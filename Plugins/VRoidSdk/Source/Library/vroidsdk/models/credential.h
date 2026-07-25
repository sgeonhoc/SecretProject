#pragma once

#include "nullable.h"
#include "array.h"

namespace vroid
{
    namespace models
    {
        struct Credential
        {
            std::string application_id; 
            std::string secret;
            std::vector<std::string> scope;
            std::string redirect_uri;
            std::string ios_url_scheme;
            std::string android_url_scheme;

            static Credential deserialize(picojson::object& obj)
            {
                return Credential {
                    obj["application_id"].get<std::string>(),
                    obj["secret"].get<std::string>(),
                    Array<std::string>::deserialize(obj["scope"].get<picojson::array>()),
                    obj["redirect_uri"].get<std::string>(),
                    obj["ios_url_scheme"].get<std::string>(),
                    obj["android_url_scheme"].get<std::string>(),
                };
            }
        };
    }
}
