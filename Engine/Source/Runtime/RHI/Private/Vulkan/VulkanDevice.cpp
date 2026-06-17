#include "VulkanDevice.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"
#include "VulkanPipeline.h"
#include "VulkanSampler.h"
#include "VulkanShader.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/Native/VulkanNativeContext.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
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

        VkImageAspectFlags GetTextureAspectMask(RHIFormat format)
        {
            return format == RHIFormat::D32Float ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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

        if (!CreateDescriptorPool())
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

        if (!CreateDepthTexture())
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

        DestroyDepthTexture();
        m_FrameResources.Destroy();
        m_Swapchain.Destroy();
        DestroyDescriptorPool();
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
        m_CommandList.Reset();
        m_ResizeRequested = false;
        m_PendingResizeWidth = 0;
        m_PendingResizeHeight = 0;
        m_Initialized = false;
    }

    RHIBackend VulkanDevice::GetBackend() const
    {
        return RHIBackend::Vulkan;
    }

    RHIClipSpaceConvention VulkanDevice::GetClipSpaceConvention() const
    {
        RHIClipSpaceConvention convention;
        convention.DepthZeroToOne = true;
        convention.FlipProjectionY = true;
        convention.UseInvertedViewportY = false;
        convention.DefaultFrontFace = RHIFrontFace::CounterClockwise;
        return convention;
    }

    bool VulkanDevice::IsValid() const
    {
        return m_Initialized && m_Device != VK_NULL_HANDLE;
    }

    RHICommandList* VulkanDevice::BeginFrame()
    {
        m_FrameActive = false;
        m_CommandList.Reset();

        if (!IsValid())
        {
            return nullptr;
        }

        if (m_ResizeRequested)
        {
            if (m_PendingResizeWidth == 0 || m_PendingResizeHeight == 0)
            {
                return nullptr;
            }

            RecreateSwapchain(m_PendingResizeWidth, m_PendingResizeHeight);
            if (m_ResizeRequested)
            {
                return nullptr;
            }
        }

        const VkExtent2D extent = m_Swapchain.GetExtent();
        if (extent.width == 0 || extent.height == 0)
        {
            return nullptr;
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
            return nullptr;
        }

        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to acquire Vulkan swapchain image: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return nullptr;
        }

        XENGINE_VK_CHECK(vkResetFences(m_Device, 1, &inFlightFence));
        XENGINE_VK_CHECK(vkResetCommandPool(m_Device, m_FrameResources.GetCommandPool(), 0));

        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        XENGINE_VK_CHECK(vkBeginCommandBuffer(m_FrameResources.GetCommandBuffer(), &beginInfo));
        m_CurrentSwapchainImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_FrameActive = true;

        m_CommandList.BeginFrame(
            m_FrameResources.GetCommandBuffer(),
            m_Swapchain.GetImage(m_CurrentImageIndex),
            m_Swapchain.GetImageView(m_CurrentImageIndex),
            m_Swapchain.GetExtent(),
            &m_CurrentSwapchainImageLayout,
            m_DepthTexture.get());

        return &m_CommandList;
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
        toTransferBarrier.oldLayout = m_CurrentSwapchainImageLayout;
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
        m_CurrentSwapchainImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    void VulkanDevice::EndFrame()
    {
        if (!m_FrameActive)
        {
            return;
        }

        VkCommandBuffer commandBuffer = m_FrameResources.GetCommandBuffer();
        m_CommandList.EndRenderingIfActive();

        if (m_CurrentSwapchainImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            VkImageSubresourceRange range {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            VkAccessFlags srcAccessMask = 0;
            VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            if (m_CurrentSwapchainImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (m_CurrentSwapchainImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }

            VkImageMemoryBarrier toPresentBarrier {};
            toPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toPresentBarrier.srcAccessMask = srcAccessMask;
            toPresentBarrier.dstAccessMask = 0;
            toPresentBarrier.oldLayout = m_CurrentSwapchainImageLayout;
            toPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            toPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toPresentBarrier.image = m_Swapchain.GetImage(m_CurrentImageIndex);
            toPresentBarrier.subresourceRange = range;

            vkCmdPipelineBarrier(
                commandBuffer,
                srcStage,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &toPresentBarrier);

            m_CurrentSwapchainImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        XENGINE_VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSemaphore imageAvailableSemaphore = m_FrameResources.GetImageAvailableSemaphore();
        VkSemaphore renderFinishedSemaphore = m_FrameResources.GetRenderFinishedSemaphore(m_CurrentImageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

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
        m_CommandList.Reset();
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

    std::shared_ptr<RHIShader> VulkanDevice::CreateShader(const RHIShaderDesc& desc)
    {
        auto vulkanShader = std::make_shared<VulkanShader>(m_Device, desc);
        if (!vulkanShader->IsValid())
        {
            return nullptr;
        }

        std::shared_ptr<RHIShader> shader = vulkanShader;
        return shader;
    }

    std::shared_ptr<RHIBuffer> VulkanDevice::CreateBuffer(
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        auto buffer = std::make_shared<VulkanBuffer>(m_Allocator.GetHandle(), desc, initialData, initialDataSize);
        if (!buffer->IsValid())
        {
            return nullptr;
        }

        return buffer;
    }

    std::shared_ptr<RHITexture> VulkanDevice::CreateTexture(
        const RHITextureDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        auto texture = std::make_shared<VulkanTexture>(m_Device, m_Allocator.GetHandle(), desc);
        if (!texture->IsValid())
        {
            return nullptr;
        }

        if (initialData != nullptr && initialDataSize > 0)
        {
            RHIBufferDesc stagingDesc;
            stagingDesc.Size = initialDataSize;
            stagingDesc.Usage = RHIBufferUsage::TransferSrc;
            stagingDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
            stagingDesc.DebugName = "Texture upload staging buffer";

            VulkanBuffer stagingBuffer(m_Allocator.GetHandle(), stagingDesc, initialData, initialDataSize);
            if (!stagingBuffer.IsValid())
            {
                XENGINE_LOG_ERROR("Failed to create texture upload staging buffer");
                return nullptr;
            }

            // TODO Stage 8/10:
            // Replace immediate submit upload path with RHIUploadManager / transfer queue / async upload.
            ImmediateSubmit([&](VkCommandBuffer commandBuffer)
            {
                VkImageSubresourceRange range {};
                range.aspectMask = GetTextureAspectMask(desc.Format);
                range.baseMipLevel = 0;
                range.levelCount = 1;
                range.baseArrayLayer = 0;
                range.layerCount = desc.ArrayLayers;

                VkImageMemoryBarrier toTransfer {};
                toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                toTransfer.srcAccessMask = 0;
                toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                toTransfer.oldLayout = *texture->GetLayoutPtr();
                toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toTransfer.image = texture->GetImage();
                toTransfer.subresourceRange = range;

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
                    &toTransfer);

                VkBufferImageCopy copyRegion {};
                copyRegion.bufferOffset = 0;
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;
                copyRegion.imageSubresource.aspectMask = range.aspectMask;
                copyRegion.imageSubresource.mipLevel = 0;
                copyRegion.imageSubresource.baseArrayLayer = 0;
                copyRegion.imageSubresource.layerCount = desc.ArrayLayers;
                copyRegion.imageOffset = { 0, 0, 0 };
                copyRegion.imageExtent = { desc.Width, desc.Height, 1 };

                vkCmdCopyBufferToImage(
                    commandBuffer,
                    stagingBuffer.GetHandle(),
                    texture->GetImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion);

                VkImageMemoryBarrier toShaderRead {};
                toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toShaderRead.image = texture->GetImage();
                toShaderRead.subresourceRange = range;

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &toShaderRead);
            });

            *texture->GetLayoutPtr() = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        return texture;
    }

    std::shared_ptr<RHISampler> VulkanDevice::CreateSampler(const RHISamplerDesc& desc)
    {
        auto sampler = std::make_shared<VulkanSampler>(m_Device, desc);
        if (!sampler->IsValid())
        {
            return nullptr;
        }

        return sampler;
    }

    std::shared_ptr<RHIBindGroupLayout> VulkanDevice::CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc)
    {
        auto layout = std::make_shared<VulkanBindGroupLayout>();
        if (!layout->Create(m_Device, desc))
        {
            return nullptr;
        }

        return layout;
    }

    std::shared_ptr<RHIBindGroup> VulkanDevice::CreateBindGroup(const RHIBindGroupDesc& desc)
    {
        auto bindGroup = std::make_shared<VulkanBindGroup>();
        if (!bindGroup->Create(m_Device, m_DescriptorPool, desc))
        {
            return nullptr;
        }

        return bindGroup;
    }

    std::shared_ptr<RHIPipeline> VulkanDevice::CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
    {
        auto pipeline = std::make_shared<VulkanPipeline>(m_Device, desc);
        if (!pipeline->IsValid())
        {
            return nullptr;
        }

        return pipeline;
    }

    RHIFormat VulkanDevice::GetSwapchainFormat() const
    {
        return VulkanFormatToRHIFormat(m_Swapchain.GetImageFormat());
    }

    bool VulkanDevice::GetVulkanNativeContext(VulkanNativeContext& outContext) const
    {
        if (!IsValid())
        {
            return false;
        }

        outContext.Instance = m_Instance.GetHandle();
        outContext.PhysicalDevice = m_PhysicalDevice;
        outContext.Device = m_Device;
        outContext.GraphicsQueue = m_GraphicsQueue.GetHandle();
        outContext.GraphicsQueueFamilyIndex = m_GraphicsQueue.GetFamilyIndex();
        outContext.MinImageCount = m_Swapchain.GetImageCount();
        outContext.ImageCount = m_Swapchain.GetImageCount();
        outContext.ColorFormat = m_Swapchain.GetImageFormat();
        outContext.DepthFormat = VK_FORMAT_D32_SFLOAT;
        return true;
    }

    void VulkanDevice::RenderVulkanOverlay(const std::function<void(RHINativeCommandBuffer)>& callback)
    {
        if (!m_FrameActive || !callback)
        {
            return;
        }

        VkCommandBuffer commandBuffer = m_FrameResources.GetCommandBuffer();
        m_CommandList.EndRenderingIfActive();

        const bool swapchainWasUndefined = m_CurrentSwapchainImageLayout == VK_IMAGE_LAYOUT_UNDEFINED;
        if (m_CurrentSwapchainImageLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            VkImageSubresourceRange range {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            VkImageMemoryBarrier barrier {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = m_CurrentSwapchainImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ?
                VK_ACCESS_TRANSFER_WRITE_BIT :
                (swapchainWasUndefined ? 0 : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = m_CurrentSwapchainImageLayout;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_Swapchain.GetImage(m_CurrentImageIndex);
            barrier.subresourceRange = range;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            m_CurrentSwapchainImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkRenderingAttachmentInfo colorAttachment {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_Swapchain.GetImageView(m_CurrentImageIndex);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = swapchainWasUndefined ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { { 0.08f, 0.08f, 0.1f, 1.0f } };

        VkRenderingInfo renderingInfo {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = m_Swapchain.GetExtent();
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        // Editor UI is rendered after the scene and before present so it overlays
        // the final swapchain image without becoming part of runtime scene rendering.
        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        callback(static_cast<RHINativeCommandBuffer>(commandBuffer));
        vkCmdEndRendering(commandBuffer);
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device);
    }

    bool VulkanDevice::CreateDepthTexture()
    {
        const VkExtent2D extent = m_Swapchain.GetExtent();
        if (extent.width == 0 || extent.height == 0)
        {
            return false;
        }

        RHITextureDesc desc;
        desc.Width = extent.width;
        desc.Height = extent.height;
        desc.Format = RHIFormat::D32Float;
        desc.Usage = RHITextureUsageFlags::DepthStencilAttachment;
        desc.DebugName = "Swapchain depth";

        auto depthTexture = std::make_unique<VulkanTexture>(m_Device, m_Allocator.GetHandle(), desc);
        if (!depthTexture->IsValid())
        {
            return false;
        }

        m_DepthTexture = std::move(depthTexture);
        return true;
    }

    bool VulkanDevice::CreateDescriptorPool()
    {
        // TODO Stage 8/10: replace this global pool with a descriptor allocator / arena.
        // TODO Stage 11: replace per-material descriptors with BindlessResourceManager.
        VkDescriptorPoolSize poolSizes[] = {
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128 },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 }
        };

        VkDescriptorPoolCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        createInfo.maxSets = 1024;
        createInfo.poolSizeCount = static_cast<u32>(std::size(poolSizes));
        createInfo.pPoolSizes = poolSizes;

        VkResult result = vkCreateDescriptorPool(m_Device, &createInfo, nullptr, &m_DescriptorPool);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan descriptor pool: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        XENGINE_LOG_INFO("Vulkan descriptor pool created");
        return true;
    }

    void VulkanDevice::DestroyDescriptorPool()
    {
        if (m_Device != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE)
        {
            XENGINE_LOG_INFO("Destroying Vulkan descriptor pool");
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
    }

    void VulkanDevice::DestroyDepthTexture()
    {
        m_DepthTexture.reset();
    }

    void VulkanDevice::ImmediateSubmit(const std::function<void(VkCommandBuffer)>& function)
    {
        if (m_Device == VK_NULL_HANDLE || m_GraphicsQueue.GetHandle() == VK_NULL_HANDLE)
        {
            return;
        }

        VkCommandPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_GraphicsFamilyIndex;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        XENGINE_VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &commandPool));

        VkCommandBufferAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        XENGINE_VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocateInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        XENGINE_VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        function(commandBuffer);

        XENGINE_VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        XENGINE_VK_CHECK(vkQueueSubmit(m_GraphicsQueue.GetHandle(), 1, &submitInfo, VK_NULL_HANDLE));
        XENGINE_VK_CHECK(vkQueueWaitIdle(m_GraphicsQueue.GetHandle()));

        vkFreeCommandBuffers(m_Device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(m_Device, commandPool, nullptr);
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
        DestroyDepthTexture();

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
            if (!CreateDepthTexture())
            {
                m_ResizeRequested = true;
                return;
            }

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

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures {};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures {};
        shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
        shaderDrawParametersFeatures.pNext = &dynamicRenderingFeatures;
        shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;

        VkDeviceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &shaderDrawParametersFeatures;
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
