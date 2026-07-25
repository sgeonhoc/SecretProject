#pragma once

#include <sstream>
#include <curl/curl.h>

namespace vroid
{
    namespace network
    {
        class CurlEasyInitException : public std::exception
        {
        };

        class CurlRequestFailedException : public std::exception
        {
        };

        class Curl final
        {
        public:
            static Curl create() noexcept(false);
            static void global_init();
            static void global_cleanup();

            // move constructor and operator
            Curl(Curl&& other) noexcept;
            Curl& operator=(Curl&& other) noexcept;

            ~Curl();

            // copy forbidden
            Curl(const Curl&) = delete;
            Curl& operator=(const Curl&) = delete;

            std::string perform() noexcept(false);
            void append_header(const std::string& key, const std::string& value);
            void append_post_fields(const std::string& key, const std::string& value);
            void set_url(const std::string& url) const;
            void is_follow_location(const bool enable) const;
            void set_accept_encoding(const std::string& enc) const;

        private:
            explicit Curl(CURL* const curl);
            CURL* curl_;
            curl_slist* header_;
            std::stringstream post_field_stream_;
        };
    }
}

