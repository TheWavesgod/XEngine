#pragma once

#include <XEngine/Renderer/Material.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHISampler.h>

#include <memory>
#include <string>
#include <vector>

namespace XEngine
{
    class RHIDevice;
    class TextureManager;

    struct MaterialRecord
    {
        std::string Name;
        MaterialDesc Desc;
        GPUMaterialData GPUData;
        std::shared_ptr<RHIBindGroup> BaseColorBindGroup;
        u32 Generation = 0;
    };

    class MaterialSystem
    {
    public:
        void Initialize(TextureManager* textureManager, RHIDevice* device);
        void Shutdown();

        MaterialHandle CreateMaterial(const std::string& name, const MaterialDesc& desc);

        const MaterialDesc* GetMaterialDesc(MaterialHandle handle) const;
        MaterialDesc* GetMaterialDesc(MaterialHandle handle);

        const GPUMaterialData* GetGPUMaterialData(MaterialHandle handle) const;
        RHIBindGroup* GetBaseColorBindGroup(MaterialHandle handle) const;
        RHIBindGroupLayout* GetBaseColorBindGroupLayout() const;

        MaterialHandle GetDefaultLitMaterial() const;
        MaterialHandle GetDefaultUnlitMaterial() const;
        MaterialHandle GetMissingMaterial() const;

        bool IsValid(MaterialHandle handle) const;

    private:
        MaterialHandle AddMaterialRecord(std::string name, const MaterialDesc& desc);
        MaterialDesc ResolveFallbackTextures(const MaterialDesc& desc) const;
        GPUMaterialData BuildGPUMaterialData(const MaterialDesc& desc) const;
        std::shared_ptr<RHIBindGroup> CreateBaseColorBindGroup(const MaterialDesc& desc) const;
        TextureHandle ResolveTexture(TextureHandle texture, TextureHandle fallback) const;

        RHIDevice* m_Device = nullptr;
        TextureManager* m_TextureManager = nullptr;
        std::shared_ptr<RHIBindGroupLayout> m_BaseColorBindGroupLayout;
        std::shared_ptr<RHISampler> m_DefaultSampler;
        std::vector<MaterialRecord> m_Materials;
        MaterialHandle m_DefaultLitMaterial;
        MaterialHandle m_DefaultUnlitMaterial;
        MaterialHandle m_MissingMaterial;
        bool m_Initialized = false;
    };
}
