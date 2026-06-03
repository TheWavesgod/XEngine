#include "VulkanDevice.h"

#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace XEngine
{
    namespace
    {
        constexpr u32 InvalidQueueFamily = 0xffffffffu;
        constexpr const char* SwapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

        struct VulkanQueueFamilyIndices
        {
            u32 GraphicsFamily = InvalidQueueFamily;
            u32 PresentFamily = InvalidQueueFamily;

            bool IsComplete() const
            {
                return GraphicsFamily != InvalidQueueFamily && PresentFamily != InvalidQueueFamily;
            }
        };

        bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
        {
            u32 extensionCount = 0;
            XENGINE_VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr));

            std::vector<VkExtensionProperties> extensions(extensionCount);
            XENGINE_VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data()));

            for (const VkExtensionProperties& extension : extensions)
            {
                if (std::strcmp(extension.extensionName, SwapchainExtensionName) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        VulkanQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            VulkanQueueFamilyIndices indices;

            u32 familyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

            std::vector<VkQueueFamilyProperties> families(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

            for (u32 index = 0; index < familyCount; ++index)
            {
                if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                {
                    indices.GraphicsFamily = index;
                }

                VkBool32 presentSupport = VK_FALSE;
                XENGINE_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &presentSupport));
                if (presentSupport == VK_TRUE)
                {
                    indices.PresentFamily = index;
                }

                if (indices.IsComplete())
                {
                    break;
                }
            }

            return indices;
        }

        int GetDeviceScore(VkPhysicalDevice physicalDevice)
        {
            VkPhysicalDeviceProperties properties {};
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                return 1000;
            }

            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                return 500;
            }

            return 100;
        }
    }

    VulkanDevice::VulkanDevice() = default;

    VulkanDevice::~VulkanDevice()
    {
        Shutdown();
    }

    bool VulkanDevice::Initialize(const VulkanDeviceCreateInfo& createInfo)
    {
        if (m_Initialized)
        {
            return true;
        }

        XENGINE_LOG_INFO("Initializing Vulkan backend");

        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to initialize volk: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        const std::vector<const char*> requiredExtensions = VulkanSurface::GetRequiredInstanceExtensions();
        if (!m_Instance.Create(createInfo.EnableValidation, requiredExtensions))
        {
            return false;
        }

        if (!m_Surface.Create(m_Instance.GetHandle(), createInfo.NativeWindow))
        {
            return false;
        }

        if (!PickPhysicalDevice())
        {
            return false;
        }

        if (!CreateLogicalDevice())
        {
            return false;
        }

        if (!m_Allocator.Create(m_Instance.GetHandle(), m_PhysicalDevice, m_Device))
        {
            return false;
        }

        m_Initialized = true;
        return true;
    }

    void VulkanDevice::Shutdown()
    {
        if (!m_Initialized && m_Device == VK_NULL_HANDLE && m_PhysicalDevice == VK_NULL_HANDLE)
        {
            return;
        }

        WaitIdle();

        m_Allocator.Destroy();

        if (m_Device != VK_NULL_HANDLE)
        {
            XENGINE_LOG_INFO("Destroying Vulkan device");
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        m_Surface.Destroy();
        m_Instance.Destroy();

        m_PhysicalDevice = VK_NULL_HANDLE;
        m_GraphicsQueue = VulkanQueue {};
        m_PresentQueue = VulkanQueue {};
        m_Initialized = false;
    }

    RHIBackend VulkanDevice::GetBackend() const
    {
        return RHIBackend::Vulkan;
    }

    bool VulkanDevice::IsValid() const
    {
        return m_Initialized && m_Device != VK_NULL_HANDLE;
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device);
    }

    bool VulkanDevice::PickPhysicalDevice()
    {
        u32 deviceCount = 0;
        XENGINE_VK_CHECK(vkEnumeratePhysicalDevices(m_Instance.GetHandle(), &deviceCount, nullptr));

        if (deviceCount == 0)
        {
            XENGINE_LOG_ERROR("No Vulkan physical devices found");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        XENGINE_VK_CHECK(vkEnumeratePhysicalDevices(m_Instance.GetHandle(), &deviceCount, devices.data()));

        VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
        VulkanQueueFamilyIndices selectedIndices;
        int selectedScore = -1;

        for (VkPhysicalDevice device : devices)
        {
            VulkanQueueFamilyIndices indices = FindQueueFamilies(device, m_Surface.GetHandle());
            if (!indices.IsComplete())
            {
                continue;
            }

            if (!CheckDeviceExtensionSupport(device))
            {
                continue;
            }

            const int score = GetDeviceScore(device);
            if (score > selectedScore)
            {
                selectedDevice = device;
                selectedIndices = indices;
                selectedScore = score;
            }
        }

        if (selectedDevice == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("No suitable Vulkan physical device found");
            return false;
        }

        m_PhysicalDevice = selectedDevice;
        m_GraphicsFamilyIndex = selectedIndices.GraphicsFamily;
        m_PresentFamilyIndex = selectedIndices.PresentFamily;

        VkPhysicalDeviceProperties properties {};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

        std::string message = "Selected GPU: ";
        message += properties.deviceName;
        XENGINE_LOG_INFO(message);

        return true;
    }

    bool VulkanDevice::CreateLogicalDevice()
    {
        constexpr f32 queuePriority = 1.0f;

        std::set<u32> uniqueFamilies = { m_GraphicsFamilyIndex, m_PresentFamilyIndex };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueFamilies.size());

        for (u32 familyIndex : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = familyIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        const char* extensions[] = { SwapchainExtensionName };

        VkPhysicalDeviceFeatures features {};

        VkDeviceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = 1;
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.pEnabledFeatures = &features;

        VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan logical device: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        volkLoadDevice(m_Device);

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(m_Device, m_GraphicsFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentFamilyIndex, 0, &presentQueue);

        m_GraphicsQueue.SetHandle(graphicsQueue, m_GraphicsFamilyIndex);
        m_PresentQueue.SetHandle(presentQueue, m_PresentFamilyIndex);

        XENGINE_LOG_INFO("Vulkan logical device created");
        return true;
    }
}
