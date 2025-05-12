#include "VkBase.h"

VK::VkBase::~VkBase()
{
    if (!instance) return;

    if (device)
    {
        WaitIdle();

        if (swapchain)
        {
            for (auto& callback : callbacks_destroySwapchain) callback();
            
            for (VkImageView& imageView : swapchainImageViews)
            {
                if (imageView) vkDestroyImageView(device, imageView, nullptr);
            }
            
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        for (auto& callback : callbacks_destroyDevice) callback();
        vkDestroyDevice(device, nullptr);
    }

    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    
    if (instance != VK_NULL_HANDLE) instance.Destroy();
}

VkResult VK::VkBase::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue,
    bool enableComputeQueue, uint32_t(& queueFamilyIndices)[3])
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (!queueFamilyCount)
    {
        return VK_RESULT_MAX_ENUM;
    }

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    auto& [ig, ip, ic] = queueFamilyIndices; //each of these refer to graphic, presentation and computation
    ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 supportGraphics = enableGraphicsQueue && queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT,
                supportPresentation = false,
                supportCompute = enableComputeQueue && queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;

        if (surface)
        {
            if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation))
            {
                std::cout << std::format("[ VkBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
                return result;
            }
        }

        if (supportGraphics && supportCompute)
        {
            //若需要呈现，最好是三个队列族索引全部相同
            if (supportPresentation)
            {
                ig = ip = ic = i;
                break;
            }
            //除非ig和ic都已取得且相同，否则将它们的值覆写为i，以确保两个队列族索引相同
            if (ig != ic ||
                ig == VK_QUEUE_FAMILY_IGNORED)
                ig = ic = i;
            //如果不需要呈现，那么已经可以break了
            if (!surface)
                break;
        }
        //若任何一个队列族索引可以被取得但尚未被取得，将其值覆写为i
        if (supportGraphics &&
            ig == VK_QUEUE_FAMILY_IGNORED)
            ig = i;
        if (supportPresentation &&
            ip == VK_QUEUE_FAMILY_IGNORED)
            ip = i;
        if (supportCompute &&
            ic == VK_QUEUE_FAMILY_IGNORED)
            ic = i;
    }

    if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
        ip == VK_QUEUE_FAMILY_IGNORED && surface ||
        ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
    {
        return VK_RESULT_MAX_ENUM;
    }

    //函数执行成功时，将所取得的队列族索引写入到成员变量
    queueFamilyIndex_graphics = ig;
    queueFamilyIndex_presentation = ip;
    queueFamilyIndex_compute = ic;

    return VK_SUCCESS;
}

VkResult VK::VkBase::GetPhysicalDevices()
{
    uint32_t deviceCount = 0;
    if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
        return result;
    }

    if (!deviceCount)
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to find any physical device supports vulkan!\n"),
        abort();
    }

    availablePhysicalDevices.resize(deviceCount);
    if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data()))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", static_cast<int32_t>(result));
        return result;
    }
    
    return VK_SUCCESS;
}

VkResult VK::VkBase::DeterminePhysicalDevice(uint32_t deviceIndex, bool enableGraphicsQueue, bool enableComputeQueue)
{
    //定义一个特殊值用于标记一个队列族索引已被找过但未找到
    static constexpr uint32_t notFound = INT32_MAX;//== VK_QUEUE_FAMILY_IGNORED & INT32_MAX

    //定义队列族索引组合的结构体
    struct queueFamilyIndexCombination {
        uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
        uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
        uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
    };

    //queueFamilyIndexCombinations用于为各个物理设备保存一份队列族索引组合
    static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
    auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];

    //如果有任何队列族索引已被找过但未找到，返回VK_RESULT_MAX_ENUM
    if (ig == notFound && enableGraphicsQueue ||
        ip == notFound && surface ||
        ic == notFound && enableComputeQueue)
        return VK_RESULT_MAX_ENUM;

    //如果有任何队列族索引应被获取但还未被找过
    if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
        ip == VK_QUEUE_FAMILY_IGNORED && surface ||
        ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
        uint32_t indices[3];
        VkResult result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
        //若GetQueueFamilyIndices(...)返回VK_SUCCESS或VK_RESULT_MAX_ENUM（vkGetPhysicalDeviceSurfaceSupportKHR(...)执行成功但没找齐所需队列族），
        //说明对所需队列族索引已有结论，保存结果到queueFamilyIndexCombinations[deviceIndex]中相应变量
        //应被获取的索引若仍为VK_QUEUE_FAMILY_IGNORED，说明未找到相应队列族，VK_QUEUE_FAMILY_IGNORED（~0u）与INT32_MAX做位与得到的数值等于notFound
        if (result == VK_SUCCESS ||
            result == VK_RESULT_MAX_ENUM) {
            if (enableGraphicsQueue)
                ig = indices[0] & INT32_MAX;
            if (surface)
                ip = indices[1] & INT32_MAX;
            if (enableComputeQueue)
                ic = indices[2] & INT32_MAX;
        }
        //如果GetQueueFamilyIndices(...)执行失败，return
        if (result)
            return result;
    }

    //若以上两个if分支皆不执行，则说明所需的队列族索引皆已被获取，从queueFamilyIndexCombinations[deviceIndex]中取得索引
    else {
        queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
        queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
        queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
    }
    physicalDevice = availablePhysicalDevices[deviceIndex];
    return VK_SUCCESS;
}

