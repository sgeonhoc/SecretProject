#pragma once

#include "nullable.h"
#include "user.h"

namespace vroid
{
    namespace models
    {
        struct Character
        {
            std::string id;
            std::string name;
            bool is_private;
            std::string created_at;
            Nullable<std::string> published_at;
            User user;

            static Character deserialize(picojson::object& obj)
            {
                Nullable<std::string> nullable;
                return Character{
                    obj["id"].get<std::string>(),
                    obj["name"].get<std::string>(),
                    obj["is_private"].get<bool>(),
                    obj["created_at"].get<std::string>(),
                    nullable.deserialize("published_at", obj),
                    User::deserialize(obj["user"].get<picojson::object>())
                };
            }
        };
    }
}
