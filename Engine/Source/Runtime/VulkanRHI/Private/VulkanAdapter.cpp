// VulkanAdapter — implementation.

#include "VulkanAdapter.h"
#include "VulkanInstance.h"

#include <volk.h>
#include <cstring>
#include <vector>

namespace XEngine
{
    namespace
    {
        // Map a Khronos vendorID to a human-readable name. Vulkan exposes
        // only a numeric vendorID; the string table is the standard list.
        const char* VendorIDToName(u32 vendorID)
        {
            switch (vendorID)
            {
                case 0x10DE: return "NVIDIA";
                case 0x1002: return "AMD";
                case 0x8086: return "Intel";
                case 0x13B5: return "ARM";
                case 0x5143: return "Qualcomm";
                case 0x1010: return "Imagination";
                case 0x144D: return "Samsung";
                case 0x106B: return "Apple";
                default:     return "Unknown";
            }
        }
    }

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

        m_VendorName   = VendorIDToName(props.vendorID);
        m_DeviceName   = props.deviceName;
        m_DriverVersion = std::to_string(VK_VERSION_MAJOR(props.driverVersion)) + "." +
                          std::to_string(VK_VERSION_MINOR(props.driverVersion)) + "." +
                          std::to_string(VK_VERSION_PATCH(props.driverVersion));
        m_ApiVersion    = std::to_string(VK_API_VERSION_MAJOR(props.apiVersion)) + "." +
                          std::to_string(VK_API_VERSION_MINOR(props.apiVersion)) + "." +
                          std::to_string(VK_API_VERSION_PATCH(props.apiVersion));

        m_Info.VendorName        = m_VendorName;
        m_Info.AdapterName       = m_DeviceName;
        m_Info.DriverVersion     = m_DriverVersion;
        m_Info.APIInfo           = m_ApiVersion;
        m_Info.VendorID          = props.vendorID;
        m_Info.DeviceID          = props.deviceID;

        switch (props.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   m_Info.Type = RHIAdapterType::Discrete; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: m_Info.Type = RHIAdapterType::Integrated; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            m_Info.Type = RHIAdapterType::CPU; break;
            default:                                     m_Info.Type = RHIAdapterType::Unknown; break;
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

        DetectSupportedFeatures();
        CacheDeviceExtensions();
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

    bool VulkanAdapter::HasDeviceExtension(const char* name) const noexcept
    {
        if (name == nullptr)
        {
            return false;
        }
        for (const auto& ext : m_DeviceExtensions)
        {
            if (ext == name)
            {
                return true;
            }
        }
        return false;
    }

    // Phase 3 stub: full VkPhysicalDeviceFeatures2 + Vulkan11/12/13 chain
    // and PushDescriptor detection will land with Phase 3. Until then we
    // report None so negotiation in RHIInstance::CreateDevice will refuse
    // any RequiredFeatures and the Vulkan backend can be re-validated
    // without hiding missing-feature bugs behind unconditional `None`.
    void VulkanAdapter::DetectSupportedFeatures()
    {
        m_SupportedFeatures = RHIFeature::None;
    }

    void VulkanAdapter::CacheDeviceExtensions()
    {
        m_DeviceExtensions.clear();
        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            return;
        }

        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &count, nullptr);
        if (count == 0)
        {
            return;
        }

        std::vector<VkExtensionProperties> props(count);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &count, props.data());

        m_DeviceExtensions.reserve(count);
        for (const auto& p : props)
        {
            m_DeviceExtensions.emplace_back(p.extensionName);
        }
    }
}
