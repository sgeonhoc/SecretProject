#pragma once

#include "web_image.h"

namespace vroid
{
    namespace models
    {
        struct UserIcon
        {
            WebImage sq170;
            WebImage sq50;

            static UserIcon deserialize(picojson::object& obj)
            {
                return UserIcon{
                    WebImage::deserialize(obj["sq170"].get<picojson::object>()),
                    WebImage::deserialize(obj["sq50"].get<picojson::object>())
                };
            }
        };
    }
}
