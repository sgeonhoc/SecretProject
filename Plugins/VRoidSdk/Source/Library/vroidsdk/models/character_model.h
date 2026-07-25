#pragma once

#include "nullable.h"
#include "age_limit.h"
#include "portrait_image.h"
#include "full_body_image.h"
#include "character_model_license.h"
#include "tag.h"
#include "character.h"
#include "character_model_version.h"
#include "array.h"

namespace vroid
{
    namespace models
    {
        struct CharacterModel
        {
            std::string id;
            Nullable<std::string> name;
            bool is_private;
            bool is_downloadable;
            bool is_other_users_available;
            bool is_hearted;
            PortraitImage portrait_image;
            FullBodyImage full_body_image;
            CharacterModelLicense license;
            std::string created_at;
            long heart_count;
            long download_count;
            long usage_count;
            long view_count;
            Nullable<std::string> published_at;
            std::vector<Tag> tags;
            AgeLimit age_limit;
            Character character;
            Nullable<CharacterModelVersion> latest_character_model_version;

            static CharacterModel deserialize(picojson::object& obj)
            {
                Nullable<std::string> nullable_string;
                return CharacterModel{
                    obj["id"].get<std::string>(),
                    nullable_string.deserialize("name", obj),
                    obj["is_private"].get<bool>(),
                    obj["is_downloadable"].get<bool>(),
                    obj["is_other_users_available"].get<bool>(),
                    obj["is_hearted"].get<bool>(),
                    PortraitImage::deserialize(obj["portrait_image"].get<picojson::object>()),
                    FullBodyImage::deserialize(obj["full_body_image"].get<picojson::object>()),
                    CharacterModelLicense::deserialize(obj["license"].get<picojson::object>()),
                    obj["created_at"].get<std::string>(),
                    static_cast<long>(obj["heart_count"].get<double>()),
                    static_cast<long>(obj["download_count"].get<double>()),
                    static_cast<long>(obj["usage_count"].get<double>()),
                    static_cast<long>(obj["view_count"].get<double>()),
                    nullable_string.deserialize("published_at", obj),
                    Array<Tag>::deserialize(obj["tags"].get<picojson::array>()),
                    AgeLimit::deserialize(obj["age_limit"].get<picojson::object>()),
                    Character::deserialize(obj["character"].get<picojson::object>()),
                    Nullable<CharacterModelVersion>::deserialize("latest_character_model_version", obj)
                };
            }
        };
    }
}
