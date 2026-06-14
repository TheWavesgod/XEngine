#include "RenderTextureManager.h"

#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>

#include <filesystem>
#include <string>
#include <utility>

namespace XEngine
{
    namespace
    {
        std::string NormalizeTexturePath(const std::string& path)
        {
            return std::filesystem::path(path).lexically_normal().generic_string();
        }

        u64 MakeAssetTextureCacheKey(AssetHandle handle)
        {
            return (static_cast<u64>(handle.Generation) << 32u) | static_cast<u64>(handle.Index);
        }
    }

    void RenderTextureManager::Initialize(RHIDevice* device)
    {
        if (m_Initialized)
        {
            return;
        }

        XENGINE_ASSERT(device != nullptr, "RenderTextureManager requires RHIDevice");
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderTextureManager requires a valid RHIDevice");
            return;
        }

        m_Device = device;
        XENGINE_LOG_INFO("RenderTextureManager initialized");

        m_DefaultWhiteTexture = CreateSolidColorTexture("DefaultWhiteTexture", 255, 255, 255, 255, false);
        XENGINE_LOG_INFO("Default white texture created");

        m_DefaultBlackTexture = CreateSolidColorTexture("DefaultBlackTexture", 0, 0, 0, 255, false);
        XENGINE_LOG_INFO("Default black texture created");

        m_DefaultNormalTexture = CreateSolidColorTexture("DefaultNormalTexture", 128, 128, 255, 255, false);
        XENGINE_LOG_INFO("Default normal texture created");

        m_MissingTexture = CreateSolidColorTexture("MissingTexture", 255, 0, 255, 255, false);
        XENGINE_LOG_INFO("Missing texture created");

