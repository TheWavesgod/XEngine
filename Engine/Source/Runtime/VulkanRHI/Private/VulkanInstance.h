// VulkanInstance — concrete RHIInstance for the Vulkan backend.
//
// Owns the VkInstance and the volk function pointer loader for the
// instance. Exposes adapter enumeration and device creation.
//
// Phase 1 (M0-M3 backend): wraps VkInstance, enumerates VkPhysicalDevices,
// returns a VulkanAdapter per adapter. CreateDevice delegates to a future
// XEngineRHI-side factory (M11+), but Phase 1 returns a VulkanDevice directly.

#pragma once

#include <XEngine/RHI/RHIInstance.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class VulkanAdapter;
    class VulkanDevice;

    class VulkanInstance : public RHIInstance 
    {
    public:
        // Factory: creates a VkInstance via volkInitialize + vkCreateInstance.
        // Returns nullptr if Vulkan SDK is unavailable or instance creation fails.
        static std::unique_ptr<VulkanInstance> CreateInstance(const RHIInstanceDesc& desc);

        explicit VulkanInstance(VkInstance instance);
        ~VulkanInstance() override;

        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override;
        RHIDevice* CreateDevice(RHIAdapter& adapter, const RHIDeviceDesc& desc) override;

        VkInstance GetVkInstance() const noexcept { return m_Instance; }

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
    };
}
