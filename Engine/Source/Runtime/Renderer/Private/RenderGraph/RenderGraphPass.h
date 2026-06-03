#pragma once

#include <functional>
#include <string>

namespace XEngine
{
    class RenderGraphBuilder;
    class RenderGraphContext;

    enum class RenderGraphPassType
    {
        Graphics,
        Compute,
        Transfer,
        Present,
        External
    };

    struct RenderGraphPassDesc
    {
        std::string Name;
        RenderGraphPassType Type = RenderGraphPassType::Graphics;
    };

    struct RenderGraphPass
    {
        RenderGraphPassDesc Desc;

        std::function<void(RenderGraphBuilder&)> Setup;
        std::function<void(RenderGraphContext&)> Execute;
    };
}
