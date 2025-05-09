#include "Vulkan/GlfwGeneral.hpp"
#include "Vulkan/Synchronization.h"
#include "Vulkan/Command.h"
#include "Vulkan/EasyVulkan.hpp"
#include "Vulkan/Pipeline.h"

using namespace VK;

PipelineLayout pipelineLayout_triangle;
Pipeline pipeline_triangle;

struct vertex
{
    glm::vec2 position;
    glm::vec4 color;
};

const auto& RenderPassAndFramebuffers()
{
    static const auto& rpwf = EasyVulkan::CreateRpwf_Screen();
    return rpwf;
}

void CreateLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);
}

void CreatePipeline()
{
    static ShaderModule vert("../shaders/VertexBuffer.vert.spv");
    static ShaderModule frag("../shaders/VertexBuffer.frag.spv");
    static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
        vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
        frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    
    auto Create = []()
    {
        GraphicsPipelineCreateInfoPack pipelineCreateInfoPack = {};

        //数据来自0号顶点缓冲区，输入频率是逐顶点输入
        pipelineCreateInfoPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
        //location为0，数据来自0号顶点缓冲区，vec2对应VK_FORMAT_R32G32_SFLOAT，用offsetof计算position在vertex中的起始位置
        pipelineCreateInfoPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, position));
        //location为1，数据来自0号顶点缓冲区，vec4对应VK_FORMAT_R32G32B32A32_SFLOAT，用offsetof计算color在vertex中的起始位置
        pipelineCreateInfoPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, color));
        
        pipelineCreateInfoPack.createInfo.layout = pipelineLayout_triangle;
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

int main()
{
    if (!InitializeWindow({1280, 720})) return -1;

    const auto& [renderPass, framebuffers] = EasyVulkan::CreateRpwf_Screen();
    CreateLayout();
    CreatePipeline();
    
    Fence fence(VK_FENCE_CREATE_SIGNALED_BIT);
    Semaphore semaphore_imageIsAvailable;
    Semaphore semaphore_renderingIsOver;

    CommandBuffer commandBuffer;
    CommandPool commandPool(VK::VkBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandPool.AllocateBuffers(commandBuffer);

    VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

    vertex vertices[] = {
        { { -.5f, -.5f }, { 1, 1, 0, 1 } },
        { {  .5f, -.5f }, { 1, 0, 0, 1 } },
        { { -.5f,  .5f }, { 0, 1, 0, 1 } },
        { {  .5f,  .5f }, { 0, 0, 1, 1 } }
    };
    VertexBuffer vertexBuffer(sizeof vertices);
    vertexBuffer.TransferData(vertices);

    uint16_t indices[] = {
        0, 1, 2,
        1, 2, 3
    };
    IndexBuffer indexBuffer(sizeof indices);
    indexBuffer.TransferData(indices);

    while (!glfwWindowShouldClose(pWindow))
    {
        while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED)) glfwWaitEvents();
        
        VkBase::Base().SwapImage(semaphore_imageIsAvailable);
        uint32_t i = VkBase::Base().CurrentImageIndex();

        
        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);

        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
        }
        
        renderPass.CmdEnd(commandBuffer);
        commandBuffer.End();

        VkBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
        VkBase::Base().PresentImage(semaphore_renderingIsOver);

        TitleFps();
        glfwPollEvents();

        fence.WaitAndReset();
    }

    TerminateWindow();
    return 0;
}