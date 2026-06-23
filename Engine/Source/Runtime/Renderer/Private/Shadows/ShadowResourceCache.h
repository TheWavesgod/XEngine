#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Math.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/Renderer/RendererSettings.h>

namespace XEngine
{
    class RHIDevice;
    class RHITexture;
    class RHISampler;

    struct DirectionalShadowResourceDesc 
    { 
        u32 Resolution = 2048; 
        u32 CascadeCount = 4; 
        
        RHIFormat DepthFormat = RHIFormat::D32Float; 
        ShadowMapStorageMode StorageMode = ShadowMapStorageMode::Texture2DArray; 
    };

    struct DirectionalShadowResources 
    {
        RHITexture* Texture = nullptr; 
        //RHITextureView* SampledView = nullptr; // TODO: need implement RHITexture

        //std::array<RHITextureView*, MaxShadowCascades> LayerDepthViews {}; 

        RHISampler* Sampler = nullptr; 
        u32 Resolution = 0; 
        u32 CascadeCount = 0; 
        RHIFormat Format = RHIFormat::Undefined;
    };

    /*
     * This class owns shadow texture resources.
     */
    class ShadowResourceCache 
    {
    public: 
        void Initialize(RHIDevice& device); 
        void Shutdown(RHIDevice& device); 

        DirectionalShadowResources& GetOrCreateDirectionalShadowResources( RHIDevice& device, const DirectionalShadowResourceDesc& desc); 

    private: 
        DirectionalShadowResources m_Directional;
    };
} // namespace XEngine
