#include <XEngine/Renderer/RenderSystem.h>

#include <XEngine/Renderer/RenderScene.h>

namespace XEngine
{
    void RenderSystem::BeginFrame() {}
    void RenderSystem::Submit(const RenderScene& scene) { (void)scene; }
    void RenderSystem::Render() {}
    void RenderSystem::EndFrame() {}
}

