#pragma once

#include "vroidsdk/picojson.h"

namespace vroid
{
    namespace models
    {
        struct DownloadLicense
        {
            std::string id;
            std::string character_model_id;
            std::string character_model_version_id;
            std::string expires_at;

            static DownloadLicense deserialize(picojson::object& obj)
            {
                return DownloadLicense{
                    obj["id"].get<std::string>(),
                    obj["character_model_id"].get<std::string>(),
                    obj["character_model_version_id"].get<std::string>(),
                    obj["expires_at"].get<std::string>()
                };
            }

            bool is_valid() const
            {
                return !id.empty() && !character_model_id.empty() &&
                    !character_model_version_id.empty() && !expires_at.empty();
            }
        };
    }
}
