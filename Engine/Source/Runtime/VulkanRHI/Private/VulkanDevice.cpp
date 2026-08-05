// VulkanDevice — implementation.

#include "VulkanDevice.h"
#include "VulkanAdapter.h"
#include "VulkanInstance.h"
#include "VulkanQueue.h"

// VMA is header-only. Define the implementation macro in exactly one TU
// so its static-inline functions get emitted.
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#include <volk.h>
#include <cstring>
#include <vector>

namespace XEngine
{
    VulkanDevice::VulkanDevice(VulkanAdapter& adapter, const RHIDeviceDesc& desc)
        : RHIDevice(adapter.GetVulkanInstance(), RHIBackend::Vulkan)
        , m_Adapter(&adapter)
    {
        (void)desc;

        VulkanInstance& instance = adapter.GetVulkanInstance();
        VkPhysicalDevice physicalDevice = adapter.GetVkPhysicalDevice();

        FindQueueFamilies();
        if (m_GraphicsFamily == ~0u)
        {
            // No graphics-capable queue family → device unusable.
            return;
        }

        // Build queue create infos for the families we picked.
        const float queuePriority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        auto addQueue = [&](uint32_t family, VkDeviceQueueCreateInfo& info) {
            info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = family;
            info.queueCount = 1;
            info.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(info);
        };

        VkDeviceQueueCreateInfo gInfo{}, cInfo{}, tInfo{};
        if (m_GraphicsFamily != ~0u) addQueue(m_GraphicsFamily, gInfo);
        if (m_ComputeFamily  != ~0u && m_ComputeFamily  != m_GraphicsFamily) addQueue(m_ComputeFamily,  cInfo);
        if (m_TransferFamily != ~0u && m_TransferFamily != m_GraphicsFamily &&
            m_TransferFamily != m_ComputeFamily) addQueue(m_TransferFamily, tInfo);

        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &features;

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_Device);
        if (result != VK_SUCCESS)
        {
            return;
        }

        volkLoadDevice(m_Device);

        // VMA allocator
        VmaAllocatorCreateInfo vmaInfo{};
        vmaInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        vmaInfo.physicalDevice = physicalDevice;
        vmaInfo.device = m_Device;
        vmaInfo.instance = instance.GetVkInstance();

        // VMA needs explicit function pointers when volk is used.
        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;
        vulkanFunctions.vkGetPhysicalDeviceProperties      = vkGetPhysicalDeviceProperties;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
        vulkanFunctions.vkFreeMemory = vkFreeMemory;
        vulkanFunctions.vkMapMemory = vkMapMemory;
        vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
        vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
        vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
        vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vulkanFunctions.vkGetImageMemoryRequirements  = vkGetImageMemoryRequirements;
        vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
        vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
        vulkanFunctions.vkCreateImage = vkCreateImage;
        vulkanFunctions.vkDestroyImage = vkDestroyImage;
        vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
        vmaInfo.pVulkanFunctions = &vulkanFunctions;

        if (vmaCreateAllocator(&vmaInfo, &m_VmaAllocator) != VK_SUCCESS)
        {
            m_VmaAllocator = VK_NULL_HANDLE;
        }

        PopulateCapabilities();

