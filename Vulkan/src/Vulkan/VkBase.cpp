#include "VkBase.h"

VK::VkBase::~VkBase()
{
    if (!instance) return;

    if (device)
    {
        WaitIdle();

        swapchain.Destroy();

        for (auto& callback : callbacks_destroyDevice) callback();
        vkDestroyDevice(device, nullptr);
    }

    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    
    if (instance != VK_NULL_HANDLE) instance.Destroy();
}

result_t VK::VkBase::GetPhysicalDevice(bool enableGraphicsQueue, bool enableComputeQueue)
{
    return physicalDevice.Create(instance, surface, enableGraphicsQueue, enableComputeQueue);
}

result_t VK::VkBase::CreateDevice()
{
    return device.Create(physicalDevice);
}

result_t VK::VkBase::BuildSwapchain(bool limitFrameRate)
{
    return swapchain.Build(physicalDevice, device, limitFrameRate);
}


VkResult VK::VkBase::RecreateDevice(VkDeviceCreateFlags flags)
{
    /*if (VkResult result = WaitIdle()) return result;

    if (swapchain)
    {
        for (auto& callback :  callbacks_destroySwapchain)
        {
            callback();
        }
        
        for (VkImageView& imageView :  swapchainImageViews)
        {
            if (imageView) vkDestroyImageView(device, imageView, nullptr);
        }
        
        swapchainImageViews.resize(0);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
        swapchainCreateInfo = {};
    }

    for (auto& callback : callbacks_destroyDevice)
    {
        callback();
    }
    if (device)
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
    return CreateDevice(flags);*/
    return VK_SUCCESS;
}

VkResult VK::VkBase::WaitIdle() const
{
    VkResult result = vkDeviceWaitIdle(device);
    if (result)
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
    }
    return result; 
}

VkResult VK::VkBase::UseLatestApiVersion()
{
    if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
    {
        return vkEnumerateInstanceVersion(&apiVersion);
    }
    return VK_SUCCESS;
}

result_t VK::VkBase::SwapImage(VkSemaphore semaphore_imageIsAvailable)
{
    if (swapchain.SwapchainCreateInfo().oldSwapchain &&
        swapchain.SwapchainCreateInfo().oldSwapchain != swapchain)
    {
        vkDestroySwapchainKHR(device, swapchain.SwapchainCreateInfo().oldSwapchain, nullptr);
        swapchain.SwapchainCreateInfo().oldSwapchain = VK_NULL_HANDLE;
    }

    while(VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
    {
        switch (result)
        {
        case VK_SUBOPTIMAL_KHR:
            
        case VK_ERROR_OUT_OF_DATE_KHR:
            if (VkResult result = swapchain.ReBuild()) return result;
            break; //注意重建交换链后仍需要获取图像，通过break递归，再次执行while的条件判定语句
            
        default:
            outStream << std::format("[ VkBase ] ERROR\nFailed to acquire the next image!\nError code: {}\n", int32_t(result));
            return result;
        }
    }

    return VK_SUCCESS;
}

result_t VK::VkBase::SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence) const
{
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkResult result = vkQueueSubmit(device.Queue_Graphics(), 1, &submitInfo, fence);
    if (result)
    {
        outStream << std::format("[ VkBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
    }
    return result;
}

result_t VK::VkBase::SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkSemaphore semaphore_imageIsAvailable,
    VkSemaphore semaphore_renderingIsOver, VkFence fence, VkPipelineStageFlags waitDstStage_imageIsAvailable) const
{
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (semaphore_imageIsAvailable)
    {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable;
        submitInfo.pWaitDstStageMask = &waitDstStage_imageIsAvailable;
    }
    if (semaphore_renderingIsOver)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
    }
    return SubmitCommandBuffer_Graphics(submitInfo, fence);
}

result_t VK::VkBase::SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence) const
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    return SubmitCommandBuffer_Graphics(submitInfo, fence);
}

result_t VK::VkBase::SubmitCommandBuffer_Compute(VkSubmitInfo& submitInfo, VkFence fence) const
{
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkResult result = vkQueueSubmit(device.Queue_Compute(), 1, &submitInfo, fence);
    if (result)
    {
        outStream << std::format("[ VkBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
    }
    return result;
}

result_t VK::VkBase::SubmitCommandBuffer_Compute(VkCommandBuffer commandBuffer, VkFence fence) const
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    return SubmitCommandBuffer_Compute(submitInfo, fence);
}

result_t VK::VkBase::PresentImage(VkPresentInfoKHR& presentInfo)
{
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    switch (VkResult result = vkQueuePresentKHR(device.Queue_Presentation(), &presentInfo))
    {
    case VK_SUCCESS:
        return VK_SUCCESS;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        return swapchain.ReBuild();
    default:
        outStream << std::format("[ VkBase ] ERROR\nFailed to queue the image for presentation!\nError code: {}\n", int32_t(result));
        return result;
    }
}

result_t VK::VkBase::PresentImage(VkSemaphore semaphore_renderingIsOver)
{
    VkPresentInfoKHR presentInfo = {
        .swapchainCount = 1,
        .pSwapchains = &swapchain.Ref(),
        .pImageIndices = &currentImageIndex
    };
    
    if (semaphore_renderingIsOver)
    {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore_renderingIsOver;
    }
    return PresentImage(presentInfo);
}


