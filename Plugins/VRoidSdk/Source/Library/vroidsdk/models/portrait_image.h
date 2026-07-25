#pragma once

#include "web_image.h"

namespace vroid
{
    namespace models
    {
        struct PortraitImage
        {
            WebImage original;
            WebImage w600;
            WebImage w300;
            WebImage sq600;
            WebImage sq300;
            WebImage sq150;

            static PortraitImage deserialize(picojson::object& obj)
            {
                return PortraitImage{
                    WebImage::deserialize(obj["original"].get<picojson::object>()),
                    WebImage::deserialize(obj["w600"].get<picojson::object>()),
                    WebImage::deserialize(obj["w300"].get<picojson::object>()),
                    WebImage::deserialize(obj["sq600"].get<picojson::object>()),
                    WebImage::deserialize(obj["sq300"].get<picojson::object>()),
                    WebImage::deserialize(obj["sq150"].get<picojson::object>())
                };
            }
        };
    }
}
