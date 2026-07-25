#pragma once

#include <vector>
#include <string>

namespace vroid
{
    namespace network
    {
        enum EVerb : uint8_t
        {
            Post,
            Get,
            GetBinary,
        };

        class HttpRequest final
        {
        public:
            struct Param
            {
                std::string key;
                std::string value;
            };

            struct Header
            {
                std::string key;
                std::string value;
            };

            HttpRequest(const HttpRequest&) = delete;
            ~HttpRequest();
            HttpRequest& operator=(const HttpRequest&) = delete;
            static std::string post(const std::string& url, const std::vector<Param>& params = {},
                                    const std::vector<Header>& headers = {}) noexcept(false);
            static std::string get(const std::string& url, const std::vector<Param>& params = {},
                                   const std::vector<Header>& headers = {}) noexcept(false);
            static std::string get_binary(const std::string& url, const std::vector<Param>& params = {},
                                          const std::vector<Header>& headers = {}) noexcept(false);

        private:
            HttpRequest();
            static std::string request(const std::string& url, const std::vector<Param>& params,
                                       const std::vector<Header>& headers,
                                       const EVerb verb) noexcept(false);
        };
    }
}
