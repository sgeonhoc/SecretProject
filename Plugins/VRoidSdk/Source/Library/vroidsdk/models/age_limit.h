#pragma once

namespace vroid
{
    namespace models
    {
        struct AgeLimit
        {
            bool is_r18;
            bool is_r15;
            bool is_adult;

            static AgeLimit deserialize(picojson::object& obj)
            {
                return AgeLimit{
                    obj["is_r15"].get<bool>(),
                    obj["is_r18"].get<bool>(),
                    obj["is_adult"].get<bool>()
                };
            }
        };
    }
}
