#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIDevice;

    // Common base for every long-lived RHI object that is owned by an RHIDevice.
    class RHIResource
    {
    public:
        virtual ~RHIResource() = default;

        RHIDevice& GetOwnerDevice() const;
        RHIBackend GetBackend() const;

    protected:
        explicit RHIResource(RHIDevice& ownerDevice);

         // Helper for derived classes to verify backend correctness before static_cast.
        void XE_AssertBackendMatches(RHIBackend expected) const;

    private:
        RHIDevice* m_OwnerDevice = nullptr;
    };
}