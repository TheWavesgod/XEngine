#pragma once

#include <XEngine/Core/Types.h>

#include <cstddef>

namespace XEngine
{
    enum class RHIBufferUsage : u32
    {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5
    };

    inline RHIBufferUsage operator|(RHIBufferUsage lhs, RHIBufferUsage rhs)
    {
        return static_cast<RHIBufferUsage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
    }

    inline bool HasFlag(RHIBufferUsage value, RHIBufferUsage flag)
    {
        return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
    }

    enum class RHIMemoryUsage
    {
        GPUOnly,
        CPUToGPU,
        GPUToCPU
    };

    struct RHIBufferDesc
    {
        std::size_t Size = 0;
        RHIBufferUsage Usage = RHIBufferUsage::None;
        RHIMemoryUsage MemoryUsage = RHIMemoryUsage::GPUOnly;
        const char* DebugName = nullptr;
    };

    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;

        virtual std::size_t GetSize() const = 0;
    };
}
