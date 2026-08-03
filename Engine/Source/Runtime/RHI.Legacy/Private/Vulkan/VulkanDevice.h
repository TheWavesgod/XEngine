#pragma once

#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/RHI/RHIDevice.h>

#include "VulkanAllocator.h"
#include "VulkanCommandList.h"
#include "VulkanDescriptor.h"
#include "VulkanFrameResources.h"
#include "VulkanInstance.h"
#include "VulkanQueue.h"
#include "VulkanSampler.h"
#include "VulkanSurface.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"

#include <volk.h>

#include <functional>
#include <memory>

namespace XEngine
{
    class RHIResourceFactory; 
    class RHISampler;
    class RHITexture;
    class RHITextureView;
    class RHIUploadManager;  

    struct VulkanDeviceCreateInfo
    {
        NativeWindowHandle NativeWindow;
        u32 Width = 1280;
        u32 Height = 720;
        bool EnableValidation = true;
        bool EnableVSync = true;
    };

    class VulkanDevice final : public RHIDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice() override;

        bool Initialize(const VulkanDeviceCreateInfo& createInfo);
        void Shutdown();

        RHIBackend GetBackend() const override;
        RHIClipSpaceConvention GetClipSpaceConvention() const override;
        bool IsValid() const override;
        RHICommandList* BeginFrame() override;
        void ClearSwapchain(const RHIColor& color) override;
        void EndFrame() override;
        void RequestResize(u32 width, u32 height) override;
        
        RHIFormat GetSwapchainFormat() const override;
        bool GetVulkanNativeContext(VulkanNativeContext& outContext) const override;
        bool GetVulkanNativeTextureBinding(
            const RHISampler& sampler,
            const RHITextureView& textureView,
            VulkanNativeTextureBinding& outBinding) const override;
        void RenderVulkanOverlay(const std::function<void(RHINativeCommandBuffer)>& callback) override;
        void WaitIdle() override;

        // NEW: accessors used by VulkanResourceFactory and VulkanUploadManager.
        VmaAllocator GetVmaAllocator() const { return m_Allocator.GetHandle(); }
        VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

        // Immediate submit is now used by both VulkanResourceFactory (Stage 3
        // texture upload) and VulkanUploadManager (Stage 4).
        void ImmediateSubmit(const std::function<void(VkCommandBuffer)>& function);

        RHIResourceFactory& GetResourceFactory() override;
        const RHIResourceFactory& GetResourceFactory() const override;

        RHIUploadManager& GetUploadManager() override;
        const RHIUploadManager& GetUploadManager() const override;
        const RHICapabilities& GetCapabilities() const override;

        inline VkDevice GetHandle() const { return m_Device; }

    private:
        bool PickPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateDescriptorPool();
        void DestroyDescriptorPool();
        bool CreateDepthTexture();
        void DestroyDepthTexture();
        void RecreateSwapchain(u32 width, u32 height);

        VulkanInstance m_Instance;
        VulkanSurface m_Surface;
        VulkanAllocator m_Allocator;
        VulkanSwapchain m_Swapchain;
        VulkanFrameResources m_FrameResources;
        VulkanCommandList m_CommandList;
        std::unique_ptr<RHIResourceFactory> m_ResourceFactory;
        std::unique_ptr<RHIUploadManager> m_UploadManager;

        std::shared_ptr<RHITexture> m_DepthTexture;
        std::shared_ptr<RHITextureView> m_DepthTextureView;
        RHICapabilities m_Capabilities {};

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VulkanQueue m_GraphicsQueue;
        VulkanQueue m_PresentQueue;

        u32 m_GraphicsFamilyIndex = 0;
        u32 m_PresentFamilyIndex = 0;
        u32 m_CurrentImageIndex = 0;
        u32 m_PendingResizeWidth = 0;
        u32 m_PendingResizeHeight = 0;
        VkImageLayout m_CurrentSwapchainImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        bool m_EnableVSync = true;
        bool m_FrameActive = false;
        bool m_ResizeRequested = false;
        bool m_Initialized = false;
    };
}
