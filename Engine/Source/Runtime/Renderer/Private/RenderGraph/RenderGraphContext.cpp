#include "RenderGraphContext.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
    RenderGraphContext::RenderGraphContext(RHIDevice& device)
        : m_Device(&device)
    {
    }

    RHIDevice& RenderGraphContext::GetDevice()
    {
        XENGINE_ASSERT(m_Device != nullptr, "RenderGraphContext requires a valid RHIDevice");
        return *m_Device;
    }
}
