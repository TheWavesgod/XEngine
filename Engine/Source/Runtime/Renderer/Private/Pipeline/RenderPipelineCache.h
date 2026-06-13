#pragma once

#include <MaterialTypes.h>

namespace XEngine
{
    class RHIDevice;
    class RHIPipeline;

    struct PipelineKey 
    { 
        MaterialShadingModel ShadingModel; 
        MaterialAlphaMode AlphaMode; 
        
        bool DepthOnly = false; 

        bool operator==(const PipelineKey& other) const = default; 
    };

    class RenderPipelineCache
    {
    public:
        bool Initialize(RHIDevice* device);
        void Shutdown();

        RHIPipeline* GetOrCreateGraphicsPipeline(const PipelineKey& key);

    private:
        RHIDevice* m_Device = nullptr;
    }
}