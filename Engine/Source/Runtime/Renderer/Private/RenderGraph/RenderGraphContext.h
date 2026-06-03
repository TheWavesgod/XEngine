#pragma once

namespace XEngine
{
    class RHIDevice;

    class RenderGraphContext
    {
    public:
        explicit RenderGraphContext(RHIDevice& device);

        RHIDevice& GetDevice();

    private:
        RHIDevice* m_Device = nullptr;
    };
}
