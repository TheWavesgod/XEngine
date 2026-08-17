// VulkanDevice — implementation.

#include "VulkanDevice.h"
#include "VulkanAdapter.h"
#include "VulkanInstance.h"
#include "VulkanQueue.h"
#include "VulkanBuffer.h"

// VMA is header-only. Define the implementation macro in exactly one TU
// so its static-inline functions get emitted.
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#include <volk.h>
#include <cstring>
#include <vector>

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIFlags.h>

namespace XEngine
{
    std::unique_ptr<VulkanDevice> VulkanDevice::Create(
        VulkanAdapter& adapter,
        const RHIDeviceDesc& desc)
    {
        // Use new + private ctor so we can release the unique_ptr early if
        // initialization fails (mirrors VulkanInstance::CreateInstance).
        std::unique_ptr<VulkanDevice> device(new VulkanDevice());

        VulkanInstance& instance = adapter.GetVulkanInstance();
        VkPhysicalDevice physicalDevice = adapter.GetVkPhysicalDevice();

        device->m_MaxFramesInFlight = desc.MaxFramesInFlight == 0 ? 2u : desc.MaxFramesInFlight;
        device->m_EnabledFeatures  = RHIFeature::None;  // Phase 3 will populate from desc.RequiredFeatures.

        // We deliberately don't keep an m_Adapter pointer. The Vulkan backend
        // queries VkPhysicalDevice through RHIAdapter (held by the caller),
        // and Phase 1 only needs it during construction. If subsequent code
        // needs stable access, it should copy what it needs into the device.

        device->FindQueueFamilies(physicalDevice);
        if (device->m_GraphicsFamily == ~0u)
        {
            // No graphics-capable queue family → device unusable.
            return nullptr;
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
        if (device->m_GraphicsFamily != ~0u) addQueue(device->m_GraphicsFamily, gInfo);
        if (device->m_ComputeFamily  != ~0u && device->m_ComputeFamily  != device->m_GraphicsFamily) addQueue(device->m_ComputeFamily,  cInfo);
        if (device->m_TransferFamily != ~0u && device->m_TransferFamily != device->m_GraphicsFamily &&
            device->m_TransferFamily != device->m_ComputeFamily) addQueue(device->m_TransferFamily, tInfo);

        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &features;

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device->m_Device);
        if (result != VK_SUCCESS)
        {
            return nullptr;
        }

        volkLoadDevice(device->m_Device);

        // VMA allocator
        VmaAllocatorCreateInfo vmaInfo{};
        vmaInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        vmaInfo.physicalDevice = physicalDevice;
        vmaInfo.device = device->m_Device;
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

        if (vmaCreateAllocator(&vmaInfo, &device->m_VmaAllocator) != VK_SUCCESS)
        {
            device->m_VmaAllocator = VK_NULL_HANDLE;
        }

        device->PopulateCapabilities(physicalDevice);

        // Create VulkanQueue wrappers for each family.
        if (device->m_GraphicsFamily != ~0u)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(device->m_Device, device->m_GraphicsFamily, 0, &q);
            device->m_GraphicsQueue = std::make_unique<VulkanQueue>(*device, q, RHIQueueType::Graphics);
        }
        if (device->m_ComputeFamily != ~0u && device->m_ComputeFamily != device->m_GraphicsFamily)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(device->m_Device, device->m_ComputeFamily, 0, &q);
            device->m_ComputeQueue = std::make_unique<VulkanQueue>(*device, q, RHIQueueType::Compute);
        }
        if (device->m_TransferFamily != ~0u && device->m_TransferFamily != device->m_GraphicsFamily &&
            device->m_TransferFamily != device->m_ComputeFamily)
        {
            VkQueue q = VK_NULL_HANDLE;
            vkGetDeviceQueue(device->m_Device, device->m_TransferFamily, 0, &q);
            device->m_TransferQueue = std::make_unique<VulkanQueue>(*device, q, RHIQueueType::Transfer);
        }

        return device;
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

    void VulkanDevice::FindQueueFamilies(VkPhysicalDevice physicalDevice)
    {
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

    void VulkanDevice::PopulateCapabilities(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);

        m_Caps.MaxTextureSize2D = props.limits.maxImageDimension2D;
        m_Caps.MaxBufferSize = static_cast<u64>(~0ull);  // not directly exposed; conservative
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

        m_Caps.MaxFramesInFlight = m_MaxFramesInFlight;

        m_Caps.MaxComputeWorkGroupInvocations = props.limits.maxComputeWorkGroupInvocations;
        m_Caps.MaxComputeSharedMemorySize = props.limits.maxComputeSharedMemorySize;

        m_Caps.MinUniformBufferOffsetAlignment = props.limits.minUniformBufferOffsetAlignment;
        m_Caps.MinStorageBufferOffsetAlignment = props.limits.minStorageBufferOffsetAlignment;
        m_Caps.MinTexelBufferOffsetAlignment    = props.limits.minTexelBufferOffsetAlignment;

        // Features (Phase 1: conservative defaults; Phase 3 derives these
        // from m_EnabledFeatures so the bool view is a single-source mirror).
        m_Caps.SupportsDepthClip = true;  // Vulkan spec requires it
        m_Caps.SupportsDepthBiasClamp = false;  // Vulkan 1.3 feature; leave false for Phase 1
        m_Caps.SupportsWideLines = (props.limits.lineWidthRange[1] > 1.0f);
        m_Caps.SupportsLargePoints = (props.limits.pointSizeRange[1] > 1.0f);
        m_Caps.SupportsTimelineSemaphore    = HasFlag(m_EnabledFeatures, RHIFeature::TimelineSemaphore);
        m_Caps.SupportsPushDescriptor       = HasFlag(m_EnabledFeatures, RHIFeature::PushDescriptor);
        m_Caps.SupportsBindless             = HasFlag(m_EnabledFeatures, RHIFeature::BindlessDescriptors);
        m_Caps.SupportsBufferDeviceAddress  = HasFlag(m_EnabledFeatures, RHIFeature::BufferDeviceAddress);
        m_Caps.SupportsRayTracing           = HasFlag(m_EnabledFeatures, RHIFeature::RayTracing);
        m_Caps.SupportsGeometryShader       = false;  // no RHIFeature bit yet
        m_Caps.SupportsTessellationShader   = false;  // no RHIFeature bit yet
    }

    RHIBuffer* VulkanDevice::CreateBufferImpl(const RHIBufferDesc& desc)
    {
        // ValidateBufferDesc is already enforced by the NVI wrapper; the
        // factory's defensive re-check is enough to keep this safe in
        // case of a future direct-Impl caller.
        if (m_VmaAllocator == VK_NULL_HANDLE)
        {
            XENGINE_LOG_WARN("VulkanDevice::CreateBufferImpl: VMA allocator not initialised; returning nullptr.");
            return nullptr;
        }
        return VulkanBuffer::Create(*this, desc).release();
    }
}
