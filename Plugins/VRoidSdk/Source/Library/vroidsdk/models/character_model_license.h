#pragma once

#include "nullable.h"

namespace vroid
{
    namespace models
    {
        struct CharacterModelLicense
        {
            std::string modification;
            std::string redistribution;
            std::string credit;
            std::string characterization_allowed_user;
            std::string sexual_expression;
            std::string violent_expression;
            std::string corporate_commercial_use;
            std::string personal_commercial_use;

            static CharacterModelLicense deserialize(picojson::object& obj)
            {
                return CharacterModelLicense{
                    obj["modification"].get<std::string>(),
                    obj["redistribution"].get<std::string>(),
                    obj["credit"].get<std::string>(),
                    obj["characterization_allowed_user"].get<std::string>(),
                    obj["sexual_expression"].get<std::string>(),
                    obj["violent_expression"].get<std::string>(),
                    obj["corporate_commercial_use"].get<std::string>(),
                    obj["personal_commercial_use"].get<std::string>()
                };
            }
        };
    }
}
