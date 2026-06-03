#pragma once

#include <volk.h>

#include <vector>

namespace XEngine
{
    class VulkanInstance
    {
    public:
        VulkanInstance() = default;
        ~VulkanInstance();

        bool Create(bool enableValidation, const std::vector<const char*>& requiredExtensions);
        void Destroy();

        VkInstance GetHandle() const;
        bool IsValidationEnabled() const;

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        bool m_ValidationEnabled = false;
    };
}
