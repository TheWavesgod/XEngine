#include "RenderShadowManager.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/Renderer/RendererSettings.h>
#include <XEngine/Renderer/RendererDebugSettings.h>
#include <XEngine/Renderer/RenderScene.h>

#include "../Pipeline/RenderFrameContext.h"
#include "../ShaderInterop/GPUShadowTypes.h"

namespace XEngine
{
    void RenderShadowManager::Initialize(RHIDevice& device)
    {
        m_ResourceCache.Initialize(device);
        m_FrameData = {};
        m_FrozenFrameData = {};
        m_HasFrozenData = false;
        XENGINE_LOG_INFO("RenderShadowManager initialized");
    }

    void RenderShadowManager::Shutdown(RHIDevice& device)
    {
        m_ResourceCache.Shutdown();
        m_FrameData = {};
        m_FrozenFrameData = {};
        m_HasFrozenData = false;
    } 

    bool RenderShadowManager::HasDirectionalShadow() const
    {
        return m_HasFrozenData ? 
            m_FrozenFrameData.Directional.Enabled :
            m_FrameData.Directional.Enabled;
    }

    const RenderShadowFrameData& RenderShadowManager::GetFrameData() const
    {
        return m_HasFrozenData ? m_FrozenFrameData : m_FrameData;
    }

    void RenderShadowManager::PrepareFrame(
        RHIDevice& device,
        const RenderScene& scene,
        const RenderFrameContext& frame,
        const ShadowSettings& settings,
        const ShadowDebugSettings& debugSettings)
    {
        if (debugSettings.FreezeShadowMatrices)
        {
            if (!m_HasFrozenData)
            {
                PrepareDirectionalShadow(device, scene, frame, settings.Directional, debugSettings);
                m_FrozenFrameData = m_FrameData;
                m_HasFrozenData = true;
            }
            // else: keep using m_FrozenFrameData
            return;
        }
        else
        {
            m_HasFrozenData = false;
            m_FrozenFrameData = {};
        }

        PrepareDirectionalShadow(device, scene, frame, settings.Directional, debugSettings);
    }

    // TODO: Maybe need to move to the scene, and do it generally
    static AABB ComputeShadowCasterBounds(const RenderScene& scene)
    {
        AABB bounds;
        bounds.Min = Vec3( std::numeric_limits<float>::infinity());
        bounds.Max = Vec3(-std::numeric_limits<float>::infinity());
        bool any = false;
        for (const RenderObject& obj : scene.OpaqueObjects)
        {
            if (!obj.CastShadow) continue;
            if (obj.Mesh.IsValid() == false) continue;
            // RenderObject.WorldBounds is in world space; expand the AABB by
            // the object's local bounds transformed to world.
            bounds.Min = Math::Min(bounds.Min, obj.WorldBounds.Min);
            bounds.Max = Math::Max(bounds.Max, obj.WorldBounds.Max);
            any = true;
        }
        if (!any)
        {
            // Fall back to a 100m cube around the origin.
            bounds.Min = Vec3(-50, -50, -50);
            bounds.Max = Vec3( 50,  50,  50);
        }
        return bounds;
    }

