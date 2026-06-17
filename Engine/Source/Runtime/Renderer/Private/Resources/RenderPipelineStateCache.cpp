#include "RenderPipelineStateCache.h"

#include "RenderFrameResources.h"
#include "RenderMaterialSystem.h"
#include "RenderShaderLibrary.h"
#include "RenderShaderTypes.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>

#include <cstddef>

namespace XEngine
{
    bool RenderPipelineStateCache::Initialize(
        RHIDevice* device,
        RenderShaderLibrary* shaderLibrary,
        RenderMaterialSystem* materialSystem,
        RenderFrameResources* frameResources)
    {
        if (device == nullptr || !device->IsValid() || shaderLibrary == nullptr ||
            materialSystem == nullptr || frameResources == nullptr)
        {
            XENGINE_LOG_ERROR("RenderPipelineStateCache requires device, shader library, material system, and frame resources");
            return false;
        }

        m_Device = device;
        m_ShaderLibrary = shaderLibrary;
        m_MaterialSystem = materialSystem;
        m_FrameResources = frameResources;
        XENGINE_LOG_INFO("RenderPipelineStateCache initialized");
        return true;
    }

    void RenderPipelineStateCache::Shutdown()
    {
        if (!m_GraphicsPipelines.empty() || m_Device != nullptr)
        {
            XENGINE_LOG_INFO("RenderPipelineStateCache shutdown");
        }
        m_GraphicsPipelines.clear();
        m_FrameResources = nullptr;
        m_MaterialSystem = nullptr;
        m_ShaderLibrary = nullptr;
        m_Device = nullptr;
    }

    RHIPipeline* RenderPipelineStateCache::GetOrCreateGraphicsPipeline(
        const GraphicsPipelineStateKey& key)
    {
        const auto cached = m_GraphicsPipelines.find(key);
        if (cached != m_GraphicsPipelines.end())
        {
            return cached->second.get();
        }

        std::shared_ptr<RHIPipeline> pipeline = CreateGraphicsPipeline(key);
        if (!pipeline)
        {
            return nullptr;
        }

        RHIPipeline* result = pipeline.get();
        m_GraphicsPipelines.emplace(key, std::move(pipeline));
        return result;
    }

    std::shared_ptr<RHIPipeline> RenderPipelineStateCache::CreateGraphicsPipeline(
        const GraphicsPipelineStateKey& key)
    {
        if (m_Device == nullptr || m_ShaderLibrary == nullptr || m_MaterialSystem == nullptr ||
            m_FrameResources == nullptr ||
            key.PassKind != RenderPassKind::ForwardOpaque)
        {
            return {};
        }

        RenderShaderKey vertexKey;
        vertexKey.Path = "shader://Passes/ForwardPBR.slang";
        vertexKey.EntryPoint = "vertexMain";
        vertexKey.Stage = ShaderStage::Vertex;
        vertexKey.Target = ShaderTarget::VulkanSPIRV;

        RenderShaderKey fragmentKey = vertexKey;
        fragmentKey.EntryPoint = "fragmentMain";
        fragmentKey.Stage = ShaderStage::Fragment;

        RHIShader* vertexShader = m_ShaderLibrary->GetOrCreateShader(vertexKey);
        RHIShader* fragmentShader = m_ShaderLibrary->GetOrCreateShader(fragmentKey);
        if (vertexShader == nullptr || fragmentShader == nullptr)
        {
            return {};
        }

        RHIGraphicsPipelineDesc desc;
        desc.VertexShader = vertexShader;
        desc.FragmentShader = fragmentShader;
        desc.ColorFormat = key.ColorFormat;
        desc.DepthFormat = key.DepthFormat;
        desc.EnableDepthTest = key.DepthTestEnabled;
        desc.EnableDepthWrite = key.DepthWriteEnabled;
        desc.VertexLayout.Stride = sizeof(MeshVertex);
        desc.VertexLayout.Attributes = {
            RHIVertexAttributeDesc { 0, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Position)) },
            RHIVertexAttributeDesc { 1, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Normal)) },
            RHIVertexAttributeDesc { 2, RHIFormat::R32G32Float, static_cast<u32>(offsetof(MeshVertex, TexCoord0)) }
        };
        // Forward pipeline layout convention:
        // Set 0 = per-frame data.
        // Set 1 = material data.
        // Set 2 = object data or push constants.
        desc.BindGroupLayouts.push_back(m_FrameResources->GetFrameBindGroupLayout());
        desc.BindGroupLayouts.push_back(m_MaterialSystem->GetPBRMaterialBindGroupLayout());
        desc.PushConstantSize = sizeof(PBRPushConstants);
        desc.PushConstantStages = RHIShaderStageFlags::AllGraphics;
        desc.DebugName = "Forward opaque graphics pipeline";

        std::shared_ptr<RHIPipeline> pipeline = m_Device->CreateGraphicsPipeline(desc);
        if (pipeline)
        {
            XENGINE_LOG_INFO("RenderPipelineStateCache cached ForwardOpaque pipeline");
        }
        else
        {
            XENGINE_LOG_ERROR("Failed to create ForwardOpaque graphics pipeline");
        }
        return pipeline;
    }
}
