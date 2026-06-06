#include "TextureManager.h"

#include "ImageLoader.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>

#include <filesystem>
#include <string>

namespace XEngine
{
    namespace
    {
        std::string NormalizeTexturePath(const std::string& path)
        {
            return std::filesystem::path(path).lexically_normal().generic_string();
        }
    }

    void TextureManager::Initialize(RHIDevice* device)
    {
        if (m_Initialized)
        {
            return;
        }

        XENGINE_ASSERT(device != nullptr, "TextureManager requires RHIDevice");
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("TextureManager requires a valid RHIDevice");
            return;
        }

        m_Device = device;
        XENGINE_LOG_INFO("TextureManager initialized");

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

    void TextureManager::Shutdown()
    {
        if (!m_Initialized && m_Textures.empty())
        {
            return;
        }

        XENGINE_LOG_INFO("TextureManager shutdown");
        m_PathCache.clear();
        m_Textures.clear();
        m_DefaultWhiteTexture = {};
        m_DefaultBlackTexture = {};
        m_DefaultNormalTexture = {};
        m_MissingTexture = {};
        m_Device = nullptr;
        m_Initialized = false;
    }

    TextureHandle TextureManager::LoadTexture2D(const std::string& path, bool srgb)
    {
        if (m_Device == nullptr || !m_Device->IsValid())
        {
            XENGINE_LOG_ERROR("Texture load failed because TextureManager has no valid RHIDevice");
            return m_MissingTexture;
        }

        const std::string normalizedPath = NormalizeTexturePath(path);
        const auto cached = m_PathCache.find(normalizedPath);
        if (cached != m_PathCache.end())
        {
            return cached->second;
        }

        XENGINE_LOG_INFO(std::string("Loading texture: ") + normalizedPath);
        ImageData image = ImageLoader::LoadRGBA8(normalizedPath);
        if (!image.IsValid())
        {
            XENGINE_LOG_WARN(std::string("Texture load failed: ") + normalizedPath);
            return m_MissingTexture;
        }

        RHITextureDesc desc;
        desc.Width = image.Width;
        desc.Height = image.Height;
        desc.MipLevels = 1;
        desc.ArrayLayers = 1;
        desc.Format = srgb ? RHIFormat::RGBA8Srgb : RHIFormat::RGBA8Unorm;
        desc.Dimension = RHITextureDimension::Texture2D;
        desc.Usage = RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;
        desc.GenerateMips = false;
        desc.DebugName = nullptr;

        std::shared_ptr<RHITexture> texture = m_Device->CreateTexture(desc, image.Pixels.data(), image.Pixels.size());
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

    TextureHandle TextureManager::CreateSolidColorTexture(
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

    RHITexture* TextureManager::GetTexture(TextureHandle handle)
    {
        return const_cast<RHITexture*>(static_cast<const TextureManager*>(this)->GetTexture(handle));
    }

    const RHITexture* TextureManager::GetTexture(TextureHandle handle) const
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

    TextureHandle TextureManager::GetDefaultWhiteTexture() const
    {
        return m_DefaultWhiteTexture;
    }

    TextureHandle TextureManager::GetDefaultBlackTexture() const
    {
        return m_DefaultBlackTexture;
    }

    TextureHandle TextureManager::GetDefaultNormalTexture() const
    {
        return m_DefaultNormalTexture;
    }

    TextureHandle TextureManager::GetMissingTexture() const
    {
        return m_MissingTexture;
    }

    TextureHandle TextureManager::AddTextureRecord(std::string path, std::shared_ptr<RHITexture> texture)
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
