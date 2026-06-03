#pragma once

#include <XEngine/Core/Types.h>

#include <volk.h>

namespace XEngine
{
    class VulkanQueue
    {
    public:
        VulkanQueue() = default;
        ~VulkanQueue() = default;

        void SetHandle(VkQueue queue, u32 familyIndex);

        VkQueue GetHandle() const;
        u32 GetFamilyIndex() const;
        bool IsValid() const;

    private:
        VkQueue m_Queue = VK_NULL_HANDLE;
        u32 m_FamilyIndex = 0;
    };
}
