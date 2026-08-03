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
        XENGINE_ASSERT(m_OwnerDevice != nullptr, "RHIResource has no valid owner device");
        return *m_OwnerDevice;
    }

    RHIBackend RHIResource::GetBackend() const
    {
        return m_OwnerDevice->GetBackend();
    }
}