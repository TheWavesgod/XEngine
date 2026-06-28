#pragma once

#include <XEngine/RHI/Native/VulkanNativeContext.h>

#include <imgui.h>
#include <volk.h>

namespace XEngine
{
    class RHIDevice;
    class RHISampler;
    class RHITextureView;

    class ImGuiVulkanBackend
    {
    public:
        bool Initialize(RHIDevice& device);
        void Shutdown();
        void RenderDrawData();
        ImTextureID RegisterTexture(RHISampler& sampler, RHITextureView& textureView);
        void UnregisterTexture(ImTextureID textureId);

        bool IsInitialized() const { return m_Initialized; }

    private:
        // ImGui owns this descriptor pool separately from runtime renderer
        // descriptor allocation so editor UI resources do not affect materials.
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        RHIDevice* m_RHIDevice = nullptr;
        bool m_Initialized = false;
    };
}
