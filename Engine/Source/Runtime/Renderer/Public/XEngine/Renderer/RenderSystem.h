#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Math/Matrix.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/Material.h>
#include <XEngine/Renderer/Mesh.h>
#include <XEngine/Renderer/RenderScene.h>

#include <memory>

namespace XEngine
{
    class AssetSystem;
    class RHIShader;
    class RHISystem;
    class SceneSystem;
    class MaterialSystem;
    class RenderMeshManager;
    class TextureManager;
    class ForwardPipeline;
    class RenderPipelineCache;

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
        AssetSystem* m_AssetSystem = nullptr;
        SceneSystem* m_SceneSystem = nullptr;

        std::unique_ptr<TextureManager> m_TextureManager;
        std::unique_ptr<MaterialSystem> m_MaterialSystem;
        std::unique_ptr<RenderMeshManager> m_RenderMeshManager;
        
        std::unique_ptr<RenderPipelineCache> m_RenderPipelineCache;

        std::shared_ptr<RHIShader> m_MeshVertexShader;
        std::shared_ptr<RHIShader> m_MeshFragmentShader;
        std::shared_ptr<RHIPipeline> m_MeshPipeline;

        std::unique_ptr<ForwardPipeline> m_ForwardPipeline;
        
        RenderScene m_RenderScene;
        
        Mat4 m_ViewProjection { 1.0f };
        float m_AspectRatio = 16.0f / 9.0f;
        
        bool m_Initialized = false;
    };
}
