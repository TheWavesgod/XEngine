#include "Device.h"
#include "VkBase.h"

namespace VK
{
    result_t PhysicalDevice::Create(bool enableGraphicsQueue, bool enableComputeQueue)
    {

        if (result_t result = AquireAvailablePhysicalDevices()) return result;

        if (result_t result = DeterminePhysicalDevice(0, enableGraphicsQueue, enableComputeQueue)) return result;

        return VK_SUCCESS;
    }

    result_t PhysicalDevice::AquireAvailablePhysicalDevices()
	{
        uint32_t deviceCount = 0;
        if (VkResult result = vkEnumeratePhysicalDevices(VkBase::Base().Instance(), &deviceCount, nullptr))
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
        if (VkResult result = vkEnumeratePhysicalDevices(VkBase::Base().Instance(), &deviceCount, availablePhysicalDevices.data()))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", static_cast<int32_t>(result));
            return result;
        }

        return VK_SUCCESS;
	}

    result_t PhysicalDevice::DeterminePhysicalDevice(uint32_t deviceIndex, bool enableGraphicsQueue, bool enableComputeQueue)
    {
        // define a special value used for makring a queue index has been checked but not found
        static constexpr uint32_t notFound = INT32_MAX; // == VK_QUEUE_FAMILY_IGNORED & INT32_MAX

        VkSurfaceKHR surface = VkBase::Base().Surface();

        // struct for all the queues
        struct queueFamilyIndexCombination {
            uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
            uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
            uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
        };

        // queueFamilyIndexCombinations is used for reserving a copy of queue family index combination for every physical devices
        static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
        auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];

        // If any queue family index has been checked but not found, return VK_RESULT_MAX_ENUM
        if (ig == notFound && enableGraphicsQueue ||
            ip == notFound && VkBase::Base().Surface() ||
            ic == notFound && enableComputeQueue)
            return VK_RESULT_MAX_ENUM;

        // If any queue family index is needed but haven't been checked
        if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
            ip == VK_QUEUE_FAMILY_IGNORED && surface ||
            ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) 
        {
            uint32_t indices[3];
            VkResult result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);

            // If GetQueueFamilyIndices(...) return VK_SUCCESS or VK_RESULT_MAX_ENUM (vkGetPhysicalDeviceSurfaceSupportKHR(...)successfully excuted but not found all the queues
            // means other queues index needed already have the result, save the result to queueFamilyIndexCombinations[deviceIndex]
            // If the queue index needed is still VK_QUEUE_FAMILY_IGNORED, means not found, VK_QUEUE_FAMILY_IGNORED (~0u) and INT32_MAX bit & is notFound
            if (result == VK_SUCCESS ||
                result == VK_RESULT_MAX_ENUM) {
                if (enableGraphicsQueue)
                    ig = indices[0] & INT32_MAX;
                if (surface)
                    ip = indices[1] & INT32_MAX;
                if (enableComputeQueue)
                    ic = indices[2] & INT32_MAX;
            }
            // If GetQueueFamilyIndices(...) failed, return
            if (result)
                return result;
        }
        // If the two branches up are not excuted, means all the queue family index needed are gained, get index from queueFamilyIndexCombinations[deviceIndex]
        else {
            queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
            queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
            queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
        }
        handle = availablePhysicalDevices[deviceIndex];

        vkGetPhysicalDeviceProperties(handle, &properties);
        vkGetPhysicalDeviceMemoryProperties(handle, &memoryProperties);

        std::cout << std::format("Renderer: {}\n", properties.deviceName);

        if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle, surface, &surfaceCapabilities))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
            return result;
        }

        if (availableSurfaceFormats.empty())
        {
            if (result_t result = GetSurfaceFormats())
            {
                return result;
            }
        }

        if (result_t result = GetSurfacePresentModes())
        {
            return result;
        }

        return VK_SUCCESS;
    }

    result_t PhysicalDevice::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3])
    {
        VkSurfaceKHR surface = VkBase::Base().Surface();

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
                // If need presentation, the best situation is all the queue index is same
                if (supportPresentation)
                {
                    ig = ip = ic = i;
                    break;
                }

                // If ig and ic is different, or they are not found, make ig = ic = i
                if (ig != ic ||
                    ig == VK_QUEUE_FAMILY_IGNORED)
                    ig = ic = i;

                // If presentation is not not needed, break cause already have ig and ic
                if (!surface)
                    break;
            }

            // If any of the queue family index can be accessed but haven't, make it = i
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

        // If any of the queue is needed but not found return VK_RESULT_MAX_ENUM
        if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
            ip == VK_QUEUE_FAMILY_IGNORED && surface ||
            ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
        {
            return VK_RESULT_MAX_ENUM;
        }

        // All the queue index are found, save it
        queueFamilyIndex_graphics = ig;
        queueFamilyIndex_presentation = ip;
        queueFamilyIndex_compute = ic;

        return VK_SUCCESS;
    }

    result_t PhysicalDevice::GetSurfaceFormats()
    {
        uint32_t surfaceFormatCount;
        if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(handle, VkBase::Base().Surface(), &surfaceFormatCount, nullptr))
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
        VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(handle, VkBase::Base().Surface(), &surfaceFormatCount, availableSurfaceFormats.data());
        if (result)
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
        }
        return result;
    }

    result_t PhysicalDevice::GetSurfacePresentModes()
    {
        uint32_t surfacePresentModeCount;
        if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(handle, VkBase::Base().Surface(), &surfacePresentModeCount, nullptr))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
            return result;
        }
        if (!surfacePresentModeCount)
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to find any surface present mode!\n");
            abort();
        }
        surfacePresentModes.resize(surfacePresentModeCount);
        if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(handle, VkBase::Base().Surface(), &surfacePresentModeCount, surfacePresentModes.data()))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
            return result;
        }
        return VK_SUCCESS;
    }

    result_t LogicalDevice::Create(VkDeviceCreateFlags flags)
    {
        PhysicalDevice& physicalDevice = VkBase::Base().PhysicalDevice();

        float queuePriority = 1.f;
        VkDeviceQueueCreateInfo queueCreateInfos[3] = { {}, {}, {} };
        for (VkDeviceQueueCreateInfo& queueCreateInfo : queueCreateInfos)
        {
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
        }

        uint32_t queueCreateInfoCount = 0;
        if (physicalDevice.QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
        {
            queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = physicalDevice.QueueFamilyIndex_Graphics();
        }

        if (physicalDevice.QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
            physicalDevice.QueueFamilyIndex_Presentation() != physicalDevice.QueueFamilyIndex_Graphics())
        {
            queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = physicalDevice.QueueFamilyIndex_Presentation();
        }

        if (physicalDevice.QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED &&
            physicalDevice.QueueFamilyIndex_Compute() != physicalDevice.QueueFamilyIndex_Graphics() &&
            physicalDevice.QueueFamilyIndex_Compute() != physicalDevice.QueueFamilyIndex_Presentation())
        {
            queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = physicalDevice.QueueFamilyIndex_Compute();
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

        if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &handle))
        {
            std::cout << std::format("[ VkBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
            return result;
        }

        if (physicalDevice.QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
        {
            vkGetDeviceQueue(handle, physicalDevice.QueueFamilyIndex_Graphics(), 0, &queue_graphics);
        }
        if (physicalDevice.QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED)
        {
            vkGetDeviceQueue(handle, physicalDevice.QueueFamilyIndex_Presentation(), 0, &queue_presentation);
        }
        if (physicalDevice.QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
        {
            vkGetDeviceQueue(handle, physicalDevice.QueueFamilyIndex_Compute(), 0, &queue_compute);
        }

        for (auto& callback : callbacks_onCreate)
        {
            callback();
        }

        return VK_SUCCESS;
    }

    void LogicalDevice::Destroy()
    {
        for (auto& callback : callbacks_onDestroy) callback();

        vkDestroyDevice(handle, nullptr);

        handle = VK_NULL_HANDLE;
    }
}
