#pragma once

#include "nullable.h"

namespace vroid
{
    namespace models
    {
        struct WebImage
        {
            int32_t width;
            int32_t height;
            std::string url;
            Nullable<std::string> url2x;

            static WebImage deserialize(picojson::object& obj)
            {
                Nullable<std::string> nullable;
                return vroid::models::WebImage{
                    static_cast<int32_t>(obj["width"].get<double>()),
                    static_cast<int32_t>(obj["height"].get<double>()),
                    obj["url"].get<std::string>(),
                    nullable.deserialize("url2x", obj),
                };
            }
        };
    }
}
