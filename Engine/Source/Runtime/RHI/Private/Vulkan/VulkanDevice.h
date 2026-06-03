#pragma once

#include <XEngine/RHI/RHIDevice.h>

#include <volk.h>

namespace XEngine
{
    class VulkanDevice final : public RHIDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice() override;

        RHIBackend GetBackend() const override;
        void WaitIdle() override;

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
    };
}
