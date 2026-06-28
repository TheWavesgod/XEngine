#include "RenderMaterialSystem.h"

#include "RenderTextureManager.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>

#include <utility>

namespace XEngine
{
    namespace
    {
        constexpr u32 MaterialFlagUnlit = 1u << 0u;
        constexpr u32 MaterialFlagMasked = 1u << 1u;
        constexpr u32 MaterialFlagBlend = 1u << 2u;
        constexpr u32 MaterialFlagDoubleSided = 1u << 3u;

        u64 MakeAssetMaterialCacheKey(AssetHandle handle)
        {
            return (static_cast<u64>(handle.Generation) << 32u) | static_cast<u64>(handle.Index);
        }

        MaterialShadingModel ToRendererShadingModel(MaterialAssetShadingModel model)
        {
            switch (model)
            {
            case MaterialAssetShadingModel::Unlit:
                return MaterialShadingModel::Unlit;
            case MaterialAssetShadingModel::Lit:
            default:
                return MaterialShadingModel::Lit;
            }
        }

        MaterialAlphaMode ToRendererAlphaMode(MaterialAssetAlphaMode mode)
        {
            switch (mode)
            {
            case MaterialAssetAlphaMode::Masked:
                return MaterialAlphaMode::Masked;
            case MaterialAssetAlphaMode::Blend:
                return MaterialAlphaMode::Blend;
            case MaterialAssetAlphaMode::Opaque:
            default:
                return MaterialAlphaMode::Opaque;
            }
        }

        TextureHandle ResolveTextureAssetHandle(
            AssetHandle textureAssetHandle,
            AssetSystem& assetSystem,
            RenderTextureManager& textureManager,
            TextureHandle fallback,
            bool srgb)
        {
            if (textureAssetHandle.IsValid())
            {
                const TextureAsset* textureAsset = assetSystem.GetTextureAsset(textureAssetHandle);
                if (textureAsset != nullptr)
                {
                    TextureHandle texture = textureManager.GetOrCreateTextureFromAsset(
                        textureAssetHandle,
                        *textureAsset,
                        srgb);
                    if (texture.IsValid() && textureManager.GetTexture(texture) != nullptr)
                    {
                        return texture;
                    }
                }
            }

            return fallback;
        }
    }

