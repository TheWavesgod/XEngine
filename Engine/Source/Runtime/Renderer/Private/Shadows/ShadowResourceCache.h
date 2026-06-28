#pragma once

#include "RenderShadowType.h"

#include <XEngine/Core/Types.h>
#include <XEngine/Math/Math.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/Renderer/RendererSettings.h>

#include <array>
#include <memory>

namespace XEngine
{
    class RHIDevice;
    class RHITexture;
    class RHITextureView;
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
        std::shared_ptr<RHITexture> Texture;
        std::shared_ptr<RHITextureView> SampledView;
        std::array<std::shared_ptr<RHITextureView>, MaxShadowCascades> LayerDepthViews {};
        std::shared_ptr<RHISampler> Sampler;
        u32 CascadeCount = 0; 
        u32 Resolution = 0; 
        RHIFormat Format = RHIFormat::Undefined;
    };

    /*
     * This class owns shadow texture resources.
     */
    class ShadowResourceCache 
    {
    public: 
        void Initialize(RHIDevice& device); 
        void Shutdown(); 

        DirectionalShadowResources& GetOrCreateDirectionalShadowResources( 
            const DirectionalShadowResourceDesc& desc); 

    private:
        RHIDevice* m_Device = nullptr;  
        DirectionalShadowResources m_Directional;
    };
} // namespace XEngine
