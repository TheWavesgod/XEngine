#pragma once

#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <vector>

namespace XEngine
{
    class RHIBindGroupLayout;
    class RHIShader;

    struct RHIVertexAttributeDesc
    {
        u32 Location = 0;
        RHIFormat Format = RHIFormat::Undefined;
        u32 Offset = 0;
    };

    struct RHIVertexBufferLayoutDesc
    {
        u32 Stride = 0;
        std::vector<RHIVertexAttributeDesc> Attributes;
    };

    struct RHIGraphicsPipelineDesc
    {
        RHIShader* VertexShader = nullptr;
        RHIShader* FragmentShader = nullptr;

        RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
        RHIFormat DepthFormat = RHIFormat::D32Float;

        // True for color-writing pipelines. False for depth-only pipelines
        // (e.g. ShadowDepth). When false, ColorFormat is ignored and
        // the backend will skip color attachment setup.
        bool HasColorAttachment = true;

        bool EnableDepthTest = true;
        bool EnableDepthWrite = true;

        bool EnableDepthBias = false;
        f32 DepthBiasConstantFactor = 0.0f;
        f32 DepthBiasClamp = 0.0f;
        f32 DepthBiasSlopeFactor = 0.0f;

        RHIVertexBufferLayoutDesc VertexLayout;

        std::vector<RHIBindGroupLayout*> BindGroupLayouts;

        u32 PushConstantSize = 0;
        RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;

        const char* DebugName = nullptr;
    };

    class RHIPipeline : public RHIResource   
    {
    public:
        ~RHIPipeline() override = default;    

    protected:                                 
        explicit RHIPipeline(RHIDevice& ownerDevice);
    };
}
