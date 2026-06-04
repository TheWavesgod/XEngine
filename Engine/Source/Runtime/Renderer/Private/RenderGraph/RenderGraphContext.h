#pragma once

namespace XEngine
{
    class RHICommandList;
    class RHIDevice;

    class RenderGraphContext
    {
    public:
        RenderGraphContext(RHIDevice& device, RHICommandList* commandList);

        RHIDevice& GetDevice();
        RHICommandList* GetCommandList();

    private:
        RHIDevice* m_Device = nullptr;
        RHICommandList* m_CommandList = nullptr;
    };
}
