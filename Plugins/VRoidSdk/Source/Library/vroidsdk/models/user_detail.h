#pragma once

#include "user.h"
#include "description_fragments.h"
#include "array.h"

namespace vroid
{
    namespace models
    {
        struct UserDetail
        {
            std::string description;
            std::vector<DescriptionFragments> description_fragments;
            User user;

            static UserDetail deserialize(picojson::object& obj)
            {
                return UserDetail{
                    obj["description"].get<std::string>(),
                    Array<DescriptionFragments>::deserialize(obj["description_fragments"].get<picojson::array>()),
                    User::deserialize(obj["user"].get<picojson::object>())
                };
            }
        };
    }
}
