#pragma once

namespace vroid
{
    namespace models
    {
        struct DescriptionFragments
        {
            std::string body;
            std::string normalized_body;
            std::string type;

            static DescriptionFragments deserialize(picojson::object& obj)
            {
                return DescriptionFragments{
                    obj["body"].get<std::string>(),
                    obj["normalized_body"].get<std::string>(),
                    obj["type"].get<std::string>()
                };
            }
        };
    }
}
