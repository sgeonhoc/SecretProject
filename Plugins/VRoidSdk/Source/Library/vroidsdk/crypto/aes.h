#pragma once

#include <array>
#include <vector>
#include <string>

namespace vroid
{
    namespace crypto
    {
        constexpr int32_t KEY_SIZE = 256;
        constexpr int32_t SALT_SIZE = 16;
        constexpr int32_t IV_SIZE = 12;
        constexpr int32_t TAG_SIZE = 16;
        constexpr int32_t TOTAL_BLOCK_SIZE = SALT_SIZE + IV_SIZE + TAG_SIZE;
        using Key = std::array<uint8_t, KEY_SIZE / 8>;
        using Salt = std::array<uint8_t, SALT_SIZE>;
        using Iv = std::array<uint8_t, IV_SIZE>;
        using Tag = std::array<uint8_t, TAG_SIZE>;
        using Buffer = std::vector<uint8_t>;

        class Aes256Gcm final
        {
        public:
            explicit Aes256Gcm(std::string password);
            Buffer encrypt_binary(const Buffer& src) const;
            Buffer decrypt_binary(const Buffer& src) const;

        private:
            std::string password_;
        };
    }
}
