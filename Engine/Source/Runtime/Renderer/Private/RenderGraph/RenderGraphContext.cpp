#include "RenderGraphContext.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
    RenderGraphContext::RenderGraphContext(RHIDevice& device, RHICommandList* commandList)
        : m_Device(&device)
        , m_CommandList(commandList)
    {
    }

    RHIDevice& RenderGraphContext::GetDevice()
    {
        XENGINE_ASSERT(m_Device != nullptr, "RenderGraphContext requires a valid RHIDevice");
        return *m_Device;
    }

    RHICommandList* RenderGraphContext::GetCommandList()
    {
        return m_CommandList;
    }
}
