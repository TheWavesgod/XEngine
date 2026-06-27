#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Core/Types.h>
#include <XEngine/Renderer/Texture.h>
#include <XEngine/RHI/Resources/RHITexture.h>
#include <XEngine/RHI/Resources/RHITextureView.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    class RHIDevice;
    class RHITexture;
    struct TextureAsset;

    struct TextureRecord
    {
        std::string Path;
        std::shared_ptr<RHITexture> Texture;
        std::shared_ptr<RHITextureView> SampledView;
        u32 Generation = 0;
    };

    class RenderTextureManager
    {
    public:
        void Initialize(RHIDevice* device);
        void Shutdown();

        TextureHandle LoadTexture2D(const std::string& path, bool srgb = true);
        TextureHandle CreateTextureFromAsset(const TextureAsset& asset, bool srgb = true);
        TextureHandle GetOrCreateTextureFromAsset(
            AssetHandle assetHandle,
            const TextureAsset& asset,
            bool srgb = true);

        TextureHandle CreateSolidColorTexture(
            const char* name,
            u8 r,
            u8 g,
            u8 b,
            u8 a,
            bool srgb = false);

        RHITexture* GetTexture(TextureHandle handle);
        const RHITexture* GetTexture(TextureHandle handle) const;

        TextureHandle GetDefaultWhiteTexture() const;
        TextureHandle GetDefaultBlackTexture() const;
        TextureHandle GetDefaultNormalTexture() const;
        TextureHandle GetMissingTexture() const;

    private:
        TextureHandle AddTextureRecord(std::string path, std::shared_ptr<RHITexture> texture);

        RHIDevice* m_Device = nullptr;
        std::vector<TextureRecord> m_Textures;
        std::unordered_map<std::string, TextureHandle> m_PathCache;
        std::unordered_map<u64, TextureHandle> m_AssetTextureCache;
        TextureHandle m_DefaultWhiteTexture;
        TextureHandle m_DefaultBlackTexture;
        TextureHandle m_DefaultNormalTexture;
        TextureHandle m_MissingTexture;
        bool m_Initialized = false;
    };
}
