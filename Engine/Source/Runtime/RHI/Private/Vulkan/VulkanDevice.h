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
        std::shared_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) override;
        std::shared_ptr<RHIBuffer> CreateBuffer(
            const RHIBufferDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) override;
        std::shared_ptr<RHITexture> CreateTexture(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) override;
        std::shared_ptr<RHISampler> CreateSampler(
            const RHISamplerDesc& desc) override;
        std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayout(
            const RHIBindGroupLayoutDesc& desc) override;
        std::shared_ptr<RHIBindGroup> CreateBindGroup(
            const RHIBindGroupDesc& desc) override;
        std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) override;
        RHIFormat GetSwapchainFormat() const override;
        void WaitIdle() override;

    private:
        bool PickPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateDescriptorPool();
        void DestroyDescriptorPool();
        bool CreateDepthTexture();
        void DestroyDepthTexture();
        void RecreateSwapchain(u32 width, u32 height);
        void ImmediateSubmit(const std::function<void(VkCommandBuffer)>& function);

        VulkanInstance m_Instance;
        VulkanSurface m_Surface;
        VulkanAllocator m_Allocator;
        VulkanSwapchain m_Swapchain;
        VulkanFrameResources m_FrameResources;
        VulkanCommandList m_CommandList;
        std::unique_ptr<VulkanTexture> m_DepthTexture;

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
