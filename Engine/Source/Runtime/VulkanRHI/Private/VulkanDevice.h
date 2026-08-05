// VulkanDevice — concrete RHIDevice for the Vulkan backend.
//
// Wraps a VkDevice + VmaAllocator. Manages per-queue family VulkanQueue
// objects and centralises backend-owned resources.
//
// Phase 1 (M0-M3): wraps device + queue + populates capabilities.
// M4-M6 hooks (CreateBuffer/CreateTexture/CreateCommandList/etc.) are
// stubbed to return nullptr; they are implemented in later phases.

#pragma once

#include <XEngine/RHI/RHIDevice.h>  // also defines RHICapabilities

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <memory>

namespace XEngine
{
    class VulkanAdapter;
    class VulkanQueue;

    class VulkanDevice : public RHIDevice
    {
    public:
        VulkanDevice(VulkanAdapter& adapter, const RHIDeviceDesc& desc);
        ~VulkanDevice() override;

        // RHIDevice interface
        void WaitIdle() override;
        RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
        const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
        u32 GetMaxFramesInFlight() const noexcept override { return 2; }
        RHIQueue* GetQueue(RHIQueueType type) const override;

        // M4-M6 hooks (Phase 2+)
        RHIBuffer* CreateBufferImpl(const RHIBufferDesc&) override { return nullptr; }
        RHITexture* CreateTextureImpl(const RHITextureDesc&) override { return nullptr; }
        RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc&) override { return nullptr; }
        RHISampler* CreateSamplerImpl(const RHISamplerDesc&) override { return nullptr; }
        RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
        RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }

        // Vulkan-specific accessors
        VkDevice GetVkDevice() const noexcept { return m_Device; }
        VmaAllocator GetVmaAllocator() const noexcept { return m_VmaAllocator; }
        u32 GetGraphicsFamily() const noexcept { return m_GraphicsFamily; }
        u32 GetComputeFamily() const noexcept { return m_ComputeFamily; }
        u32 GetTransferFamily() const noexcept { return m_TransferFamily; }

    private:
        void PopulateCapabilities();
        void FindQueueFamilies();

        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_VmaAllocator = VK_NULL_HANDLE;
        RHICapabilities m_Caps;
        class VulkanAdapter* m_Adapter = nullptr;  // raw pointer (lifetime managed by RHIInstance)

        u32 m_GraphicsFamily = ~0u;
        u32 m_ComputeFamily  = ~0u;
        u32 m_TransferFamily = ~0u;

        std::unique_ptr<VulkanQueue> m_GraphicsQueue;
        std::unique_ptr<VulkanQueue> m_ComputeQueue;
        std::unique_ptr<VulkanQueue> m_TransferQueue;
    };
}
