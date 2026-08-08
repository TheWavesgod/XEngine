// VulkanAdapter — concrete RHIAdapter for the Vulkan backend.
//
// Wraps a VkPhysicalDevice. Exposes static info (vendor, name, type, memory)
// and forwards device creation to the owning RHIInstance.

#pragma once

#include <XEngine/RHI/RHIAdapter.h>
#include <XEngine/RHI/RHIEnums.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace XEngine
{
    class VulkanInstance;

    class VulkanAdapter : public RHIAdapter
    {
    public:
        VulkanAdapter(VulkanInstance& instance, VkPhysicalDevice physicalDevice);
        ~VulkanAdapter() override;

        // RHIAdapter interface
        RHIAdapterInfo GetInfo() const override;
        RHIFeature GetSupportedFeatures() const noexcept override { return m_SupportedFeatures; }
        bool SupportsRequiredCapabilities(const RHICapabilities& required) const override;

        // Vulkan-specific accessors
        VkPhysicalDevice GetVkPhysicalDevice() const noexcept { return m_PhysicalDevice; }
        VulkanInstance& GetVulkanInstance() const noexcept { return m_Instance; }

        // Lookup of cached device-extension list (Phase 3).
        bool HasDeviceExtension(const char* name) const noexcept;

    private:
        void DetectSupportedFeatures();
        void CacheDeviceExtensions();

        VulkanInstance& m_Instance;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

        // Stable storage for std::string_view fields in m_Info.
        std::string m_VendorName;
        std::string m_DeviceName;
        std::string m_DriverVersion;
        std::string m_ApiVersion;
        RHIAdapterInfo m_Info;

        RHIFeature m_SupportedFeatures = RHIFeature::None;
        std::vector<std::string> m_DeviceExtensions;
    };
}
