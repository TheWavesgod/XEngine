#include "Vulkan/GlfwGeneral.hpp"
#include "Vulkan/Synchronization.h"
#include "Vulkan/Command.h"
#include "Vulkan/EasyVulkan.hpp"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Descriptor.h"

using namespace VK;

PipelineLayout pipelineLayout_triangle;
Pipeline pipeline_triangle;

DescriptorSetLayout descriptorSetLayout_texture;
PipelineLayout pipelineLayout_texture;

struct vertex
{
    glm::vec2 position;
    glm::vec2 texCoord;
    glm::vec4 color;
};

const auto& RenderPassAndFramebuffers()
{
    static const auto& rpwf = EasyVulkan::CreateRpwf_Screen();
    return rpwf;
}

void CreateLayout()
{ 
    VkPushConstantRange pushConstantRange = {
        VK_SHADER_STAGE_VERTEX_BIT,
        0,  // offset
        24
    };

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_texture = {
        .binding = 0,                                                
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
        .descriptorCount = 1,                                        
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT                   
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_texture = {
        .bindingCount = 1,
        .pBindings = &descriptorSetLayoutBinding_texture
    };
    descriptorSetLayout_texture.Create(descriptorSetLayoutCreateInfo_texture);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .setLayoutCount = 0,
        .pSetLayouts = descriptorSetLayout_texture.Address()
    };
    pipelineLayout_texture.Create(pipelineLayoutCreateInfo);

    /*VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);*/
}

void CreatePipeline()
{
    static ShaderModule vert("FirstTriangle.vert");
    static ShaderModule frag("FirstTriangle.frag");

    static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
        vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
        frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    
    auto Create = []()
    {
        GraphicsPipelineCreateInfoPack pipelineCreateInfoPack = {};

        // Data comes from layout 0 vertex buffer, input per vertex
        pipelineCreateInfoPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
        // location is 0, data comes from layout 0，vec2 referes to VK_FORMAT_R32G32_SFLOAT, use offsetof to caculate the start position in struct vertex
        pipelineCreateInfoPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, position));
        pipelineCreateInfoPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, texCoord));
        // location is 2, vec4 refers to VK_FORMAT_R32G32B32A32_SFLOAT
        pipelineCreateInfoPack.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, color));
        
        pipelineCreateInfoPack.createInfo.layout = pipelineLayout_texture;
        pipelineCreateInfoPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
        pipelineCreateInfoPack.inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineCreateInfoPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
        pipelineCreateInfoPack.scissors.emplace_back(VkOffset2D{}, windowSize);
        pipelineCreateInfoPack.multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineCreateInfoPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
        pipelineCreateInfoPack.UpdateAllArrays();
        pipelineCreateInfoPack.createInfo.stageCount = 2;
        pipelineCreateInfoPack.createInfo.pStages = shaderStageCreateInfos_triangle;
        
        pipeline_triangle.Create(pipelineCreateInfoPack);
    };

    auto Destroy = []()
    {
        pipeline_triangle.~Pipeline();
    };

    VkBase::Base().AddCallback_CreateSwapchain(Create);
    VkBase::Base().AddCallback_DestroySwapchain(Destroy);

    Create();
}

int main1()
{
    if (!InitializeWindow({1280, 720})) return -1;

    EasyVulkan::BootScreen("../Resources/Images/StartupImage.png", VK_FORMAT_R8G8B8A8_UNORM);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    const auto& [renderPass, framebuffers] = EasyVulkan::CreateRpwf_Screen();

    CreateLayout();
    CreatePipeline();
    
    Fence fence(VK_FENCE_CREATE_SIGNALED_BIT);
    Semaphore semaphore_imageIsAvailable;
    Semaphore semaphore_renderingIsOver;

    CommandBuffer commandBuffer;
    CommandPool commandPool(VK::VkBase::Base().PhysicalDevice().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandPool.AllocateBuffers(commandBuffer);

    VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

    vertex vertices[] = {
        { { -.5f, -.5f }, {0, 0}, { 1, 1, 0, 1 } },
        { {  .5f, -.5f }, {1, 0}, { 1, 0, 0, 1 } },
        { { -.5f,  .5f }, {0, 1}, { 0, 1, 0, 1 } },
        { {  .5f,  .5f }, {1, 1}, { 0, 0, 1, 1 } }
    };
    VertexBuffer vertexBuffer(sizeof vertices);
    vertexBuffer.TransferData(vertices);

    uint16_t indices[] = {
        0, 1, 2,
        1, 2, 3
    };
    IndexBuffer indexBuffer(sizeof indices);
    indexBuffer.TransferData(indices);

    glm::vec2 pushConstants[] = {
        {  .0f, .0f },
        { -.5f, .0f },
        {  .5f, .0f }
    };

    Texture2d texture("../Resources/Images/StartupImage.png", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
    VkSamplerCreateInfo samplerCreatInfo = Texture::MakeSamplerCreateInfo();
    Sampler sampler(samplerCreatInfo);

    VkDescriptorPoolSize descriptorPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };
    DescriptorPool descriptorPool(1, descriptorPoolSizes);
    DescriptorSet descriptorSet_texture;
    descriptorPool.AllocateSets(descriptorSet_texture, descriptorSetLayout_texture);
    VkDescriptorImageInfo imageInfo = {
        .sampler = sampler,
        .imageView = texture.ImageViewRef(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    descriptorSet_texture.Write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    while (!glfwWindowShouldClose(pWindow))
    {
        while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED)) glfwWaitEvents();
        
        VkBase::Base().Swapchain().SwapImage(semaphore_imageIsAvailable);
        uint32_t i = VkBase::Base().Swapchain().CurrentImageIndex();

        
        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);

        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);  
            //vkCmdPushConstants(commandBuffer, pipelineLayout_triangle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof pushConstants, &pushConstants);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout_texture, 0, 1, descriptorSet_texture.Address(), 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
        }
        
        renderPass.CmdEnd(commandBuffer);
        commandBuffer.End();

        VkBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
        VkBase::Base().Swapchain().PresentImage(semaphore_renderingIsOver);

        TitleFps();
        glfwPollEvents();

        fence.WaitAndReset();
    }

    TerminateWindow();
    return 0;
}