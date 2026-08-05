// VulkanInstance — implementation.

#include "VulkanInstance.h"
#include "VulkanAdapter.h"
#include "VulkanDevice.h"

#include <volk.h>
#include <cstring>

namespace XEngine
{
    namespace VulkanRHI
    {
        std::unique_ptr<RHIInstance> CreateInstance(const RHIInstanceDesc& desc)
        {
            return VulkanInstance::CreateInstance(desc);
        }
    }

    std::unique_ptr<VulkanInstance> VulkanInstance::CreateInstance(const RHIInstanceDesc& desc)
    {
        // volkInitialize is safe to call multiple times (it's a no-op after
        // the first successful call). We do it unconditionally so the
        // static library can be linked into apps without external setup.
        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = desc.ApplicationName.data();
        appInfo.applicationVersion = desc.ApplicationVersion;
        appInfo.pEngineName = "XEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkInstance instance = VK_NULL_HANDLE;
        result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }

        volkLoadInstance(instance);

        return std::make_unique<VulkanInstance>(instance);
    }

    VulkanInstance::VulkanInstance(VkInstance instance)
        : RHIInstance(RHIInstanceDesc{}, RHIBackend::Vulkan)
        , m_Instance(instance)
    {
    }

    VulkanInstance::~VulkanInstance()
    {
        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }
    }

    std::vector<std::unique_ptr<RHIAdapter>> VulkanInstance::EnumerateAdapters()
    {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
        if (count == 0)
        {
            return {};
        }

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

        std::vector<std::unique_ptr<RHIAdapter>> adapters;
        adapters.reserve(count);
        for (VkPhysicalDevice dev : devices)
        {
            adapters.push_back(std::make_unique<VulkanAdapter>(*this, dev));
        }
        return adapters;
    }

    RHIDevice* VulkanInstance::CreateDevice(RHIAdapter& adapter, const RHIDeviceDesc& desc)
    {
        (void)desc;
        auto& vAdapter = static_cast<VulkanAdapter&>(adapter);
        return new VulkanDevice(vAdapter, desc);
    }
}
