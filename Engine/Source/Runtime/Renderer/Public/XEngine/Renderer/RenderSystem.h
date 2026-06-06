#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Math/Matrix.h>

#include <memory>

namespace XEngine
{
    class RHIPipeline;
    class RHIShader;
    class RHISystem;
    class StaticMesh;
    class TextureManager;

    class RenderSystem final : public ISubsystem
    {
    public:
        RenderSystem();
        ~RenderSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

    private:
        void Render();

        RHISystem* m_RHISystem = nullptr;
        std::unique_ptr<TextureManager> m_TextureManager;
        std::unique_ptr<StaticMesh> m_CubeMesh;
        std::shared_ptr<RHIShader> m_MeshVertexShader;
        std::shared_ptr<RHIShader> m_MeshFragmentShader;
        std::shared_ptr<RHIPipeline> m_MeshPipeline;
        Matrix4 m_Model {};
        Matrix4 m_ModelViewProjection {};
        bool m_Initialized = false;
    };
}
