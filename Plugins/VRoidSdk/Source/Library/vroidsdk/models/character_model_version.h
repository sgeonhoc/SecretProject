#pragma once

#include "nullable.h"

namespace vroid
{
    namespace models
    {
        struct CharacterModelVersion
        {
            std::string id;
            std::string created_at;
            Nullable<std::string> spec_version;
            Nullable<std::string> exporter_version;
            int32_t triangle_count;
            int32_t mesh_count;
            int32_t mesh_primitive_count;
            int32_t mesh_primitive_morph_count;
            int32_t material_count;
            int32_t texture_count;
            int32_t joint32_t32_t_count;
            bool is_vendor_forbidden_use_by_others;
            bool is_vendor_protected_download;
            Nullable<int32_t> original_file_size;
            Nullable<int32_t> original_compressed_file_size;

            static CharacterModelVersion deserialize(picojson::object& obj)
            {
                Nullable<std::string> nullable_string;
                Nullable<int32_t> nullable_int;
                return CharacterModelVersion{
                    obj["id"].get<std::string>(),
                    obj["created_at"].get<std::string>(),
                    nullable_string.deserialize("spec_version", obj),
                    nullable_string.deserialize("exporter_version", obj),
                    static_cast<int32_t>(obj["triangle_count"].get<double>()),
                    static_cast<int32_t>(obj["mesh_count"].get<double>()),
                    static_cast<int32_t>(obj["mesh_primitive_count"].get<double>()),
                    static_cast<int32_t>(obj["mesh_primitive_morph_count"].get<double>()),
                    static_cast<int32_t>(obj["material_count"].get<double>()),
                    static_cast<int32_t>(obj["texture_count"].get<double>()),
                    static_cast<int32_t>(obj["joint_count"].get<double>()),
                    obj["is_vendor_forbidden_use_by_others"].get<bool>(),
                    obj["is_vendor_protected_download"].get<bool>(),
                    nullable_int.deserialize("original_file_size", obj),
                    nullable_int.deserialize("original_compressed_file_size", obj)
                };
            }
        };
    }
}
