#include "ImGuiVulkanBackend.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <string>

namespace XEngine
{
    namespace
    {
        void CheckVkResult(VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                XENGINE_LOG_ERROR(std::string("ImGui Vulkan backend error: ") + std::to_string(result));
            }
        }
    }

    bool ImGuiVulkanBackend::Initialize(RHIDevice& device)
    {
        if (m_Initialized)
        {
            return true;
        }

        VulkanNativeContext context;
        if (!device.GetVulkanNativeContext(context))
        {
            XENGINE_LOG_ERROR("ImGui Vulkan backend requires a Vulkan RHIDevice");
            return false;
        }

        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 64 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 16 }
        };

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 144;
        poolInfo.poolSizeCount = static_cast<u32>(std::size(poolSizes));
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(context.Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to create ImGui Vulkan descriptor pool");
            return false;
        }

        VkPipelineRenderingCreateInfoKHR pipelineRendering {};
        pipelineRendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRendering.colorAttachmentCount = 1;
        pipelineRendering.pColorAttachmentFormats = &context.ColorFormat;
        pipelineRendering.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        pipelineRendering.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        ImGui_ImplVulkan_InitInfo initInfo {};
        initInfo.ApiVersion = VK_API_VERSION_1_3;
        initInfo.Instance = context.Instance;
        initInfo.PhysicalDevice = context.PhysicalDevice;
        initInfo.Device = context.Device;
        initInfo.QueueFamily = context.GraphicsQueueFamilyIndex;
        initInfo.Queue = context.GraphicsQueue;
        initInfo.DescriptorPool = m_DescriptorPool;
        initInfo.MinImageCount = context.MinImageCount;
        initInfo.ImageCount = context.ImageCount;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRendering;
        initInfo.UseDynamicRendering = true;
        initInfo.CheckVkResultFn = CheckVkResult;

        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            XENGINE_LOG_ERROR("Failed to initialize ImGui Vulkan backend");
            vkDestroyDescriptorPool(context.Device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
            return false;
        }

        m_Device = context.Device;
        m_RHIDevice = &device;
        m_Initialized = true;
        XENGINE_LOG_INFO("ImGui Vulkan backend initialized");
        return true;
    }

    void ImGuiVulkanBackend::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplVulkan_Shutdown();
        if (m_Device != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
        }

        m_DescriptorPool = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_RHIDevice = nullptr;
        m_Initialized = false;
    }

    void ImGuiVulkanBackend::RenderDrawData()
    {
        if (!m_Initialized || m_RHIDevice == nullptr)
        {
            return;
        }

        ImDrawData* drawData = ImGui::GetDrawData();
        m_RHIDevice->RenderVulkanOverlay(
            [drawData](RHINativeCommandBuffer commandBuffer)
            {
                ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(commandBuffer));
            });
    }

    ImTextureID ImGuiVulkanBackend::RegisterTexture(RHISampler& sampler, RHITexture& texture)
    {
        VkSampler vkSampler = static_cast<VkSampler>(sampler.GetNativeSampler(RHIBackend::Vulkan));
        VkImageView imageView = static_cast<VkImageView>(texture.GetNativeImageView(RHIBackend::Vulkan));
        if (!m_Initialized ||
            vkSampler == VK_NULL_HANDLE ||
            imageView == VK_NULL_HANDLE)
        {
            return ImTextureID_Invalid;
        }

        // Editor-private bridge from RHI resources to ImGui's Vulkan texture
        // descriptor. Runtime renderer never includes ImGui headers.
        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(
            vkSampler,
            imageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return reinterpret_cast<ImTextureID>(descriptorSet);
    }

    void ImGuiVulkanBackend::UnregisterTexture(ImTextureID textureId)
    {
        if (!m_Initialized || textureId == ImTextureID_Invalid)
        {
            return;
        }

        ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(textureId));
    }
}
