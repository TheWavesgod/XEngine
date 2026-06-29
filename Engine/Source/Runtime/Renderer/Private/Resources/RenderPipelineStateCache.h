#pragma once

#include "GraphicsPipelineStateKey.h"

#include <memory>
#include <unordered_map>

namespace XEngine
{
    class RHIDevice;
    class RHIPipeline;
    class RenderMaterialSystem;
    class RenderFrameResources;
    class RenderShaderLibrary;

    class RenderPipelineStateCache
    {
    public:
        bool Initialize(
            RHIDevice* device,
            RenderShaderLibrary* shaderLibrary,
            RenderMaterialSystem* materialSystem,
            RenderFrameResources* frameResources);
        void Shutdown();

        RHIPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineStateKey& key);
        RHIPipeline* GetOrCreateShadowDepthPipeline(RHIFormat colorFormat, RHIFormat depthFormat);

    private:
        std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(const GraphicsPipelineStateKey& key);

        RHIDevice* m_Device = nullptr;
        RenderShaderLibrary* m_ShaderLibrary = nullptr;
        RenderMaterialSystem* m_MaterialSystem = nullptr;
        RenderFrameResources* m_FrameResources = nullptr;
        std::unordered_map<
            GraphicsPipelineStateKey,
            std::shared_ptr<RHIPipeline>,
            GraphicsPipelineStateKeyHash> m_GraphicsPipelines;
    };
}
