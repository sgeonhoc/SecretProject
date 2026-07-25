#pragma once

#include "nullable.h"

namespace vroid
{
    namespace models
    {
        struct Tag
        {
            std::string name;
            Nullable<std::string> locale;
            Nullable<std::string> en_name;
            Nullable<std::string> ja_name;

            static Tag deserialize(picojson::object& obj)
            {
                Nullable<std::string> nullable;
                return Tag{
                    obj["name"].get<std::string>(),
                    nullable.deserialize("locale", obj),
                    nullable.deserialize("en_name", obj),
                    nullable.deserialize("ja_name", obj)
                };
            }
        };
    }
}
