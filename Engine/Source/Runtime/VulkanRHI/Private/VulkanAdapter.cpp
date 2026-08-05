// VulkanAdapter — implementation.

#include "VulkanAdapter.h"
#include "VulkanInstance.h"

#include <volk.h>
#include <cstring>
#include <vector>

namespace XEngine
{
    VulkanAdapter::VulkanAdapter(VulkanInstance& instance, VkPhysicalDevice physicalDevice)
        : RHIAdapter(instance, RHIBackend::Vulkan)
        , m_Instance(instance)
        , m_PhysicalDevice(physicalDevice)
    {
        // Populate m_Info once with stable storage so the returned
        // std::string_view fields can outlive the call to vkGetPhysicalDeviceProperties.
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);

        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);

        m_VendorName  = props.deviceName;  // null-terminated char array
        m_DeviceName = props.deviceName;
        m_DriverVersion = props.driverVersion;
        m_ApiVersion   = props.apiVersion;

        m_Info.VendorName        = m_VendorName;
        m_Info.AdapterName       = m_DeviceName;
        m_Info.DriverVersion     = m_DriverVersion;
        m_Info.APIInfo           = "Vulkan " + std::to_string(VK_API_VERSION_MAJOR(props.apiVersion)) + "." +
                                    std::to_string(VK_API_VERSION_MINOR(props.apiVersion)) + "." +
                                    std::to_string(VK_API_VERSION_PATCH(props.apiVersion));
        m_Info.VendorID           = props.vendorID;
        m_Info.DeviceID           = props.deviceID;

        switch (props.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   m_Info.Type = RHIAdapterType::Discrete; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: m_Info.Type = RHIAdapterType::Integrated; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:           m_Info.Type = RHIAdapterType::CPU; break;
            default:                                    m_Info.Type = RHIAdapterType::Unknown; break;
        }

        // Aggregate heap memory across all memory heaps.
        u64 totalMem = 0;
        for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i)
        {
            if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                totalMem += memProps.memoryHeaps[i].size;
            }
        }
        m_Info.DedicatedMemoryBytes = totalMem;
        // System/shared memory is hard to attribute; leave at 0 for Phase 1.
        m_Info.SharedMemoryBytes = 0;
    }

    VulkanAdapter::~VulkanAdapter() = default;

    RHIAdapterInfo VulkanAdapter::GetInfo() const
    {
        return m_Info;
    }

    bool VulkanAdapter::SupportsRequiredCapabilities(const RHICapabilities& required) const
    {
        // Phase 1: accept everything caller asks for. Real cap matching
        // is implemented in Phase 2 (M3 backend hardening).
        (void)required;
        return true;
    }
}
