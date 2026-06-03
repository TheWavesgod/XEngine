#pragma once

#include "RenderGraphPass.h"

#include <cstddef>
#include <vector>

namespace XEngine
{
    class RenderGraph
    {
    public:
        using SetupFunc = std::function<void(RenderGraphBuilder&)>;
        using ExecuteFunc = std::function<void(RenderGraphContext&)>;

        void AddPass(const RenderGraphPassDesc& desc, SetupFunc setup, ExecuteFunc execute);

        void Clear();
        void Compile();
        void Execute(RenderGraphContext& context);

        bool IsCompiled() const;
        std::size_t GetPassCount() const;

    private:
        std::vector<RenderGraphPass> m_Passes;
        bool m_Compiled = false;
    };
}
