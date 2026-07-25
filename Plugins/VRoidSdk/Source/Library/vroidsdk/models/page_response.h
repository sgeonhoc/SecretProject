#pragma once

#include "api_links_format.h"
#include "array.h"

namespace vroid
{
    namespace models
    {
        template<typename ResponseType>
        struct PageResponse
        {
            std::vector<ResponseType> data;
            ApiLinksFormat next;

            static PageResponse<ResponseType> deserialize(picojson::object& response)
            {
                auto links(response["_links"].get<picojson::object>());
                return PageResponse<ResponseType> {
                    Array<ResponseType>::deserialize(response["data"].get<picojson::array>()),
                    ApiLinksFormat::deserialize(links)
                };
            }
        };
    }
}
