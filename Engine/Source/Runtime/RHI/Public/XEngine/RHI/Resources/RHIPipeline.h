#pragma once

#include <XEngine/RHI/RHITypes.h>
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

        bool EnableDepthTest = true;
        bool EnableDepthWrite = true;

        RHIVertexBufferLayoutDesc VertexLayout;

        std::vector<RHIBindGroupLayout*> BindGroupLayouts;

        u32 PushConstantSize = 0;
        RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;

        const char* DebugName = nullptr;
    };

    class RHIPipeline
    {
    public:
        virtual ~RHIPipeline() = default;
    };
}
