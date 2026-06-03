#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RenderGraph;

    void AddClearPass(RenderGraph& graph, const RHIColor& clearColor);
}
