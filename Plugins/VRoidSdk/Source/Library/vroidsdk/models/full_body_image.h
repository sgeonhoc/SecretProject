#pragma once

#include "web_image.h"

namespace vroid
{
    namespace models
    {
        struct FullBodyImage
        {
            WebImage original;
            WebImage w600;
            WebImage w300;

            static FullBodyImage deserialize(picojson::object& obj)
            {
                return FullBodyImage{
                    WebImage::deserialize(obj["original"].get<picojson::object>()),
                    WebImage::deserialize(obj["w600"].get<picojson::object>()),
                    WebImage::deserialize(obj["w300"].get<picojson::object>()),
                };
            }
        };
    }
}