        // Create VulkanQueue wrappers for each family.
        if (m_GraphicsFamily != ~0u)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &q);
            m_GraphicsQueue = std::make_unique<VulkanQueue>(*this, q, RHIQueueType::Graphics);
        }
        if (m_ComputeFamily != ~0u && m_ComputeFamily != m_GraphicsFamily)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(m_Device, m_ComputeFamily, 0, &q);
            m_ComputeQueue = std::make_unique<VulkanQueue>(*this, q, RHIQueueType::Compute);
        }
        if (m_TransferFamily != ~0u && m_TransferFamily != m_GraphicsFamily &&
            m_TransferFamily != m_ComputeFamily)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(m_Device, m_TransferFamily, 0, &q);
            m_TransferQueue = std::make_unique<VulkanQueue>(*this, q, RHIQueueType::Transfer);
        }
    }

    VulkanDevice::~VulkanDevice()
    {
        // Queues must be destroyed before the device.
        m_GraphicsQueue.reset();
        m_ComputeQueue.reset();
        m_TransferQueue.reset();

        if (m_VmaAllocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(m_VmaAllocator);
            m_VmaAllocator = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);
        }
    }

    RHIQueue* VulkanDevice::GetQueue(RHIQueueType type) const
    {
        switch (type)
        {
            case RHIQueueType::Graphics: return m_GraphicsQueue.get();
            case RHIQueueType::Compute:  return m_ComputeQueue.get();
            case RHIQueueType::Transfer: return m_TransferQueue.get();
        }
        return nullptr;
    }

    void VulkanDevice::FindQueueFamilies()
    {
        VkPhysicalDevice physicalDevice = m_Adapter->GetVkPhysicalDevice();
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
        if (count == 0)
        {
            return;
        }
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());

        m_GraphicsFamily = ~0u;
        m_ComputeFamily  = ~0u;
        m_TransferFamily = ~0u;

        for (uint32_t i = 0; i < count; ++i)
        {
            const VkQueueFlags flags = families[i].queueFlags;
            // Unified transfer queue (no graphics/compute) is preferred for transfer family.
            if (m_GraphicsFamily == ~0u && (flags & VK_QUEUE_GRAPHICS_BIT))
            {
                m_GraphicsFamily = i;
            }
            if (m_ComputeFamily == ~0u && (flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT))
            {
                m_ComputeFamily = i;
            }
            if (m_TransferFamily == ~0u && (flags & VK_QUEUE_TRANSFER_BIT) &&
                !(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_COMPUTE_BIT))
            {
                m_TransferFamily = i;
            }
        }

        // Fallback: if no dedicated compute/transfer found, share with graphics.
        if (m_ComputeFamily == ~0u)  m_ComputeFamily  = m_GraphicsFamily;
        if (m_TransferFamily == ~0u) m_TransferFamily = m_GraphicsFamily;
    }

    void VulkanDevice::PopulateCapabilities()
    {
        VkPhysicalDevice physicalDevice = m_Adapter->GetVkPhysicalDevice();

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);

        m_Caps.MaxTextureSize2D = props.limits.maxImageDimension2D;
        m_Caps.MaxBufferSize = ~0ull;  // not directly exposed; conservative
        m_Caps.MaxSamplerAnisotropy = props.limits.maxSamplerAnisotropy;
        m_Caps.MaxSampleCount = 1u;
        if (props.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_8_BIT) m_Caps.MaxSampleCount = 8;
        else if (props.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT) m_Caps.MaxSampleCount = 4;
        else if (props.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_2_BIT) m_Caps.MaxSampleCount = 2;

        m_Caps.MaxViewports = props.limits.maxViewports;
        m_Caps.MaxColorAttachments = 0;
        if (props.limits.framebufferColorSampleCounts != 0)
        {
            // Vulkan exposes per-attachment limits; pick a conservative upper bound.
            m_Caps.MaxColorAttachments = 8;
        }

        m_Caps.MaxFramesInFlight = 2;  // conservative default; user can override via Swapchain

        m_Caps.MaxComputeWorkGroupInvocations = props.limits.maxComputeWorkGroupInvocations;
        m_Caps.MaxComputeSharedMemorySize = props.limits.maxComputeSharedMemorySize;

        m_Caps.MinUniformBufferOffsetAlignment = props.limits.minUniformBufferOffsetAlignment;
        m_Caps.MinStorageBufferOffsetAlignment = props.limits.minStorageBufferOffsetAlignment;
        m_Caps.MinTexelBufferOffsetAlignment    = props.limits.minTexelBufferOffsetAlignment;

        // Features (Phase 1: conservative false defaults).
        m_Caps.SupportsDepthClip = true;  // Vulkan spec requires it
        m_Caps.SupportsDepthBiasClamp = false;  // Vulkan 1.3 feature; leave false for Phase 1
        m_Caps.SupportsWideLines = (props.limits.lineWidthRange[1] > 1.0f);
        m_Caps.SupportsLargePoints = (props.limits.pointSizeRange[1] > 1.0f);
    }
}
