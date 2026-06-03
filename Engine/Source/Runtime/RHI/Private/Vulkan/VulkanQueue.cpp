#include "VulkanQueue.h"

namespace XEngine
{
    void VulkanQueue::SetHandle(VkQueue queue, u32 familyIndex)
    {
        m_Queue = queue;
        m_FamilyIndex = familyIndex;
    }

    VkQueue VulkanQueue::GetHandle() const
    {
        return m_Queue;
    }

    u32 VulkanQueue::GetFamilyIndex() const
    {
        return m_FamilyIndex;
    }

    bool VulkanQueue::IsValid() const
    {
        return m_Queue != VK_NULL_HANDLE;
    }
}
