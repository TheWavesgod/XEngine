#pragma once

#include <XEngine/Asset/AssetHandle.h>
#include <XEngine/Renderer/Material.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHISampler.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    class AssetSystem;
    class RHIDevice;
    class RenderTextureManager;
    struct MaterialAsset;

    struct MaterialRecord
    {
        std::string Name;
        MaterialDesc Desc;
        GPUMaterialData GPUData;
        std::shared_ptr<RHIBindGroup> BaseColorBindGroup;
        std::shared_ptr<RHIBindGroup> PBRBindGroup;
        u32 Generation = 0;
    };

    class RenderMaterialSystem
    {
    public:
        void Initialize(RenderTextureManager* textureManager, RHIDevice* device);
        void Shutdown();

        MaterialHandle CreateMaterial(const std::string& name, const MaterialDesc& desc);
        // Converts CPU-side Asset material data into a renderer material.
        // TextureAsset handles are resolved through AssetSystem and RenderTextureManager.
        MaterialHandle CreateMaterialFromAsset(
            const MaterialAsset& asset,
            AssetSystem& assetSystem,
            RenderTextureManager& textureManager);
        MaterialHandle GetOrCreateMaterialFromAsset(
            AssetHandle assetHandle,
            const MaterialAsset& asset,
            AssetSystem& assetSystem,
            RenderTextureManager& textureManager);

        const MaterialDesc* GetMaterialDesc(MaterialHandle handle) const;
        MaterialDesc* GetMaterialDesc(MaterialHandle handle);

        const GPUMaterialData* GetGPUMaterialData(MaterialHandle handle) const;
        RHIBindGroup* GetBaseColorBindGroup(MaterialHandle handle) const;
        RHIBindGroupLayout* GetBaseColorBindGroupLayout() const;
        RHIBindGroup* GetPBRMaterialBindGroup(MaterialHandle handle) const;
        RHIBindGroupLayout* GetPBRMaterialBindGroupLayout() const;

        MaterialHandle GetDefaultLitMaterial() const;
        MaterialHandle GetDefaultUnlitMaterial() const;
        MaterialHandle GetMissingMaterial() const;

        bool IsValid(MaterialHandle handle) const;

    private:
        MaterialHandle AddMaterialRecord(std::string name, const MaterialDesc& desc);
        MaterialDesc ResolveFallbackTextures(const MaterialDesc& desc) const;
        GPUMaterialData BuildGPUMaterialData(const MaterialDesc& desc) const;
        std::shared_ptr<RHIBindGroup> CreateBaseColorBindGroup(const MaterialDesc& desc) const;
        std::shared_ptr<RHIBindGroup> CreatePBRBindGroup(const MaterialDesc& desc) const;
        RHITexture* ResolveRHITexture(TextureHandle texture, TextureHandle fallback) const;
        TextureHandle ResolveTexture(TextureHandle texture, TextureHandle fallback) const;

        RHIDevice* m_Device = nullptr;
        RenderTextureManager* m_TextureManager = nullptr;
        std::shared_ptr<RHIBindGroupLayout> m_BaseColorBindGroupLayout;
        std::shared_ptr<RHIBindGroupLayout> m_PBRMaterialBindGroupLayout;
        std::shared_ptr<RHISampler> m_DefaultSampler;
        std::vector<MaterialRecord> m_Materials;
        std::unordered_map<u64, MaterialHandle> m_AssetMaterialCache;
        MaterialHandle m_DefaultLitMaterial;
        MaterialHandle m_DefaultUnlitMaterial;
        MaterialHandle m_MissingMaterial;
        bool m_Initialized = false;
    };
}
