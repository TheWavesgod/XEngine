#include "RenderGraph.h"

#include "RenderGraphBuilder.h"

#include <XEngine/Logging/Log.h>

#include <string>

namespace XEngine
{
    void RenderGraph::AddPass(const RenderGraphPassDesc& desc, SetupFunc setup, ExecuteFunc execute)
    {
        RenderGraphPass pass;
        pass.Desc = desc;
        pass.Setup = std::move(setup);
        pass.Execute = std::move(execute);

        m_Passes.emplace_back(std::move(pass));
        m_Compiled = false;
    }

    void RenderGraph::Clear()
    {
        m_Passes.clear();
        m_Compiled = false;
    }

    void RenderGraph::Compile()
    {
        RenderGraphBuilder builder;
        for (RenderGraphPass& pass : m_Passes)
        {
            if (pass.Setup)
            {
                pass.Setup(builder);
            }
        }

        std::string message = "RenderGraph compile: ";
        message += std::to_string(m_Passes.size());
        message += " passes";
        XENGINE_LOG_DEBUG(message);

        // TODO: future stages may compile passes through JobSystem.
        m_Compiled = true;
    }

    void RenderGraph::Execute(RenderGraphContext& context)
    {
        if (!m_Compiled)
        {
            Compile();
        }

        for (RenderGraphPass& pass : m_Passes)
        {
            std::string message = "Executing RenderGraph pass: ";
            message += pass.Desc.Name;
            XENGINE_LOG_DEBUG(message);

            if (pass.Execute)
            {
                pass.Execute(context);
            }
        }
    }

    bool RenderGraph::IsCompiled() const
    {
        return m_Compiled;
    }

    std::size_t RenderGraph::GetPassCount() const
    {
        return m_Passes.size();
    }
}