    void RenderMaterialSystem::Initialize(RenderTextureManager* textureManager, RHIDevice* device)
    {
        if (m_Initialized)
        {
            return;
        }

        XENGINE_ASSERT(textureManager != nullptr, "RenderMaterialSystem requires RenderTextureManager");
        XENGINE_ASSERT(device != nullptr, "RenderMaterialSystem requires RHIDevice");
        if (textureManager == nullptr || device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderMaterialSystem requires RenderTextureManager and a valid RHIDevice");
            return;
        }

        m_TextureManager = textureManager;
        m_Device = device;
        RHIResourceFactory& factory = m_Device->GetResourceFactory();

        RHIBindGroupLayoutDesc layoutDesc;
        layoutDesc.DebugName = "Material base color bind group layout";
        // Stage 6D manual convention:
        // set 0, binding 0 = combined image sampler for base color in fragment shader.
        layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            0,
            RHIBindingType::CombinedImageSampler,
            RHIShaderStageFlags::Fragment,
            1
        });
        m_BaseColorBindGroupLayout = factory.CreateBindGroupLayout(layoutDesc);
        if (!m_BaseColorBindGroupLayout)
        {
            XENGINE_LOG_ERROR("Failed to create material base color bind group layout");
            return;
        }

        RHIBindGroupLayoutDesc pbrLayoutDesc;
        pbrLayoutDesc.DebugName = "PBR material bind group layout";
        pbrLayoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            0,
            RHIBindingType::CombinedImageSampler,
            RHIShaderStageFlags::Fragment,
            1
        });
        pbrLayoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            1,
            RHIBindingType::CombinedImageSampler,
            RHIShaderStageFlags::Fragment,
            1
        });
        pbrLayoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            2,
            RHIBindingType::CombinedImageSampler,
            RHIShaderStageFlags::Fragment,
            1
        });
        pbrLayoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
            3,
            RHIBindingType::CombinedImageSampler,
            RHIShaderStageFlags::Fragment,
            1
        });
        m_PBRMaterialBindGroupLayout = factory.CreateBindGroupLayout(pbrLayoutDesc);
        if (!m_PBRMaterialBindGroupLayout)
        {
            XENGINE_LOG_ERROR("Failed to create PBR material bind group layout");
            return;
        }

        RHISamplerDesc samplerDesc;
        samplerDesc.MinFilter = RHIFilter::Linear;
        samplerDesc.MagFilter = RHIFilter::Linear;
        samplerDesc.AddressU = RHIAddressMode::Repeat;
        samplerDesc.AddressV = RHIAddressMode::Repeat;
        samplerDesc.AddressW = RHIAddressMode::Repeat;
        samplerDesc.DebugName = "Material default linear repeat sampler";
        m_DefaultSampler = factory.CreateSampler(samplerDesc);
        if (!m_DefaultSampler)
        {
            XENGINE_LOG_ERROR("Failed to create material default sampler");
            return;
        }

        MaterialDesc defaultLit;
        defaultLit.ShadingModel = MaterialShadingModel::Lit;
        defaultLit.BaseColorTexture = m_TextureManager->GetDefaultWhiteTexture();
        defaultLit.NormalTexture = m_TextureManager->GetDefaultNormalTexture();
        defaultLit.MetallicRoughnessTexture = m_TextureManager->GetDefaultWhiteTexture();
        defaultLit.AOTexture = m_TextureManager->GetDefaultWhiteTexture();
        m_DefaultLitMaterial = AddMaterialRecord("DefaultLit", defaultLit);

        MaterialDesc defaultUnlit;
        defaultUnlit.ShadingModel = MaterialShadingModel::Unlit;
        defaultUnlit.BaseColorTexture = m_TextureManager->GetDefaultWhiteTexture();
        defaultUnlit.NormalTexture = m_TextureManager->GetDefaultNormalTexture();
        defaultUnlit.MetallicRoughnessTexture = m_TextureManager->GetDefaultWhiteTexture();
        defaultUnlit.AOTexture = m_TextureManager->GetDefaultWhiteTexture();
        m_DefaultUnlitMaterial = AddMaterialRecord("DefaultUnlit", defaultUnlit);

        MaterialDesc missing;
        missing.ShadingModel = MaterialShadingModel::Unlit;
        missing.BaseColorFactor = Vec4 { 1.0f, 0.0f, 1.0f, 1.0f };
        missing.BaseColorTexture = m_TextureManager->GetMissingTexture();
        missing.NormalTexture = m_TextureManager->GetDefaultNormalTexture();
        missing.MetallicRoughnessTexture = m_TextureManager->GetDefaultWhiteTexture();
        missing.AOTexture = m_TextureManager->GetDefaultWhiteTexture();
        m_MissingMaterial = AddMaterialRecord("MissingMaterial", missing);

        m_Initialized = true;
        XENGINE_LOG_INFO("RenderMaterialSystem initialized");
    }

    void RenderMaterialSystem::Shutdown()
    {
        if (!m_Initialized && m_Materials.empty())
        {
            return;
        }

        XENGINE_LOG_INFO("RenderMaterialSystem shutdown");
        m_AssetMaterialCache.clear();
        m_Materials.clear();
        m_DefaultSampler.reset();
        m_PBRMaterialBindGroupLayout.reset();
        m_BaseColorBindGroupLayout.reset();
        m_DefaultLitMaterial = {};
        m_DefaultUnlitMaterial = {};
        m_MissingMaterial = {};
        m_TextureManager = nullptr;
        m_Device = nullptr;
        m_Initialized = false;
    }

    MaterialHandle RenderMaterialSystem::CreateMaterial(const std::string& name, const MaterialDesc& desc)
    {
        if (!m_Initialized)
        {
            XENGINE_LOG_ERROR("Cannot create material before RenderMaterialSystem is initialized");
            return {};
        }

        return AddMaterialRecord(name, desc);
    }

    MaterialHandle RenderMaterialSystem::CreateMaterialFromAsset(
        const MaterialAsset& asset,
        AssetSystem& assetSystem,
        RenderTextureManager& textureManager)
    {
        if (!m_Initialized)
        {
            XENGINE_LOG_ERROR("Cannot create material from asset before RenderMaterialSystem is initialized");
            return {};
        }

        if (!asset.IsValid())
        {
            XENGINE_LOG_WARN("Cannot create renderer material from invalid MaterialAsset");
            return m_MissingMaterial;
        }

        MaterialDesc desc;
        desc.ShadingModel = ToRendererShadingModel(asset.ShadingModel);
        desc.AlphaMode = ToRendererAlphaMode(asset.AlphaMode);
        desc.BaseColorFactor = asset.BaseColorFactor;
        desc.MetallicFactor = asset.MetallicFactor;
        desc.RoughnessFactor = asset.RoughnessFactor;
        desc.AlphaCutoff = asset.AlphaCutoff;
        desc.Padding0 = asset.Padding0;
        desc.DoubleSided = asset.DoubleSided;

        desc.BaseColorTexture = ResolveTextureAssetHandle(
            asset.BaseColorTexture,
            assetSystem,
            textureManager,
            textureManager.GetDefaultWhiteTexture(),
            true);
        desc.NormalTexture = ResolveTextureAssetHandle(
            asset.NormalTexture,
            assetSystem,
            textureManager,
            textureManager.GetDefaultNormalTexture(),
            false);
        desc.MetallicRoughnessTexture = ResolveTextureAssetHandle(
            asset.MetallicRoughnessTexture,
            assetSystem,
            textureManager,
            textureManager.GetDefaultWhiteTexture(),
            false);
        desc.AOTexture = ResolveTextureAssetHandle(
            asset.AOTexture,
            assetSystem,
            textureManager,
            textureManager.GetDefaultWhiteTexture(),
            false);

        return CreateMaterial(asset.Name, desc);
    }

    MaterialHandle RenderMaterialSystem::GetOrCreateMaterialFromAsset(
        AssetHandle assetHandle,
        const MaterialAsset& asset,
        AssetSystem& assetSystem,
        RenderTextureManager& textureManager)
    {
        if (!assetHandle.IsValid())
        {
            return CreateMaterialFromAsset(asset, assetSystem, textureManager);
        }

        const u64 key = MakeAssetMaterialCacheKey(assetHandle);
        const auto cached = m_AssetMaterialCache.find(key);
        if (cached != m_AssetMaterialCache.end() && IsValid(cached->second))
        {
            return cached->second;
        }

        MaterialHandle handle = CreateMaterialFromAsset(asset, assetSystem, textureManager);
        if (IsValid(handle))
        {
            m_AssetMaterialCache[key] = handle;
        }

        return handle;
    }

    const MaterialDesc* RenderMaterialSystem::GetMaterialDesc(MaterialHandle handle) const
    {
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return &m_Materials[handle.Index].Desc;
    }

    MaterialDesc* RenderMaterialSystem::GetMaterialDesc(MaterialHandle handle)
    {
        return const_cast<MaterialDesc*>(static_cast<const RenderMaterialSystem*>(this)->GetMaterialDesc(handle));
    }

    const GPUMaterialData* RenderMaterialSystem::GetGPUMaterialData(MaterialHandle handle) const
    {
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return &m_Materials[handle.Index].GPUData;
    }

    RHIBindGroup* RenderMaterialSystem::GetBaseColorBindGroup(MaterialHandle handle) const
    {
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return m_Materials[handle.Index].BaseColorBindGroup.get();
    }

    RHIBindGroupLayout* RenderMaterialSystem::GetBaseColorBindGroupLayout() const
    {
        return m_BaseColorBindGroupLayout.get();
    }

    RHIBindGroup* RenderMaterialSystem::GetPBRMaterialBindGroup(MaterialHandle handle) const
    {
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return m_Materials[handle.Index].PBRBindGroup.get();
    }

    RHIBindGroupLayout* RenderMaterialSystem::GetPBRMaterialBindGroupLayout() const
    {
        return m_PBRMaterialBindGroupLayout.get();
    }

    MaterialHandle RenderMaterialSystem::GetDefaultLitMaterial() const
    {
        return m_DefaultLitMaterial;
    }

    MaterialHandle RenderMaterialSystem::GetDefaultUnlitMaterial() const
    {
        return m_DefaultUnlitMaterial;
    }

    MaterialHandle RenderMaterialSystem::GetMissingMaterial() const
    {
        return m_MissingMaterial;
    }

    bool RenderMaterialSystem::IsValid(MaterialHandle handle) const
    {
        if (!handle.IsValid() || handle.Index >= m_Materials.size())
        {
            return false;
        }

        return m_Materials[handle.Index].Generation == handle.Generation;
    }

    MaterialHandle RenderMaterialSystem::AddMaterialRecord(std::string name, const MaterialDesc& desc)
    {
        MaterialDesc resolvedDesc = ResolveFallbackTextures(desc);

        MaterialRecord record;
        record.Name = std::move(name);
        record.Desc = resolvedDesc;
        record.GPUData = BuildGPUMaterialData(resolvedDesc);
        record.BaseColorBindGroup = CreateBaseColorBindGroup(resolvedDesc);
        record.PBRBindGroup = CreatePBRBindGroup(resolvedDesc);
        record.Generation = 1;

        MaterialHandle handle;
        handle.Index = static_cast<u32>(m_Materials.size());
        handle.Generation = record.Generation;

        m_Materials.push_back(std::move(record));
        return handle;
    }

    MaterialDesc RenderMaterialSystem::ResolveFallbackTextures(const MaterialDesc& desc) const
    {
        MaterialDesc resolved = desc;
        if (m_TextureManager == nullptr)
        {
            return resolved;
        }

        resolved.BaseColorTexture = ResolveTexture(resolved.BaseColorTexture, m_TextureManager->GetDefaultWhiteTexture());
        resolved.NormalTexture = ResolveTexture(resolved.NormalTexture, m_TextureManager->GetDefaultNormalTexture());
        resolved.MetallicRoughnessTexture = ResolveTexture(
            resolved.MetallicRoughnessTexture,
            m_TextureManager->GetDefaultWhiteTexture());
        resolved.AOTexture = ResolveTexture(resolved.AOTexture, m_TextureManager->GetDefaultWhiteTexture());
        return resolved;
    }

    GPUMaterialData RenderMaterialSystem::BuildGPUMaterialData(const MaterialDesc& desc) const
    {
        GPUMaterialData gpu;
        gpu.BaseColorFactor = desc.BaseColorFactor;
        gpu.MetallicFactor = desc.MetallicFactor;
        gpu.RoughnessFactor = desc.RoughnessFactor;
        gpu.AlphaCutoff = desc.AlphaCutoff;

        // TODO Stage 11: replace TextureHandle.Index placeholders with
        // BindlessResourceManager texture indices.
        gpu.BaseColorTextureIndex = desc.BaseColorTexture.Index;
        gpu.NormalTextureIndex = desc.NormalTexture.Index;
        gpu.MetallicRoughnessTextureIndex = desc.MetallicRoughnessTexture.Index;
        gpu.AOTextureIndex = desc.AOTexture.Index;

        if (desc.ShadingModel == MaterialShadingModel::Unlit)
        {
            gpu.Flags |= MaterialFlagUnlit;
        }

        if (desc.AlphaMode == MaterialAlphaMode::Masked)
        {
            gpu.Flags |= MaterialFlagMasked;
        }
        else if (desc.AlphaMode == MaterialAlphaMode::Blend)
        {
            gpu.Flags |= MaterialFlagBlend;
        }

        if (desc.DoubleSided)
        {
            gpu.Flags |= MaterialFlagDoubleSided;
        }

        return gpu;
    }

    std::shared_ptr<RHIBindGroup> RenderMaterialSystem::CreateBaseColorBindGroup(const MaterialDesc& desc) const
    {
        if (m_Device == nullptr || m_TextureManager == nullptr ||
            !m_BaseColorBindGroupLayout || !m_DefaultSampler)
        {
            return nullptr;
        }

        RHITextureView* baseColorView = m_TextureManager->GetTextureView(desc.BaseColorTexture);
        if (baseColorView == nullptr)
        {
            baseColorView = m_TextureManager->GetTextureView(m_TextureManager->GetMissingTexture());
        }

        if (baseColorView == nullptr)
        {
            XENGINE_LOG_ERROR("Failed to resolve base color texture for material bind group");
            return nullptr;
        }

        RHIBindGroupDesc bindGroupDesc;
        bindGroupDesc.Layout = m_BaseColorBindGroupLayout.get();
        bindGroupDesc.DebugName = "Material base color bind group";
        bindGroupDesc.Resources.push_back(RHIBindingResource {
            0,
            RHIBindingType::CombinedImageSampler,
            baseColorView,
            m_DefaultSampler.get(),
            nullptr
        });

        // TODO Stage 10/11: move toward reflection-driven layouts and bindless texture indices.
        return m_Device->GetResourceFactory().CreateBindGroup(bindGroupDesc);
    }

    std::shared_ptr<RHIBindGroup> RenderMaterialSystem::CreatePBRBindGroup(const MaterialDesc& desc) const
    {
        if (m_Device == nullptr || m_TextureManager == nullptr ||
            !m_PBRMaterialBindGroupLayout || !m_DefaultSampler)
        {
            return nullptr;
        }

        RHITextureView* baseColorView = ResolveRHITextureView(desc.BaseColorTexture, m_TextureManager->GetDefaultWhiteTexture());
        RHITextureView* normalView = ResolveRHITextureView(desc.NormalTexture, m_TextureManager->GetDefaultNormalTexture());
        RHITextureView* metallicRoughnessView = ResolveRHITextureView(
            desc.MetallicRoughnessTexture,
            m_TextureManager->GetDefaultWhiteTexture());
        RHITextureView* aoView = ResolveRHITextureView(desc.AOTexture, m_TextureManager->GetDefaultWhiteTexture());

        if (baseColorView == nullptr || normalView == nullptr ||
            metallicRoughnessView == nullptr || aoView == nullptr)
        {
            XENGINE_LOG_ERROR("Failed to resolve one or more textures for PBR material bind group");
            return nullptr;
        }

        RHIBindGroupDesc bindGroupDesc;
        bindGroupDesc.Layout = m_PBRMaterialBindGroupLayout.get();
        bindGroupDesc.DebugName = "PBR material bind group";
        bindGroupDesc.Resources.push_back(RHIBindingResource {
            0,
            RHIBindingType::CombinedImageSampler,
            baseColorView,
            m_DefaultSampler.get(),
            nullptr
        });
        bindGroupDesc.Resources.push_back(RHIBindingResource {
            1,
            RHIBindingType::CombinedImageSampler,
            normalView,
            m_DefaultSampler.get(),
            nullptr
        });
        bindGroupDesc.Resources.push_back(RHIBindingResource {
            2,
            RHIBindingType::CombinedImageSampler,
            metallicRoughnessView,
            m_DefaultSampler.get(),
            nullptr
        });
        bindGroupDesc.Resources.push_back(RHIBindingResource {
            3,
            RHIBindingType::CombinedImageSampler,
            aoView,
            m_DefaultSampler.get(),
            nullptr
        });

        // TODO Stage 7:
        // glTF metallic-roughness channel convention should be handled carefully.
        // TODO Stage 11: replace per-material descriptors with bindless texture indices.
        return m_Device->GetResourceFactory().CreateBindGroup(bindGroupDesc);
    }

    RHITextureView* RenderMaterialSystem::ResolveRHITextureView(
        TextureHandle texture,
        TextureHandle fallback) const
    {
        if (m_TextureManager == nullptr)
        {
            return nullptr;
        }

        RHITextureView* resolvedView = m_TextureManager->GetTextureView(texture);
        if (resolvedView != nullptr)
        {
            return resolvedView;
        }

        return m_TextureManager->GetTextureView(fallback);
    }

    TextureHandle RenderMaterialSystem::ResolveTexture(TextureHandle texture, TextureHandle fallback) const
    {
        if (m_TextureManager == nullptr || m_TextureManager->GetTexture(texture) == nullptr)
        {
            return fallback;
        }

        return texture;
    }
}
