#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIResource.h>

#include <cstddef>

namespace XEngine
{
    struct RHIBufferDesc
    {
        std::size_t Size = 0;
        RHIBufferUsage Usage = RHIBufferUsage::None;
        RHIMemoryUsage MemoryUsage = RHIMemoryUsage::GPUOnly;
        const char* DebugName = nullptr;
    };

    class RHIBuffer : public RHIResource
    {
    public:
        ~RHIBuffer() override = default;

        virtual std::size_t GetSize() const = 0;
        virtual bool Update(const void* data, std::size_t size, std::size_t offset = 0) = 0;

    protected:
        explicit RHIBuffer(RHIDevice& ownerDevice);
    };
}
