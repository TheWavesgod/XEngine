#include <XEngine/Renderer/RenderSystem.h>

#include "Mesh/PrimitiveMeshes.h"
#include "Mesh/StaticMesh.h"
#include "Passes/ClearPass.h"
#include "Passes/ForwardMeshPass.h"
#include "Passes/PresentPass.h"
#include "Passes/TrianglePass.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphContext.h"
#include "Resources/TextureManager.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/Vector.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHISystem.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderSystem.h>

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace XEngine
{
    namespace
    {
        constexpr f32 Pi = 3.14159265358979323846f;

        Matrix4 Identity()
        {
            Matrix4 result {};
            result.Values[0] = 1.0f;
            result.Values[5] = 1.0f;
            result.Values[10] = 1.0f;
            result.Values[15] = 1.0f;
            return result;
        }

        Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs)
        {
            Matrix4 result {};
            for (u32 row = 0; row < 4; ++row)
            {
                for (u32 column = 0; column < 4; ++column)
                {
                    result.Values[column * 4 + row] =
                        lhs.Values[0 * 4 + row] * rhs.Values[column * 4 + 0] +
                        lhs.Values[1 * 4 + row] * rhs.Values[column * 4 + 1] +
                        lhs.Values[2 * 4 + row] * rhs.Values[column * 4 + 2] +
                        lhs.Values[3 * 4 + row] * rhs.Values[column * 4 + 3];
                }
            }
            return result;
        }

        Vector3 Subtract(const Vector3& lhs, const Vector3& rhs)
        {
            return { lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z };
        }

        Vector3 Cross(const Vector3& lhs, const Vector3& rhs)
        {
            return {
                lhs.Y * rhs.Z - lhs.Z * rhs.Y,
                lhs.Z * rhs.X - lhs.X * rhs.Z,
                lhs.X * rhs.Y - lhs.Y * rhs.X
            };
        }

        f32 Dot(const Vector3& lhs, const Vector3& rhs)
        {
            return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
        }

        Vector3 Normalize(const Vector3& value)
        {
            const f32 length = std::sqrt(Dot(value, value));
            if (length <= 0.0f)
            {
                return {};
            }
            return { value.X / length, value.Y / length, value.Z / length };
        }

        Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
        {
            const Vector3 forward = Normalize(Subtract(target, eye));
            const Vector3 right = Normalize(Cross(forward, up));
            const Vector3 cameraUp = Cross(right, forward);

            Matrix4 result = Identity();
            result.Values[0] = right.X;
            result.Values[1] = cameraUp.X;
            result.Values[2] = -forward.X;
            result.Values[4] = right.Y;
            result.Values[5] = cameraUp.Y;
            result.Values[6] = -forward.Y;
            result.Values[8] = right.Z;
            result.Values[9] = cameraUp.Z;
            result.Values[10] = -forward.Z;
            result.Values[12] = -Dot(right, eye);
            result.Values[13] = -Dot(cameraUp, eye);
            result.Values[14] = Dot(forward, eye);
            return result;
        }

        Matrix4 Perspective(f32 fovRadians, f32 aspect, f32 nearPlane, f32 farPlane)
        {
            const f32 tanHalfFov = std::tan(fovRadians * 0.5f);
            Matrix4 result {};
            result.Values[0] = 1.0f / (aspect * tanHalfFov);
            result.Values[5] = -1.0f / tanHalfFov;
            result.Values[10] = farPlane / (nearPlane - farPlane);
            result.Values[11] = -1.0f;
            result.Values[14] = -(farPlane * nearPlane) / (farPlane - nearPlane);
            return result;
        }

        RHIShaderDesc MakeRHIShaderDesc(const CompiledShader& shader, const char* debugName)
        {
            RHIShaderDesc desc;
            desc.Stage = shader.Stage;
            desc.Target = shader.Target;
            desc.Format = shader.Format;
            desc.EntryPoint = "main";
            desc.Code = shader.Bytecode.data();
            desc.CodeSize = shader.Bytecode.size();
            desc.DebugName = debugName;
            return desc;
        }
    }

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

        ShaderSystem* shaderSystem = context.Engine->GetSubsystemManager().GetSubsystem<ShaderSystem>();
        XENGINE_ASSERT(shaderSystem != nullptr, "RenderSystem requires ShaderSystem for Stage 4B");
        if (shaderSystem == nullptr || !shaderSystem->IsCompilerAvailable())
        {
            XENGINE_LOG_ERROR("RenderSystem requires an available ShaderSystem");
            return;
        }

        RHIDevice* device = m_RHISystem->GetDevice();
        XENGINE_ASSERT(device != nullptr, "RenderSystem requires a valid RHIDevice");
        if (device == nullptr || !device->IsValid())
        {
            XENGINE_LOG_ERROR("RenderSystem requires a valid RHIDevice");
            return;
        }

        m_TextureManager = std::make_unique<TextureManager>();
        m_TextureManager->Initialize(device);

        const std::string checkerPath = "Assets/Textures/checker.png";
        if (std::filesystem::exists(checkerPath))
        {
            m_TextureManager->LoadTexture2D(checkerPath, true);
        }
        else
        {
            XENGINE_LOG_WARN("Assets/Textures/checker.png not found; using default texture validation only.");
        }

        m_CubeMesh = std::make_unique<StaticMesh>(CreateHardcodedCubeMesh(*device));
        if (!m_CubeMesh->VertexBuffer || !m_CubeMesh->IndexBuffer)
        {
            XENGINE_LOG_ERROR("Failed to create hardcoded cube mesh buffers");
            return;
        }

        ShaderCompileDesc vertexDesc;
        vertexDesc.Path = "Engine/Shaders/Passes/MeshForward.slang";
        vertexDesc.EntryPoint = "vertexMain";
        vertexDesc.Stage = ShaderStage::Vertex;
        vertexDesc.Target = ShaderTarget::VulkanSPIRV;
        vertexDesc.GenerateDebugInfo = true;
        vertexDesc.EnableOptimization = false;

        XENGINE_LOG_INFO("Compiling MeshForward.slang vertexMain");
        CompiledShader vertexShader = shaderSystem->Compile(vertexDesc);
        if (!vertexShader.IsValid())
        {
            XENGINE_LOG_ERROR(vertexShader.Diagnostics.empty() ? "MeshForward vertex shader compilation failed" :
                                                                  vertexShader.Diagnostics);
            return;
        }

        ShaderCompileDesc fragmentDesc;
        fragmentDesc.Path = "Engine/Shaders/Passes/MeshForward.slang";
        fragmentDesc.EntryPoint = "fragmentMain";
        fragmentDesc.Stage = ShaderStage::Fragment;
        fragmentDesc.Target = ShaderTarget::VulkanSPIRV;
        fragmentDesc.GenerateDebugInfo = true;
        fragmentDesc.EnableOptimization = false;

        XENGINE_LOG_INFO("Compiling MeshForward.slang fragmentMain");
        CompiledShader fragmentShader = shaderSystem->Compile(fragmentDesc);
        if (!fragmentShader.IsValid())
        {
            XENGINE_LOG_ERROR(fragmentShader.Diagnostics.empty() ? "MeshForward fragment shader compilation failed" :
                                                                    fragmentShader.Diagnostics);
            return;
        }

        m_MeshVertexShader = device->CreateShader(MakeRHIShaderDesc(vertexShader, "MeshForward vertex"));
        if (!m_MeshVertexShader)
        {
            XENGINE_LOG_ERROR("Failed to create MeshForward vertex RHI shader");
            return;
        }

        m_MeshFragmentShader = device->CreateShader(MakeRHIShaderDesc(fragmentShader, "MeshForward fragment"));
        if (!m_MeshFragmentShader)
        {
            XENGINE_LOG_ERROR("Failed to create MeshForward fragment RHI shader");
            return;
        }

        RHIGraphicsPipelineDesc pipelineDesc;
        pipelineDesc.VertexShader = m_MeshVertexShader.get();
        pipelineDesc.FragmentShader = m_MeshFragmentShader.get();
        pipelineDesc.ColorFormat = device->GetSwapchainFormat();
        pipelineDesc.DepthFormat = RHIFormat::D32Float;
        pipelineDesc.EnableDepthTest = true;
        pipelineDesc.EnableDepthWrite = true;
        pipelineDesc.VertexLayout.Stride = sizeof(MeshVertex);
        pipelineDesc.VertexLayout.Attributes = {
            RHIVertexAttributeDesc { 0, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Position)) },
            RHIVertexAttributeDesc { 1, RHIFormat::R32G32B32Float, static_cast<u32>(offsetof(MeshVertex, Color)) }
        };
        pipelineDesc.PushConstantSize = sizeof(Matrix4);
        pipelineDesc.PushConstantStages = ShaderStage::Vertex;
        pipelineDesc.DebugName = "Creating mesh forward graphics pipeline";

        m_MeshPipeline = device->CreateGraphicsPipeline(pipelineDesc);
        if (!m_MeshPipeline)
        {
            XENGINE_LOG_ERROR("Failed to create MeshForward graphics pipeline");
            return;
        }

        m_Model = Identity();
        const Matrix4 view = LookAt({ 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        const Matrix4 projection = Perspective(60.0f * Pi / 180.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
        m_ModelViewProjection = Multiply(projection, Multiply(view, m_Model));

        m_Initialized = true;
    }

    void RenderSystem::OnDestroy()
    {
        if (m_Initialized)
        {
            XENGINE_LOG_INFO("Destroying RenderSystem");
        }

        if (m_RHISystem != nullptr)
        {
            RHIDevice* device = m_RHISystem->GetDevice();
            if (device != nullptr && device->IsValid())
            {
                device->WaitIdle();
            }
        }

        m_MeshPipeline.reset();
        m_MeshFragmentShader.reset();
        m_MeshVertexShader.reset();
        m_CubeMesh.reset();
        if (m_TextureManager)
        {
            m_TextureManager->Shutdown();
            m_TextureManager.reset();
        }
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

        RHICommandList* commandList = device->BeginFrame();

        RHIColor clearColor;
        clearColor.R = 0.1f;
        clearColor.G = 0.1f;
        clearColor.B = 0.15f;
        clearColor.A = 1.0f;

        RenderGraph graph;
        graph.Clear();
        AddClearPass(graph, clearColor);
        std::vector<RenderObject> objects;
        RenderObject cube;
        cube.Mesh = m_CubeMesh.get();
        cube.Model = m_Model;
        cube.ModelViewProjection = m_ModelViewProjection;
        cube.ObjectId = 1;
        cube.MeshId = 1;
        cube.MaterialId = 0;
        objects.push_back(cube);
        AddForwardMeshPass(graph, m_MeshPipeline.get(), objects);
        AddPresentPass(graph);
        graph.Compile();

        RenderGraphContext context(*device, commandList);
        graph.Execute(context);

        device->EndFrame();
    }
}
