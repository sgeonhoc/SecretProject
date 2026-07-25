#pragma once

namespace vroid
{
    namespace models
    {
        template <typename BaseType>
        class Array
        {
        public:
            template <typename ResponseType = BaseType>
            static std::vector<ResponseType> deserialize(picojson::array& array)
            {
                std::vector<ResponseType> data;
                data.reserve(array.size());
                for (const auto& it : array)
                {
                    auto object(it.get<picojson::object>());
                    data.emplace_back(ResponseType::deserialize(object));
                }
                return data;
            }
        };
    }
}
