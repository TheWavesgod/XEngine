#include "VulkanQueue.h"

namespace XEngine
{
    void VulkanQueue::SetHandle(VkQueue queue, u32 familyIndex, RHIQueueType type)
    {
        m_Queue = queue;
        m_FamilyIndex = familyIndex;
        m_Type = type;
    }

    VkQueue VulkanQueue::GetHandle() const
    {
        return m_Queue;
    }

    u32 VulkanQueue::GetFamilyIndex() const
    {
        return m_FamilyIndex;
    }

    RHIQueueType VulkanQueue::GetType() const
    {
        return m_Type;
    }

    bool VulkanQueue::IsValid() const
    {
        return m_Queue != VK_NULL_HANDLE;
    }
}
