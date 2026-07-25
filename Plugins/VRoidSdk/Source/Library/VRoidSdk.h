#pragma once

#ifdef VROIDSDK_EXPORTS
#define VRoidHub_API __declspec(dllexport)
#else
#define VRoidHub_API __declspec(dllimport)
#endif

#include "vroidsdk/api.h"

extern "C"
{
    struct VRoidSdkInternal;

    VRoidHub_API VRoidSdkInternal* VRoidSdkConstructSimple();
    VRoidHub_API VRoidSdkInternal* VRoidSdkConstruct(const char* application_id, const char* secret_key,
                                                     const char* redirect_uri = "urn:ietf:wg:oauth:2.0:oob");
    VRoidHub_API void VRoidSdkDestruct(const VRoidSdkInternal*);
    VRoidHub_API bool VRoidSdkInitializeApi(VRoidSdkInternal* internal, const char* access_token);
    VRoidHub_API void VRoidSdkReadVrmBinary(const char* path, uint8_t* const binary);
    VRoidHub_API size_t VRoidSdkReadVrmBinarySize(const char* path);
    VRoidHub_API char* VRoidSdkGetUserName(const VRoidSdkInternal*);
    VRoidHub_API char* VRoidSdkGetUserId(const VRoidSdkInternal*);
#if 0
    VRoidHub_API void VRoidSdkGetEncrypted(const VRoidSdkInternal*, uint8_t* const encrypted);
    VRoidHub_API size_t VRoidSdkGetEncryptedLengthInBytes(const VRoidSdkInternal*);
    VRoidHub_API void VRoidSdkGetPlain(const VRoidSdkInternal*, uint8_t* const plain);
    VRoidHub_API size_t VRoidSdkGetPlainLengthInBytes(const VRoidSdkInternal*);
#endif
    VRoidHub_API char* VRoidSdkCreateAuthCode(const VRoidSdkInternal*, const bool is_multiplay);
    VRoidHub_API char* VRoidSdkCreateAuthUrl(const VRoidSdkInternal*, const char* auth_code);
    VRoidHub_API char* VRoidSdkCreateAuthRefreshUrl(const VRoidSdkInternal*, const char* refresh_token);

    VRoidHub_API bool VRoidSdkDownloadVRM(VRoidSdkInternal*, const char* id, const bool is_multiplay);
    //VRoidHub_API void VRoidSdkDownloadVRMSingle(VRoidSdkInternal*);
    VRoidHub_API char* VRoidSdkDownloadMultiplayLicenseId(const VRoidSdkInternal*, const char* model_id);
    VRoidHub_API void VRoidSdkSaveVRM(const VRoidSdkInternal*, const char* path, const bool is_multiplay);

    VRoidHub_API char* VRoidSdkCreateCharacterEndpoint();
    VRoidHub_API char* VRoidSdkCreateStaffpicksEndpoint();
    VRoidHub_API char* VRoidSdkCreateHeartsEndpoint();
    VRoidHub_API char* VRoidSdkCreateUserIconEndpoint(const VRoidSdkInternal*, const bool sq170);
    VRoidHub_API char* VRoidSdkCreateCharacterPropertyEndpoint(const char* model_id);
    VRoidHub_API void VRoidSdkFreeCharPointer(char*);
}

namespace vroid
{
    class VRoidSdk
    {
    public:
        VRoidSdk()
            : internal_(VRoidSdkConstructSimple())
        {
        }
        VRoidSdk(const std::string& application_id, const std::string& secret_key)
            : internal_(VRoidSdkConstruct(application_id.c_str(), secret_key.c_str()))
        {
        }

        virtual ~VRoidSdk()
        {
            VRoidSdkDestruct(internal_);
        }

        bool InitializeApi(const std::string& access_token) const
        {
            return VRoidSdkInitializeApi(internal_, access_token.c_str());
        }

        std::string GetUserName() const
        {
            const auto name(std::shared_ptr<char>(VRoidSdkGetUserName(internal_), VRoidSdkFreeCharPointer));
            return name.get();
        }

        std::string GetUserId() const
        {
            const auto id(std::shared_ptr<char>(VRoidSdkGetUserId(internal_), VRoidSdkFreeCharPointer));
            return id.get();
        }

        std::string CreateAuthCode(const bool is_multiplay) const
        {
            const auto auth_code(std::shared_ptr<char>(VRoidSdkCreateAuthCode(internal_, is_multiplay), VRoidSdkFreeCharPointer));
            return auth_code.get();
        }

        std::string CreateAuthUrl(const std::string& auth_code) const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateAuthUrl(internal_, auth_code.c_str()), VRoidSdkFreeCharPointer));
            return url.get();
        }

        std::string CreateAuthRefreshUrl(const std::string& refresh_token) const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateAuthRefreshUrl(internal_, refresh_token.c_str()), VRoidSdkFreeCharPointer));
            return url.get();
        }

        bool DownloadVRM(const std::string& id, const bool is_multiplay) const
        {
            return VRoidSdkDownloadVRM(internal_, id.c_str(), is_multiplay);
        }

        std::string DownloadMultiplayLicenseId(const std::string& model_id) const
        {
            const auto multiplay_license_id(std::shared_ptr<char>(VRoidSdkDownloadMultiplayLicenseId(internal_, model_id.c_str()), VRoidSdkFreeCharPointer));
            return multiplay_license_id.get();
        }

        void SaveVRM(const std::string& path, const bool is_multiplay) const
        {
            VRoidSdkSaveVRM(internal_, path.c_str(), is_multiplay);
        }

        std::string CreateCharacterEndpoint() const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateCharacterEndpoint(), VRoidSdkFreeCharPointer));
            return url.get();
        }

        std::string CreateStaffpicksEndpoint() const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateStaffpicksEndpoint(), VRoidSdkFreeCharPointer));
            return url.get();
        }

        std::string CreateHeartsEndpoint() const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateHeartsEndpoint(), VRoidSdkFreeCharPointer));
            return url.get();
        }

        std::string CreateUserIconEndpoint(const bool sq170 = true) const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateUserIconEndpoint(internal_, sq170), VRoidSdkFreeCharPointer));
            return url.get();
        }

        std::string CreateCharacterPropertyEndpoint(const std::string& model_id) const
        {
            const auto url(std::shared_ptr<char>(VRoidSdkCreateCharacterPropertyEndpoint(model_id.c_str()), VRoidSdkFreeCharPointer));
            return url.get();
        }
#if 0
        std::vector<uint8_t> GetEncrypted() const;
        std::vector<uint8_t> GetPlain() const;
        std::string GetDownloadModelId() const;
#endif
        std::vector<uint8_t> ReadVRMBinary(const std::string& path) const
        {
            const char* path_char(path.c_str());
            std::vector<uint8_t> binary(VRoidSdkReadVrmBinarySize(path_char));
            VRoidSdkReadVrmBinary(path_char, binary.data());
            return binary;
        }

    private:
        VRoidSdkInternal* internal_;
    };
}
