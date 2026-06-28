#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <cstddef>

namespace XEngine
{
    class RHIPipeline;
    class RHIBuffer;
    class RHIBindGroup;
    class RHITextureView;

    class RHICommandList
    {
    public:
        virtual ~RHICommandList() = default;

        virtual void SetRenderOutput(const RHIRenderOutputDesc& output) = 0;
        virtual void SetGraphicsPipeline(RHIPipeline* pipeline) = 0;
        virtual void SetRenderViewport(const RHIRect2D& viewport) = 0;
        virtual void TransitionTextureToShaderRead(RHITextureView* textureView) = 0;
        virtual void SetBindGroup(u32 setIndex, RHIBindGroup* bindGroup) = 0;
        virtual void SetVertexBuffer(RHIBuffer* buffer, u64 offset = 0) = 0;
        virtual void SetIndexBuffer(RHIBuffer* buffer, RHIIndexFormat format, u64 offset = 0) = 0;
        virtual void PushConstants(
            RHIShaderStageFlags stages,
            const void* data,
            std::size_t size,
            std::size_t offset = 0) = 0;

        virtual void Draw(
            u32 vertexCount,
            u32 instanceCount,
            u32 firstVertex,
            u32 firstInstance) = 0;

        virtual void DrawIndexed(
            u32 indexCount,
            u32 instanceCount,
            u32 firstIndex,
            i32 vertexOffset,
            u32 firstInstance) = 0;
    };
}