    void RenderShadowManager::PrepareDirectionalShadow(
        RHIDevice& device,
        const RenderScene& scene,
        const RenderFrameContext& frame,
        const DirectionalShadowSettings& settings,
        const ShadowDebugSettings& debugSettings)
    {
        // Reset the per-frame data.
        m_FrameData = {};
        m_FrameData.Directional.Enabled = false;
        m_FrameData.Directional.CascadeCount = 0;

        // Master switch.
        if (!settings.Enabled || settings.Technique != DirectionalShadowTechnique::CascadedShadowMaps)
        {
            return;
        }

        // Find the first enabled directional light with CastShadow.
        const RenderLight* shadowLight = nullptr;
        for (const RenderLight& light : scene.Lights)
        {
            if (!light.Enabled) continue;
            if (light.Type != RenderLightType::Directional) continue;
            if (!light.CastShadow) continue;
            shadowLight = &light;
            break;
        }

        if (shadowLight = nullptr)
        {
            return;
        }

        // Acquire or recreate shadow resources.
        DirectionalShadowResourceDesc resDesc;
        resDesc.Resolution   = settings.Resolution;
        resDesc.CascadeCount = settings.CascadeCount;
        resDesc.DepthFormat  = RHIFormat::D32Float;
        resDesc.StorageMode  = settings.StorageMode;

        DirectionalShadowResources& res =
            m_ResourceCache.GetOrCreateDirectionalShadowResources(resDesc);

        if (!res.Texture || !res.SampledView || !res.Sampler)
        {
            XENGINE_LOG_WARN("Shadow resources unavailable; disabling shadows this frame");
            return;
        }
        for (u32 i = 0; i < res.CascadeCount; ++i)
        {
            if (!res.LayerDepthViews[i])
            {
                XENGINE_LOG_WARN("Shadow per-layer view missing; disabling shadows this frame");
                return;
            }
        }

        // Build the per-cascade matrices via the planner.
        DirectionalShadowPlanDesc planDesc;
        planDesc.Light             = shadowLight;
        planDesc.CameraView        = frame.ViewMatrix;
        planDesc.CameraProjection  = frame.ProjectionMatrix;
        planDesc.CameraNear        = /* per-camera near plane; */ 0.1f;     // TODO: Pass Camera info here
        planDesc.CameraFar         = /* per-camera far plane; */ 1000.0f;
        planDesc.CameraPosition    = frame.CameraWorldPosition;
        planDesc.SceneBounds       = ComputeShadowCasterBounds(scene); 
        planDesc.CascadeCount      = settings.CascadeCount;
        planDesc.Resolution        = settings.Resolution;
        planDesc.SplitLambda       = settings.SplitLambda;
        planDesc.DepthBias         = settings.DepthBias;
        planDesc.NormalBias        = settings.NormalBias;
        planDesc.StabilizeCascades = settings.StabilizeCascades;
        planDesc.ReverseZ          = true; // Vulkan default
        if (!m_DirectionalPlanner.BuildPlan(planDesc, m_FrameData.Directional))
        {
            XENGINE_LOG_WARN("DirectionalShadowPlanner failed; disabling shadows this frame");
            m_FrameData = {};
            return;
        }

        // Wire GPU resources into the frame data.
        m_FrameData.Directional.Enabled        = true;
        m_FrameData.Directional.CascadeCount   = settings.CascadeCount;
        m_FrameData.Directional.ShadowTexture  = res.Texture.get();
        m_FrameData.Directional.SampledView    = res.SampledView.get();
        m_FrameData.Directional.Sampler        = res.Sampler.get();
        for (u32 i = 0; i < settings.CascadeCount; ++i)
        {
            m_FrameData.Directional.CascadeDepthViews[i] = res.LayerDepthViews[i].get();
        }
    }

    void RenderShadowManager::FillGPUShadowData(GPUShadowData& outData) const
    {
        const RenderDirectionalShadowFrameData& dir = GetFrameData().Directional;

        outData.ShadowParams = Vec4(
            dir.Enabled ? 1.0f : 0.0f,
            static_cast<float>(dir.CascadeCount),
            static_cast<float>(dir.CascadeCount > 0 ? dir.Cascades[0].Resolution : 0),
            0.0f); // visualize cascades is set by the caller (RenderFrameResources) from debug settings.

        for (u32 i = 0; i < MaxShadowCascades; ++i)
        {
            GPUCascadeShadowData& gpu = outData.Cascades[i];
            if (i < dir.CascadeCount)
            {
                const RenderShadowCascade& c = dir.Cascades[i];
                gpu.LightViewProjection = c.LightViewProjection;
                const float texelSize = (c.Resolution > 0)
                    ? 1.0f / static_cast<float>(c.Resolution)
                    : 0.0f;
                gpu.Params = Vec4(
                    c.SplitFar,
                    c.BiasParams.x, // depth bias
                    c.BiasParams.y, // normal bias
                    texelSize);
            }
            else
            {
                gpu.LightViewProjection = Mat4(1.0f);
                gpu.Params = Vec4(0.0f);
            }
        }
    }
}