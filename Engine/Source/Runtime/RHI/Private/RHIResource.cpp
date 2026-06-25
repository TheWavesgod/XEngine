#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/Core/Assert.h>

namespace XEngine
{
    RHIResource::RHIResource(RHIDevice& ownerDevice)
        : m_OwnerDevice(&ownerDevice)
    {
    }

    RHIDevice& RHIResource::GetOwnerDevice() const
    {
        XENGINE_ASSERT(m_OwnerDevice != nullptr, "RHIResource have no vliad owner device!");
        return *m_OwnerDevice;
    }

    RHIBackend RHIResource::GetBackend() const
    {
        return m_OwnerDevice->GetBackend();
    }

    void RHIResource::XE_AssertBackendMatches(RHIBackend expected) const
    {
        XENGINE_ASSERT(GetBackend() == expected, "Resource's backend is not expected");
    }
}