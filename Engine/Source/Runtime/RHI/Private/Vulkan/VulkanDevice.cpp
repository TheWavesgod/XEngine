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

        m_EnableVSync = createInfo.EnableVSync;
        m_PendingResizeWidth = createInfo.Width;
        m_PendingResizeHeight = createInfo.Height;

        VulkanSwapchainCreateInfo swapchainCreateInfo;
        swapchainCreateInfo.PhysicalDevice = m_PhysicalDevice;
        swapchainCreateInfo.Device = m_Device;
        swapchainCreateInfo.Surface = m_Surface.GetHandle();
        swapchainCreateInfo.GraphicsQueueFamilyIndex = m_GraphicsFamilyIndex;
        swapchainCreateInfo.PresentQueueFamilyIndex = m_PresentFamilyIndex;
        swapchainCreateInfo.Width = createInfo.Width;
        swapchainCreateInfo.Height = createInfo.Height;
        swapchainCreateInfo.EnableVSync = m_EnableVSync;

        if (!m_Swapchain.Create(swapchainCreateInfo))
        {
            return false;
        }

        if (!m_FrameResources.Create(m_Device, m_GraphicsFamilyIndex, m_Swapchain.GetImageCount()))
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

        m_FrameResources.Destroy();
        m_Swapchain.Destroy();
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
        m_CurrentImageIndex = 0;
        m_FrameActive = false;
        m_ResizeRequested = false;
        m_PendingResizeWidth = 0;
        m_PendingResizeHeight = 0;
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

    void VulkanDevice::BeginFrame()
    {
        m_FrameActive = false;

        if (!IsValid())
        {
            return;
        }

        if (m_ResizeRequested)
        {
            if (m_PendingResizeWidth == 0 || m_PendingResizeHeight == 0)
            {
                return;
            }

            RecreateSwapchain(m_PendingResizeWidth, m_PendingResizeHeight);
            if (m_ResizeRequested)
            {
                return;
            }
        }

        const VkExtent2D extent = m_Swapchain.GetExtent();
        if (extent.width == 0 || extent.height == 0)
        {
            return;
        }

        VkFence inFlightFence = m_FrameResources.GetInFlightFence();
        XENGINE_VK_CHECK(vkWaitForFences(m_Device, 1, &inFlightFence, VK_TRUE, UINT64_MAX));

        VkSemaphore imageAvailableSemaphore = m_FrameResources.GetImageAvailableSemaphore();
        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            m_Swapchain.GetHandle(),
            UINT64_MAX,
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &m_CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            m_ResizeRequested = true;
            return;
        }

        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to acquire Vulkan swapchain image: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return;
        }

        XENGINE_VK_CHECK(vkResetFences(m_Device, 1, &inFlightFence));
        XENGINE_VK_CHECK(vkResetCommandPool(m_Device, m_FrameResources.GetCommandPool(), 0));

        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        XENGINE_VK_CHECK(vkBeginCommandBuffer(m_FrameResources.GetCommandBuffer(), &beginInfo));
        m_FrameActive = true;
    }

    void VulkanDevice::ClearSwapchain(const RHIColor& color)
    {
        if (!m_FrameActive)
        {
            return;
        }

        VkCommandBuffer commandBuffer = m_FrameResources.GetCommandBuffer();
        VkImage image = m_Swapchain.GetImage(m_CurrentImageIndex);

        VkImageSubresourceRange range {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier toTransferBarrier {};
        toTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransferBarrier.srcAccessMask = 0;
        toTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.image = image;
        toTransferBarrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toTransferBarrier);

        VkClearColorValue clearValue {};
        clearValue.float32[0] = color.R;
        clearValue.float32[1] = color.G;
        clearValue.float32[2] = color.B;
        clearValue.float32[3] = color.A;

        vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);

        VkImageMemoryBarrier toPresentBarrier {};
        toPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toPresentBarrier.dstAccessMask = 0;
        toPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        toPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresentBarrier.image = image;
        toPresentBarrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &toPresentBarrier);
    }

    void VulkanDevice::EndFrame()
    {
        if (!m_FrameActive)
        {
            return;
        }

        VkCommandBuffer commandBuffer = m_FrameResources.GetCommandBuffer();
        XENGINE_VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSemaphore imageAvailableSemaphore = m_FrameResources.GetImageAvailableSemaphore();
        VkSemaphore renderFinishedSemaphore = m_FrameResources.GetRenderFinishedSemaphore(m_CurrentImageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

        XENGINE_VK_CHECK(vkQueueSubmit(m_GraphicsQueue.GetHandle(), 1, &submitInfo, m_FrameResources.GetInFlightFence()));

        VkSwapchainKHR swapchain = m_Swapchain.GetHandle();

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult result = vkQueuePresentKHR(m_PresentQueue.GetHandle(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            m_ResizeRequested = true;
        }
        else if (result != VK_SUCCESS)
        {
            std::string message = "Failed to present Vulkan swapchain image: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
        }

        m_FrameActive = false;
    }

    void VulkanDevice::RequestResize(u32 width, u32 height)
    {
        m_PendingResizeWidth = width;
        m_PendingResizeHeight = height;
        m_ResizeRequested = true;

        std::string message = "Vulkan swapchain resize requested: ";
        message += std::to_string(width);
        message += "x";
        message += std::to_string(height);
        XENGINE_LOG_INFO(message);
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device);
    }

    void VulkanDevice::RecreateSwapchain(u32 width, u32 height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        std::string message = "Recreating Vulkan swapchain: ";
        message += std::to_string(width);
        message += "x";
        message += std::to_string(height);
        XENGINE_LOG_INFO(message);

        vkDeviceWaitIdle(m_Device);

        VulkanSwapchainCreateInfo swapchainCreateInfo;
        swapchainCreateInfo.PhysicalDevice = m_PhysicalDevice;
        swapchainCreateInfo.Device = m_Device;
        swapchainCreateInfo.Surface = m_Surface.GetHandle();
        swapchainCreateInfo.GraphicsQueueFamilyIndex = m_GraphicsFamilyIndex;
        swapchainCreateInfo.PresentQueueFamilyIndex = m_PresentFamilyIndex;
        swapchainCreateInfo.Width = width;
        swapchainCreateInfo.Height = height;
        swapchainCreateInfo.EnableVSync = m_EnableVSync;

        if (m_Swapchain.Recreate(swapchainCreateInfo))
        {
            if (m_FrameResources.GetRenderFinishedSemaphoreCount() != m_Swapchain.GetImageCount())
            {
                m_FrameResources.Destroy();
                if (!m_FrameResources.Create(m_Device, m_GraphicsFamilyIndex, m_Swapchain.GetImageCount()))
                {
                    m_ResizeRequested = true;
                    return;
                }
            }

            m_ResizeRequested = false;
        }
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
