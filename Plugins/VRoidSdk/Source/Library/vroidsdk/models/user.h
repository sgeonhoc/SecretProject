#pragma once

#include "user_icon.h"

namespace vroid
{
    namespace models
    {
        struct User
        {
            std::string id;
            std::string name;
            std::string pixiv_user_id;
            UserIcon icon;

            static User deserialize(picojson::object& obj)
            {
                return User{
                    obj["id"].get<std::string>(),
                    obj["name"].get<std::string>(),
                    obj["pixiv_user_id"].get<std::string>(),
                    UserIcon::deserialize(obj["icon"].get<picojson::object>())
                };
            }
        };
    }
}