        m_Initialized = true;
    }

    void RenderTextureManager::Shutdown()
    {
        if (!m_Initialized && m_Textures.empty())
        {
            return;
        }

        XENGINE_LOG_INFO("RenderTextureManager shutdown");
        m_AssetTextureCache.clear();
        m_PathCache.clear();
        m_Textures.clear();
        m_DefaultWhiteTexture = {};
        m_DefaultBlackTexture = {};
        m_DefaultNormalTexture = {};
        m_MissingTexture = {};
        m_Device = nullptr;
        m_Initialized = false;
    }

    TextureHandle RenderTextureManager::LoadTexture2D(const std::string& path, bool srgb)
    {
        (void)srgb;
        if (m_Device == nullptr || !m_Device->IsValid())
        {
            XENGINE_LOG_ERROR("Texture load failed because RenderTextureManager has no valid RHIDevice");
            return m_MissingTexture;
        }

        const std::string normalizedPath = NormalizeTexturePath(path);
        // TODO Stage 7C:
        // Deprecated. Renderer should not decode image files directly.
        // Use AssetSystem::ImportAsset and CreateTextureFromAsset instead.
        XENGINE_LOG_WARN(std::string("Deprecated renderer texture path requested: ") + normalizedPath);
        return m_MissingTexture;
    }

    TextureHandle RenderTextureManager::CreateTextureFromAsset(const TextureAsset& asset, bool srgb)
    {
        if (m_Device == nullptr || !m_Device->IsValid())
        {
            XENGINE_LOG_ERROR("Texture creation failed because RenderTextureManager has no valid RHIDevice");
            return m_MissingTexture;
        }

        if (!asset.IsValid() || asset.Format != TextureAssetFormat::RGBA8)
        {
            XENGINE_LOG_WARN("TextureAsset is invalid or has an unsupported format");
            return m_MissingTexture;
        }

        const std::string normalizedPath = NormalizeTexturePath(asset.SourcePath);
        const auto cached = m_PathCache.find(normalizedPath);
        if (cached != m_PathCache.end())
        {
            return cached->second;
        }

        RHITextureDesc desc;
        desc.Width = asset.Width;
        desc.Height = asset.Height;
        desc.MipLevels = 1;
        desc.ArrayLayers = 1;
        desc.Format = (srgb && asset.IsSRGB) ? RHIFormat::RGBA8Srgb : RHIFormat::RGBA8Unorm;
        desc.Dimension = RHITextureDimension::Texture2D;
        desc.Usage = RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;
        desc.GenerateMips = false;
        desc.DebugName = normalizedPath.c_str();

        std::shared_ptr<RHITexture> texture = m_Device->CreateTexture(desc, asset.Pixels.data(), asset.Pixels.size());
        if (!texture)
        {
            XENGINE_LOG_ERROR(std::string("Failed to create RHI texture: ") + normalizedPath);
            return m_MissingTexture;
        }

        XENGINE_LOG_INFO(std::string("Created RHI texture: ") + normalizedPath);
        TextureHandle handle = AddTextureRecord(normalizedPath, texture);
        m_PathCache.emplace(normalizedPath, handle);
        return handle;
    }

    TextureHandle RenderTextureManager::GetOrCreateTextureFromAsset(
        AssetHandle assetHandle,
        const TextureAsset& asset,
        bool srgb)
    {
        if (!assetHandle.IsValid())
        {
            return CreateTextureFromAsset(asset, srgb);
        }

        const u64 key = MakeAssetTextureCacheKey(assetHandle);
        const auto cached = m_AssetTextureCache.find(key);
        if (cached != m_AssetTextureCache.end() && GetTexture(cached->second) != nullptr)
        {
            return cached->second;
        }

        TextureHandle handle = CreateTextureFromAsset(asset, srgb);
        if (handle.IsValid() && GetTexture(handle) != nullptr)
        {
            m_AssetTextureCache[key] = handle;
        }

        return handle;
    }

    TextureHandle RenderTextureManager::CreateSolidColorTexture(
        const char* name,
        u8 r,
        u8 g,
        u8 b,
        u8 a,
        bool srgb)
    {
        if (m_Device == nullptr || !m_Device->IsValid())
        {
            XENGINE_LOG_ERROR("Cannot create solid color texture without a valid RHIDevice");
            return {};
        }

        const u8 pixel[] = { r, g, b, a };

        RHITextureDesc desc;
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArrayLayers = 1;
        desc.Format = srgb ? RHIFormat::RGBA8Srgb : RHIFormat::RGBA8Unorm;
        desc.Dimension = RHITextureDimension::Texture2D;
        desc.Usage = RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;
        desc.GenerateMips = false;
        desc.DebugName = name;

        std::shared_ptr<RHITexture> texture = m_Device->CreateTexture(desc, pixel, sizeof(pixel));
        if (!texture)
        {
            XENGINE_LOG_ERROR(std::string("Failed to create solid color texture: ") + (name != nullptr ? name : "<unnamed>"));
            return {};
        }

        return AddTextureRecord(name != nullptr ? name : "<unnamed>", texture);
    }

    RHITexture* RenderTextureManager::GetTexture(TextureHandle handle)
    {
        return const_cast<RHITexture*>(static_cast<const RenderTextureManager*>(this)->GetTexture(handle));
    }

    const RHITexture* RenderTextureManager::GetTexture(TextureHandle handle) const
    {
        if (!handle.IsValid() || handle.Index >= m_Textures.size())
        {
            return nullptr;
        }

        const TextureRecord& record = m_Textures[handle.Index];
        if (record.Generation != handle.Generation)
        {
            return nullptr;
        }

        return record.Texture.get();
    }

    TextureHandle RenderTextureManager::GetDefaultWhiteTexture() const
    {
        return m_DefaultWhiteTexture;
    }

    TextureHandle RenderTextureManager::GetDefaultBlackTexture() const
    {
        return m_DefaultBlackTexture;
    }

    TextureHandle RenderTextureManager::GetDefaultNormalTexture() const
    {
        return m_DefaultNormalTexture;
    }

    TextureHandle RenderTextureManager::GetMissingTexture() const
    {
        return m_MissingTexture;
    }

    TextureHandle RenderTextureManager::AddTextureRecord(std::string path, std::shared_ptr<RHITexture> texture)
    {
        if (!texture)
        {
            return {};
        }

        TextureRecord record;
        record.Path = std::move(path);
        record.Texture = std::move(texture);
        record.Generation = 1;

        TextureHandle handle;
        handle.Index = static_cast<u32>(m_Textures.size());
        handle.Generation = record.Generation;

        m_Textures.push_back(std::move(record));
        return handle;
    }
}