VkResult VK::VkBase::CreateDevice(VkDeviceCreateFlags flags)
{
    float queuePriority = 1.f;
    VkDeviceQueueCreateInfo queueCreateInfos[3] = {{}, {}, {}};
    for (VkDeviceQueueCreateInfo& queueCreateInfo : queueCreateInfos)
    {
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount  = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
    }
    uint32_t queueCreateInfoCount = 0;
    if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
    {
        queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
    }
    if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
        queueFamilyIndex_presentation != queueFamilyIndex_graphics)
    {
        queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
    }
    if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
        queueFamilyIndex_compute != queueFamilyIndex_graphics &&
        queueFamilyIndex_compute != queueFamilyIndex_presentation)
    {
        queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.flags = flags;
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
        return result;
    }

    if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
    {
        vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
    }
    if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
    {
        vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
    }
    if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
    {
        vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);
    }

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

    std::cout << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);

    for (auto& callback : callbacks_createDevice)
    {
        callback();
    }
    
    return VK_SUCCESS;
}

VkResult VK::VkBase::CreateSwapchain_Internal()
{
    if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
        return result;
    }

    uint32_t swapchainImageCount;
    if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
        return result;
    }
    swapchainImages.resize(swapchainImageCount);
    if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
        return result;
    }

    swapchainImageViews.resize(swapchainImageCount);
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
    imageViewCreateInfo.subresourceRange= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    for (size_t i = 0; i < swapchainImageCount; ++i)
    {
        imageViewCreateInfo.image = swapchainImages[i];
        if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
            return result;
        }
    }
    return VK_SUCCESS;
}

VkResult VK::VkBase::GetSurfaceFormats()
{
    uint32_t surfaceFormatCount;
    if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
        return result;
    }

    if (!surfaceFormatCount)
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to find any supported surface format!\n");
        abort();
    }
    availableSurfaceFormats.resize(surfaceFormatCount);
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
    if (result)
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
    }
    return result;
}

VkResult VK::VkBase::SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat)
{
    bool formatIsAvailable = false;
    if (!surfaceFormat.format)
    {
        //如果格式未指定，只匹配色彩空间，图像格式有啥就用啥
        for (VkSurfaceFormatKHR& i : availableSurfaceFormats)
        {
            if (i.colorSpace == surfaceFormat.colorSpace)
            {
                swapchainCreateInfo.imageFormat = i.format;
                swapchainCreateInfo.imageColorSpace = i.colorSpace;
                formatIsAvailable = true;
                break;
            }
        }
            
    }
    else
    {
        //否则匹配格式和色彩空间
        for (auto& i : availableSurfaceFormats)
        {
            if (i.format == surfaceFormat.format && i.colorSpace == surfaceFormat.colorSpace)
            {
                swapchainCreateInfo.imageFormat = i.format;
                swapchainCreateInfo.imageColorSpace = i.colorSpace;
                formatIsAvailable = true;
                break;
            }
        }
    }

    //如果没有符合的格式，恰好有个语义相符的错误代码
    if (!formatIsAvailable)
    {
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }
    //如果交换链已存在，调用RecreateSwapchain()重建交换链
    if (swapchain)
    {
        return RecreateSwapchain();
    }
        
    return VK_SUCCESS;
}

