#pragma once

#include "../picojson.h"

namespace vroid
{
    namespace models
    {
        class NullableAccessViolationException : public std::exception
        {
        };

        template <typename BaseType>
        class Nullable
        {
        public:
            Nullable()
                : is_null_(true)
            {
            }
            explicit Nullable(BaseType value)
                : value_(std::move(value)),
                  is_null_(false)
            {
            }

            template <typename ResponseType = BaseType>
            static Nullable<ResponseType> deserialize(const std::string& key, picojson::object& obj)
            {
                return obj[key].is<picojson::null>()
                           ? Nullable<ResponseType>()
                           : Nullable<ResponseType>(ResponseType::deserialize(obj[key].get<picojson::object>()));
            }
#if 0
            template <>
            static Nullable<std::string> deserialize<std::string>(const std::string& key, picojson::object& obj)
            {
                return obj[key].is<picojson::null>()
                           ? Nullable<std::string>()
                           : Nullable<std::string>(obj[key].get<std::string>());
            }

            template <>
            static Nullable<int32_t> deserialize<int32_t>(const std::string& key, picojson::object& obj)
            {
                return obj[key].is<picojson::null>()
                           ? Nullable<int32_t>()
                           : Nullable<int32_t>(static_cast<int32_t>(obj[key].get<double>()));
            }
#else
            template <>
            Nullable<std::string> deserialize<std::string>(const std::string& key, picojson::object& obj)
            {
                return obj[key].is<picojson::null>()
                           ? Nullable<std::string>()
                           : Nullable<std::string>(obj[key].get<std::string>());
            }

            template <>
            Nullable<int32_t> deserialize<int32_t>(const std::string& key, picojson::object& obj)
            {
                return obj[key].is<picojson::null>()
                           ? Nullable<int32_t>()
                           : Nullable<int32_t>(static_cast<int32_t>(obj[key].get<double>()));
            }
#endif

            BaseType get() const
            {
                if (is_null_)
                {
                    throw NullableAccessViolationException();
                }
                return value_;
            }

            void set(const BaseType& value)
            {
                _value(value);
                is_null_ = false;
            }

            bool is_null() const { return is_null_; }

            // override operator
            // if(nullable_x) {...}
            explicit operator bool() const
            {
                return !is_null();
            }

            // override operator
            // BaseType x = nullable_x;
            explicit operator BaseType() const
            {
                return get();
            }

            // override operator
            // nullable_x = x;
            Nullable<BaseType>& operator=(const BaseType& value)
            {
                set(value);
                return *this;
            }

        private:
            BaseType value_;
            bool is_null_;
        };
    }
}
