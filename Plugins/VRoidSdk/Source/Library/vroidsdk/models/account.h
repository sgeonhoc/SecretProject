#pragma once

#include "user_detail.h"
#include "age_limit.h"

namespace vroid
{
    namespace models
    {
        struct Account
        {
            AgeLimit age_limit;
            std::string locale;
            UserDetail user_detail;

            static Account deserialize(picojson::object& obj)
            {
                return Account{
                    AgeLimit::deserialize(obj["age_limit"].get<picojson::object>()),
                    obj["locale"].get<std::string>(),
                    UserDetail::deserialize(obj["user_detail"].get<picojson::object>())
                };
            }
        };
    }
}
