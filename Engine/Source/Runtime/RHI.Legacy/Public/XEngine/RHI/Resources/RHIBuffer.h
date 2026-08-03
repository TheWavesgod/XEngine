#pragma once

#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

#include <cstddef>

namespace XEngine
{
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
