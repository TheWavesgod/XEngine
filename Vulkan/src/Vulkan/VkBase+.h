#pragma once

#include "VkBase.h"
#include "VKFormat.h"

#include "Memory.h"
#include "Image.h"

#include "Command.h"

#include "Synchronization.h"

namespace VK
{
    class VkBasePlus
    {
        VkFormatProperties formatProperties[std::size(formatInfos_v1_0)] = {};
        CommandPool commandPool_graphics;
        CommandPool commandPool_presentation;
        CommandPool commandPool_compute;
        CommandBuffer commandBuffer_transfer;//从commandPool_graphics分配
        CommandBuffer commandBuffer_presentation;
        
        static VkBasePlus singleton;

        VkBasePlus() {
            //在创建逻辑设备时执行Initialize()
            auto Initialize = []()
            {
                if (VkBase::Base().PhysicalDevice().QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
                {
                    singleton.commandPool_graphics.Create(VkBase::Base().PhysicalDevice().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                    singleton.commandPool_graphics.AllocateBuffers(singleton.commandBuffer_transfer);
                }
                    
                if (VkBase::Base().PhysicalDevice().QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
                {
                    singleton.commandPool_compute.Create(VkBase::Base().PhysicalDevice().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                }
                
                if (VkBase::Base().PhysicalDevice().QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
                    VkBase::Base().PhysicalDevice().QueueFamilyIndex_Presentation() != VkBase::Base().PhysicalDevice().QueueFamilyIndex_Graphics() &&
                    VkBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
                {
                    singleton.commandPool_presentation.Create(VkBase::Base().PhysicalDevice().QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
                    singleton.commandPool_presentation.AllocateBuffers(singleton.commandBuffer_presentation);
                }

                for (size_t i = 0; i < std::size(singleton.formatProperties); i++)
                {
                    vkGetPhysicalDeviceFormatProperties(VkBase::Base().PhysicalDevice(), VkFormat(i), &singleton.formatProperties[i]);
                }
                    
                /*待后续填充*/
            };
            //在销毁逻辑设备时执行CleanUp()
            //如果你不需要更换物理设备或在运行中重启Vulkan（皆涉及重建逻辑设备），那么此CleanUp回调非必要
            //程序运行结束时，无论是否有这个回调，graphicsBasePlus中的对象必会在析构graphicsBase前被析构掉
            auto CleanUp = []()
            {
                singleton.commandPool_graphics.~CommandPool();
                singleton.commandPool_presentation.~CommandPool();
                singleton.commandPool_compute.~CommandPool();
            };
            
            VkBase::Plus(singleton);
            
            VkBase::Base().AddCallback_CreateDevice(Initialize);
            VkBase::Base().AddCallback_DestroyDevice(CleanUp);
        }

        VkBasePlus(VkBasePlus&&) = delete;
        ~VkBasePlus() = default;

    public:
        // Getter
        const VkFormatProperties& FormatProperties(VkFormat format) const
        {
#ifndef NDEBUG
            if (uint32_t(format) >= std::size(formatInfos_v1_0))
            {
                outStream << std::format("[ FormatProperties ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n");
                abort();
            }
#endif
            return formatProperties[format];
        }
        const CommandPool& CommandPool_Graphics() const { return commandPool_graphics; }
        const CommandPool& CommandPool_Compute() const { return commandPool_compute; }
        const CommandBuffer& CommandBuffer_Transfer() const { return commandBuffer_transfer; }
        
        //Const Function
        result_t ExecuteCommandBuffer_Graphics(VkCommandBuffer commandBuffer) const
        {
            Fence fence(0);
            VkSubmitInfo submitInfo = {
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer
            };
            
            VkResult result = VkBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
            if (!result) fence.Wait();
            return result;
        }
        
        //该函数专用于向呈现队列提交用于接收交换链图像的队列族所有权的命令缓冲区
        /*result_t AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver, VkSemaphore semaphore_ownershipIsTransfered, VkFence fence = VK_NULL_HANDLE) const
        {
            if (VkResult result = commandBuffer_presentation.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) return result;
            
            VkBase::Base().CmdTransferImageOwnership(commandBuffer_presentation);
            
            if (VkResult result = commandBuffer_presentation.End()) return result;
            
            return VkBase::Base().SubmitCommandBuffer_Presentation(commandBuffer_presentation, semaphore_renderingIsOver, semaphore_ownershipIsTransfered, fence);
        }*/
    };

    inline VkBasePlus VkBasePlus::singleton;

    constexpr formatInfo FormatInfo(VkFormat format)
    {
#ifndef NDEBUG
        if (uint32_t(format) >= std::size(formatInfos_v1_0))
        {
            outStream << std::format("[ FormatInfo ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n");
            abort();
        }
#endif
        return formatInfos_v1_0[uint32_t(format)];
    }
    
    constexpr VkFormat Corresponding16BitFloatFormat(VkFormat format_32BitFloat)
    {
        switch (format_32BitFloat) {
        case VK_FORMAT_R32_SFLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        }
        return format_32BitFloat;
    }
    
    inline const VkFormatProperties& FormatProperties(VkFormat format)
    {
        return VkBase::Plus().FormatProperties(format);
    }

    struct ImageOperation
    {
        struct ImageMemoryBarrierParameterPack 
        {
            const bool isNeeded = false;                            // if need barrier，false default
            const VkPipelineStageFlags stage = 0;                   // srcStages or dstStages
            const VkAccessFlags access = 0;                         // srcAccessMask or dstAccessMask
            const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED; // oldLayout or newLayout
            
            // default constructor，isNeeded reserved false
            constexpr ImageMemoryBarrierParameterPack() = default;
            
            // If parameters are specified, all three parameters must be displayed and specified，isNeeded is assigned to true
            constexpr ImageMemoryBarrierParameterPack(VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout layout) :
                isNeeded(true), stage(stage), access(access), layout(layout) 
            {
            }
        };

        static void CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, const VkBufferImageCopy& region,
            ImageMemoryBarrierParameterPack imb_from, ImageMemoryBarrierParameterPack imb_to)
        {
            VkImageMemoryBarrier imageMemoryBarrier = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                nullptr,
                imb_from.access,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                imb_from.layout,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED, 
                VK_QUEUE_FAMILY_IGNORED,
                image,
                {
                    region.imageSubresource.aspectMask,
                    region.imageSubresource.mipLevel,
                    1,
                    region.imageSubresource.baseArrayLayer,
                    region.imageSubresource.layerCount 
                }
            };

            if (imb_from.isNeeded)
            {
                vkCmdPipelineBarrier(commandBuffer, imb_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }

            vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            if (imb_to.isNeeded) 
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.dstAccessMask = imb_to.access;
                imageMemoryBarrier.newLayout = imb_to.layout;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
                    0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }

        static void CmdBlitImage(VkCommandBuffer commandBuffer, VkImage image_src, VkImage image_dst, const VkImageBlit& region,
            ImageMemoryBarrierParameterPack imb_dst_from, ImageMemoryBarrierParameterPack imb_dst_to, VkFilter filter = VK_FILTER_LINEAR)
        {
            VkImageMemoryBarrier imageMemoryBarrier = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                nullptr,
                imb_dst_from.access,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                imb_dst_from.layout,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                image_dst,
                {
                    region.dstSubresource.aspectMask,
                    region.dstSubresource.mipLevel,
                    1,
                    region.dstSubresource.baseArrayLayer,
                    region.dstSubresource.layerCount
                }
            };

            if (imb_dst_from.isNeeded)
            {
                vkCmdPipelineBarrier(commandBuffer, imb_dst_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }

            vkCmdBlitImage(commandBuffer,
                image_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image_dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region, filter
            );

            if (imb_dst_to.isNeeded)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.dstAccessMask = imb_dst_to.access;
                imageMemoryBarrier.newLayout = imb_dst_to.layout;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_dst_to.stage, 0,
                    0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }

        static void CmdGenerateMipmap2d(VkCommandBuffer commandBuffer, VkImage image, VkExtent2D imageExtent, uint32_t mipLevelCount, uint32_t layerCount,
            ImageMemoryBarrierParameterPack imb_to, VkFilter minFilter = VK_FILTER_LINEAR) 
        {
            auto MipmapExtent = 
                [](VkExtent2D imageExtent, uint32_t mipLevel) 
                {
					VkOffset3D extent = { int32_t(imageExtent.width >> mipLevel), int32_t(imageExtent.height >> mipLevel), 1 };
					extent.x = std::max(1, extent.x);
					extent.y = std::max(1, extent.x);
					return extent;
                };

            if (layerCount > 1) 
            {
                std::unique_ptr<VkImageBlit[]> regions = std::make_unique<VkImageBlit[]>(layerCount);

                for (uint32_t i = 1; i < mipLevelCount; i++)
                {
                    VkOffset3D mipmapExtent_src = MipmapExtent(imageExtent, i - 1);
                    VkOffset3D mipmapExtent_dst = MipmapExtent(imageExtent, i);
                    for (uint32_t j = 1; j < layerCount; j++)
                    {
                        regions[j] = {
                            { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, j, 1 },
                            { {}, mipmapExtent_src },
                            { VK_IMAGE_ASPECT_COLOR_BIT, i, j, 1 },
                            { {}, mipmapExtent_dst }
                        };
                    }

                    VkImageMemoryBarrier imageMemoryBarrier = {
                        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        nullptr,
                        0,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ,
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        image,
                        { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, layerCount }
                    };

                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

                    vkCmdBlitImage(commandBuffer,
                        image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        layerCount, regions.get(), minFilter);

                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }
            }
            else
            {
                for (uint32_t i = 1; i < mipLevelCount; ++i) 
                {
                    VkImageBlit region = {
                        { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 },
                        { {}, MipmapExtent(imageExtent, i - 1) },
                        { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 },
                        { {}, MipmapExtent(imageExtent, i) }
                    };

                    CmdBlitImage(commandBuffer, image, image, region,
                        { VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
                        { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }, minFilter);
                }
            }
                
            if (imb_to.isNeeded) 
            {
                VkImageMemoryBarrier imageMemoryBarrier = {
                    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    nullptr,
                    0,
                    imb_to.access,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    imb_to.layout,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image,
                    { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount }
                };

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
                    0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }
    };
}
