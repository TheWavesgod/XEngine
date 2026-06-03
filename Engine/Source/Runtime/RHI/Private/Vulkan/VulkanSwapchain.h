#pragma once

#include <XEngine/Core/Types.h>

#include <volk.h>

#include <vector>

namespace XEngine
{
    struct VulkanSwapchainCreateInfo
    {
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        VkSurfaceKHR Surface = VK_NULL_HANDLE;
        u32 GraphicsQueueFamilyIndex = 0;
        u32 PresentQueueFamilyIndex = 0;
        u32 Width = 1280;
        u32 Height = 720;
        bool EnableVSync = true;
    };

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain() = default;
        ~VulkanSwapchain();

        bool Create(const VulkanSwapchainCreateInfo& createInfo);
        void Destroy();

        bool Recreate(const VulkanSwapchainCreateInfo& createInfo);

        VkSwapchainKHR GetHandle() const;
        VkFormat GetImageFormat() const;
        VkExtent2D GetExtent() const;

        u32 GetImageCount() const;
        VkImage GetImage(u32 index) const;
        VkImageView GetImageView(u32 index) const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR m_ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkExtent2D m_Extent {};

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
    };
}
