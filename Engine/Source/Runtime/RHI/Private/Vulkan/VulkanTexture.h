#pragma once

namespace XEngine
{
    class VulkanTexture
    {
    public:
        VulkanTexture() = default;
        ~VulkanTexture() = default;

        // TODO(Stage 2B): Own VkImage, VkImageView, and VMA allocation handles.
    };
}
