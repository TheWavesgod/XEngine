#include "RenderPipelineStateCache.h"

#include "RenderFrameResources.h"
#include "RenderMaterialSystem.h"
#include "RenderShaderLibrary.h"
#include "RenderShaderTypes.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIResourceFactory.h>
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
            m_FrameResources == nullptr)
        {
            return {};
        }

        if (key.PassKind != RenderPassKind::ForwardOpaque &&
            key.PassKind != RenderPassKind::ShadowDepth)
        {
            return {};
        }

        RHIShader* vertexShader = nullptr;
        RHIShader* fragmentShader = nullptr;
        const char* debugName = nullptr;
        std::vector<RHIBindGroupLayout*> bindGroupLayouts;
        u32 pushConstantSize = 0;

        if (key.PassKind == RenderPassKind::ShadowDepth)
        {
            // Depth-only cascade shadow pass.
            // Uses DepthOnly.slang (vertex + ForceInline fragment that emits SV_Depth).
            RenderShaderKey vertexKey;
            vertexKey.Path = "shader://Passes/DepthOnly.slang";
            vertexKey.EntryPoint = "vertexMain";
            vertexKey.Stage = ShaderStage::Vertex;
            vertexKey.Target = ShaderTarget::VulkanSPIRV;

            RenderShaderKey fragmentKey = vertexKey;
            fragmentKey.EntryPoint = "fragmentMain";
            fragmentKey.Stage = ShaderStage::Fragment;

            vertexShader = m_ShaderLibrary->GetOrCreateShader(vertexKey);
            fragmentShader = m_ShaderLibrary->GetOrCreateShader(fragmentKey);
            if (vertexShader == nullptr || fragmentShader == nullptr)
            {
                return {};
            }

            bindGroupLayouts = {}; // ShadowDepth owns no Set 0 binding; cascade LVP arrives via push constants.
            pushConstantSize = sizeof(ShadowDepthPushConstants);
            debugName = "ShadowDepth graphics pipeline";
        }
        else // RenderPassKind::ForwardOpaque
        {
            RenderShaderKey vertexKey;
            vertexKey.Path = "shader://Passes/ForwardPBR.slang";
            vertexKey.EntryPoint = "vertexMain";
            vertexKey.Stage = ShaderStage::Vertex;
            vertexKey.Target = ShaderTarget::VulkanSPIRV;

            RenderShaderKey fragmentKey = vertexKey;
            fragmentKey.EntryPoint = "fragmentMain";
            fragmentKey.Stage = ShaderStage::Fragment;

            vertexShader = m_ShaderLibrary->GetOrCreateShader(vertexKey);
            fragmentShader = m_ShaderLibrary->GetOrCreateShader(fragmentKey);
            if (vertexShader == nullptr || fragmentShader == nullptr)
            {
                return {};
            }

            bindGroupLayouts.push_back(m_FrameResources->GetFrameBindGroupLayout());
            bindGroupLayouts.push_back(m_MaterialSystem->GetPBRMaterialBindGroupLayout());
            pushConstantSize = sizeof(PBRPushConstants);
            debugName = "Forward opaque graphics pipeline";
        }

        RHIGraphicsPipelineDesc desc;
        desc.VertexShader = vertexShader;
        desc.FragmentShader = fragmentShader;
        desc.ColorFormat = key.ColorFormat;
        desc.DepthFormat = key.DepthFormat;
        desc.HasColorAttachment = key.HasColorAttachment;
        desc.EnableDepthTest = key.DepthTestEnabled;
        desc.EnableDepthWrite = key.DepthWriteEnabled;
        desc.EnableDepthBias = key.EnableDepthBias;
        desc.DepthBiasConstantFactor = key.DepthBiasConstantFactor;
        desc.DepthBiasClamp = key.DepthBiasClamp;
        desc.DepthBiasSlopeFactor = key.DepthBiasSlopeFactor;
        desc.VertexLayout.Stride = sizeof(MeshVertex);
        desc.VertexLayout.Attributes = {
            RHIVertexAttributeDesc { 0, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Position)) },
            RHIVertexAttributeDesc { 1, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Normal)) },
            RHIVertexAttributeDesc { 2, RHIFormat::R32G32Float, static_cast<u32>(offsetof(MeshVertex, TexCoord0)) }
        };
        desc.BindGroupLayouts = bindGroupLayouts;
        desc.PushConstantSize = pushConstantSize;
        desc.PushConstantStages = RHIShaderStageFlags::AllGraphics;
        desc.DebugName = debugName;

        std::shared_ptr<RHIPipeline> pipeline =
            m_Device->GetResourceFactory().CreateGraphicsPipeline(desc);
        if (pipeline)
        {
            XENGINE_LOG_INFO("RenderPipelineStateCache cached pipeline");
        }
        else
        {
            XENGINE_LOG_ERROR("Failed to create graphics pipeline");
        }
        return pipeline;
    }

    RHIPipeline* RenderPipelineStateCache::GetOrCreateShadowDepthPipeline(
    RHIFormat colorFormat, RHIFormat depthFormat)
    {
        GraphicsPipelineStateKey key;
        key.PassKind         = RenderPassKind::ShadowDepth;
        key.ShadingModel     = MaterialShadingModel::Unlit;     // depth-only is unlit
        key.AlphaMode        = MaterialAlphaMode::Opaque;
        key.VertexLayout     = VertexLayoutKind::MeshVertex;
        key.ColorFormat      = colorFormat;     // typically Undefined
        key.DepthFormat      = depthFormat;     // typically D32Float
        key.DepthTestEnabled = true;
        key.DepthWriteEnabled = true;
        key.BlendEnabled     = false;
        key.DoubleSided      = false;
        key.HasColorAttachment = false;        // depth-only
        return GetOrCreateGraphicsPipeline(key);
    }
}
