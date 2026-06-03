#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

namespace XEngine
{
    class VulkanAllocator
    {
    public:
        VulkanAllocator() = default;
        ~VulkanAllocator();

        bool Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
        void Destroy();

        VmaAllocator GetHandle() const;

    private:
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
    };
}