VkResult VK::VkBase::CreateSwapchain(bool limitFrameRate, VkSwapchainCreateFlagsKHR flags)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
        return result;
    }

    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
    swapchainCreateInfo.imageExtent =
        surfaceCapabilities.currentExtent.width == -1 ?
                VkExtent2D{
                    glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
                    glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
                surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;

    if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    else
    {
        for (size_t i = 0; i < 4; ++i)
        {
            if (surfaceCapabilities.supportedCompositeAlpha & 1 << i)
            {
                swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
                break;
            }
        }
    }

    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    else
    {
        std::cout << std::format("[ VkBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");
    }

    if (availableSurfaceFormats.empty())
    {
        if (VkResult result = GetSurfaceFormats())
        {
            return result;
        }
    }

    if (!swapchainCreateInfo.imageFormat)
    {
        if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
            SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }))
        {
            //如果找不到上述图像格式和色彩空间的组合，那只能有什么用什么，采用availableSurfaceFormats中的第一组
            swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
            swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
            std::cout << std::format("[ VkBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
        }
    }

    uint32_t surfacePresentModeCount;
    if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
        return result;
    }
    if (!surfacePresentModeCount)
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to find any surface present mode!\n");
        abort();
    }
    std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
    if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data()))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
        return result;
    }
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!limitFrameRate)
    {
        for (size_t i = 0; i < surfacePresentModeCount; i++)
        {
            if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }
       
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.flags = flags;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.clipped = VK_TRUE;

    if (VkResult result = CreateSwapchain_Internal())
    {
        return result;
    }

    for (auto& callback :  callbacks_createSwapchain)
    {
        callback();
    }

    return VK_SUCCESS;
}

VkResult VK::VkBase::RecreateSwapchain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities))
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
        return result;
    }
    if (surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0)
    {
        return VK_SUBOPTIMAL_KHR;
    }
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.oldSwapchain = swapchain;

    VkResult result = vkQueueWaitIdle(queue_graphics);
    //仅在等待图形队列成功，且图形与呈现所用队列不同时等待呈现队列
    if (result == VK_SUCCESS && queue_graphics != queue_presentation)
    {
        result = vkQueueWaitIdle(queue_presentation);
    }
    if (result)
    {
        std::cout << std::format("[ VkBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
        return result;
    }

    for (auto& callback :  callbacks_destroySwapchain)
    {
        callback();
    }

    for (VkImageView& imageView : swapchainImageViews)
    {
        if (imageView)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }
    swapchainImageViews.resize(0);

    if (result = CreateSwapchain_Internal())
    {
        return result;
    }
    
    for (const auto& callback :  callbacks_createSwapchain)
    {
        callback();
    }
    
    return VK_SUCCESS;
}

VkResult VK::VkBase::RecreateDevice(VkDeviceCreateFlags flags)
{
    if (VkResult result = WaitIdle()) return result;

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
    return CreateDevice(flags);
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
    if (swapchainCreateInfo.oldSwapchain &&
        swapchainCreateInfo.oldSwapchain != swapchain)
    {
        vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    }

    while(VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
    {
        switch (result)
        {
        case VK_SUBOPTIMAL_KHR:
            
        case VK_ERROR_OUT_OF_DATE_KHR:
            if (VkResult result = RecreateSwapchain()) return result;
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
    VkResult result = vkQueueSubmit(queue_graphics, 1, &submitInfo, fence);
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
    VkResult result = vkQueueSubmit(queue_compute, 1, &submitInfo, fence);
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
    switch (VkResult result = vkQueuePresentKHR(queue_presentation, &presentInfo))
    {
    case VK_SUCCESS:
        return VK_SUCCESS;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        return RecreateSwapchain();
    default:
        outStream << std::format("[ graphicsBase ] ERROR\nFailed to queue the image for presentation!\nError code: {}\n", int32_t(result));
        return result;
    }
}

result_t VK::VkBase::PresentImage(VkSemaphore semaphore_renderingIsOver)
{
    VkPresentInfoKHR presentInfo = {
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &currentImageIndex
    };
    
    if (semaphore_renderingIsOver)
    {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore_renderingIsOver;
    }
    return PresentImage(presentInfo);
}


