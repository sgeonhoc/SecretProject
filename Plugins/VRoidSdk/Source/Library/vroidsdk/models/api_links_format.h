#pragma once

#include "nullable.h"

namespace vroid
{
    namespace models
    {
        struct ApiLinksFormat
        {
            Nullable<std::string> href;

            static ApiLinksFormat deserialize(picojson::object& obj)
            {
                Nullable<std::string> nullable;
                return ApiLinksFormat{
                    nullable.deserialize("href", obj)
                };
            }
        };
    }
}
