#include <XEngine/Renderer/RenderSystem.h>

#include "Passes/ClearPass.h"
#include "Passes/PresentPass.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphContext.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHISystem.h>

namespace XEngine
{
    RenderSystem::RenderSystem() = default;

    RenderSystem::~RenderSystem()
    {
        OnDestroy();
    }

    void RenderSystem::OnCreate(const SubsystemContext& context)
    {
        XENGINE_LOG_INFO("Creating RenderSystem");

        XENGINE_ASSERT(context.Engine != nullptr, "RenderSystem requires a valid Engine");
        if (context.Engine == nullptr)
        {
            XENGINE_LOG_ERROR("RenderSystem requires a valid Engine");
            return;
        }

        m_RHISystem = context.Engine->GetSubsystemManager().GetSubsystem<RHISystem>();
        XENGINE_ASSERT(m_RHISystem != nullptr, "RenderSystem requires RHISystem");
        if (m_RHISystem == nullptr)
        {
            XENGINE_LOG_ERROR("RenderSystem requires RHISystem");
            return;
        }

        m_Initialized = true;
    }

    void RenderSystem::OnDestroy()
    {
        if (!m_Initialized)
        {
            return;
        }

        XENGINE_LOG_INFO("Destroying RenderSystem");
        m_RHISystem = nullptr;
        m_Initialized = false;
    }

    void RenderSystem::OnUpdate(float deltaTime)
    {
        (void)deltaTime;
        Render();
    }

    void RenderSystem::Render()
    {
        if (m_RHISystem == nullptr)
        {
            return;
        }

        RHIDevice* device = m_RHISystem->GetDevice();
        if (device == nullptr || !device->IsValid())
        {
            return;
        }

        device->BeginFrame();

        RHIColor clearColor;
        clearColor.R = 0.1f;
        clearColor.G = 0.1f;
        clearColor.B = 0.15f;
        clearColor.A = 1.0f;

        RenderGraph graph;
        graph.Clear();
        AddClearPass(graph, clearColor);
        AddPresentPass(graph);
        graph.Compile();

        RenderGraphContext context(*device);
        graph.Execute(context);

        device->EndFrame();
    }
}
