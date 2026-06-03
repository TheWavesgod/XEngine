#include "VulkanInstance.h"

#include "VulkanUtils.h"

#include <XEngine/Core/Types.h>
#include <XEngine/Logging/Log.h>

#include <cstring>
#include <string>
#include <vector>

namespace XEngine
{
    namespace
    {
        constexpr const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";

        bool IsValidationLayerAvailable()
        {
            u32 layerCount = 0;
            XENGINE_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

            std::vector<VkLayerProperties> layers(layerCount);
            XENGINE_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()));

            for (const VkLayerProperties& layer : layers)
            {
                if (std::strcmp(layer.layerName, ValidationLayerName) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData)
        {
            const char* message = callbackData != nullptr ? callbackData->pMessage : "Vulkan validation message";

            if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
            {
                XENGINE_LOG_ERROR(message);
            }
            else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
            {
                XENGINE_LOG_WARN(message);
            }
            else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0)
            {
                XENGINE_LOG_INFO(message);
            }
            else
            {
                XENGINE_LOG_TRACE(message);
            }

            return VK_FALSE;
        }

        VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerInfo()
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = VulkanDebugCallback;
            return createInfo;
        }
    }

    VulkanInstance::~VulkanInstance()
    {
        Destroy();
    }

    bool VulkanInstance::Create(bool enableValidation, const std::vector<const char*>& requiredExtensions)
    {
        if (m_Instance != VK_NULL_HANDLE)
        {
            return true;
        }

        if (requiredExtensions.empty())
        {
            XENGINE_LOG_ERROR("Vulkan instance requires at least one platform extension");
            return false;
        }

        std::vector<const char*> extensions = requiredExtensions;

        std::vector<const char*> layers;
        m_ValidationEnabled = false;
        if (enableValidation)
        {
            if (IsValidationLayerAvailable())
            {
                layers.push_back(ValidationLayerName);
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                m_ValidationEnabled = true;
            }
            else
            {
                XENGINE_LOG_WARN("Vulkan validation layer is unavailable. Continuing without validation.");
            }
        }

        VkApplicationInfo applicationInfo {};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = "XEngine";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.pEngineName = "XEngine";
        applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.apiVersion = VK_API_VERSION_1_3;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = CreateDebugMessengerInfo();

        VkInstanceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &applicationInfo;
        createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = static_cast<u32>(layers.size());
        createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
        createInfo.pNext = m_ValidationEnabled ? &debugCreateInfo : nullptr;

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan instance: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            return false;
        }

        volkLoadInstance(m_Instance);
        XENGINE_LOG_INFO("Vulkan instance created");

        if (m_ValidationEnabled)
        {
            result = vkCreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger);
            if (result == VK_SUCCESS)
            {
                XENGINE_LOG_INFO("Vulkan debug messenger created");
            }
            else
            {
                std::string message = "Failed to create Vulkan debug messenger: ";
                message += VulkanResultToString(result);
                XENGINE_LOG_WARN(message);
            }
        }

        return true;
    }

    void VulkanInstance::Destroy()
    {
        if (m_Instance == VK_NULL_HANDLE)
        {
            return;
        }

        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        XENGINE_LOG_INFO("Destroying Vulkan instance");
        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
        m_ValidationEnabled = false;
    }

    VkInstance VulkanInstance::GetHandle() const
    {
        return m_Instance;
    }

    bool VulkanInstance::IsValidationEnabled() const
    {
        return m_ValidationEnabled;
    }
}
