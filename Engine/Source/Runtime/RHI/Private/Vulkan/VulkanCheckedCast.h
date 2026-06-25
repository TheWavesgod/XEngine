#pragma once

#include <XEngine/Core/Assert.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class VulkanDevice;

    // Replaces dynamic_cast<VulkanX*>(rhiX*) in backend code.
    // Debug builds verify owner device matches and backend is Vulkan.
    // Release builds use static_cast.
    template <typename VulkanType, typename RHIType>
    VulkanType* CheckedVulkanCast(RHIType* resource, const VulkanDevice& expectedDevice)
    {
        XENGINE_ASSERT(resource != nullptr, "RHIResource is not valid");
        XENGINE_ASSERT(&resource->GetOwnerDevice() == &expectedDevice, "");
        XENGINE_ASSERT(resource->GetBackend() == RHIBackend::Vulkan, "");
        return static_cast<VulkanType*>(resource);
    }

    template <typename VulkanType, typename RHIType>
    const VulkanType* CheckedVulkanCast(const RHIType* resource, const VulkanDevice& expectedDevice)
    {
        XENGINE_ASSERT(resource != nullptr, "");
        XENGINE_ASSERT(&resource->GetOwnerDevice() == &expectedDevice, "");
        XENGINE_ASSERT(resource->GetBackend() == RHIBackend::Vulkan, "");
        return static_cast<const VulkanType*>(resource);
    }
}