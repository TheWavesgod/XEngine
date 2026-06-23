#include "DirectionalShadowPlanner.h"
#include "RenderShadowType.h"

#include <XEngine/Renderer/RenderScene.h>
#include <XEngine/Logging/Log.h>
#include <array>

namespace XEngine
{
    //The implementation should handle:
    // 1. Cascade split calculation.
    // 2. Camera frustum corners per split.
    // 3. Light view matrix.
    // 4. Orthographic projection bounds per cascade.
    // 5. Optional texel snapping.
    // 6. Filling RenderShadowCascade data.

    static void ComputeCascadeSplits(
        float cameraNear,
        float cameraFar,
        u32 cascadeCount,
        float splitLambda,
        float* outSplits)
    {
        splitLambda = std::clamp(splitLambda, 0.0f, 1.0f);

        const float nearClip = cameraNear;
        const float farClip = cameraFar;
        const float clipRange = farClip - nearClip;

        const float ratio = farClip / nearClip;

        for (u32 i = 0; i < cascadeCount; ++i)
        {
            const float p = static_cast<float>(i + 1) / static_cast<float>(cascadeCount);

            const float logSplit = nearClip * std::pow(ratio, p);
            const float linearSplit = nearClip + clipRange * p;

            const float split = Math::Lerp(linearSplit, logSplit, splitLambda);
        }
    }

    static std::array<Vec3, 8> GetCameraFrustumCornersWorldSpace(
        const Mat4& cameraView,
        const Mat4& cameraProjection)
    {
        const Mat4 invViewProj = Math::Inverse(cameraProjection * cameraView);

        constexpr float ndcNearZ = 0.0f;
        constexpr float ndcFarZ  = 1.0f;

        const std::array<Vec3, 8> ndcCorners =
        {
            Vec3(-1.0f, -1.0f, ndcNearZ),
            Vec3( 1.0f, -1.0f, ndcNearZ),
            Vec3( 1.0f,  1.0f, ndcNearZ),
            Vec3(-1.0f,  1.0f, ndcNearZ),

            Vec3(-1.0f, -1.0f, ndcFarZ),
            Vec3( 1.0f, -1.0f, ndcFarZ),
            Vec3( 1.0f,  1.0f, ndcFarZ),
            Vec3(-1.0f,  1.0f, ndcFarZ)
        };

        std::array<Vec3, 8> cornersWorld;

        for (u32 i = 0; i < 8; ++i)
        {
            Vec4 p = invViewProj * Vec4(ndcCorners[i], 1.0f);
            cornersWorld[i] = Vec3(p) / p.w;
        }

        return cornersWorld;
    }

    static std::array<Vec3, 8> GetCascadeFrustumCornersWorldSpace(
        const std::array<Vec3, 8>& fullFrustumCorners,
        float cameraNear,
        float cameraFar,
        float splitNear,
        float splitFar)
    {
        std::array<Vec3, 8> cascadeCorners;

        const float nearT = (splitNear - cameraNear) / (cameraFar - cameraNear);
        const float farT  = (splitFar  - cameraNear) / (cameraFar - cameraNear);

        for (u32 i = 0; i < 4; ++i)
        {
            const Vec3 fullNear = fullFrustumCorners[i];
            const Vec3 fullFar  = fullFrustumCorners[i + 4];

            const Vec3 ray = fullFar - fullNear;

            cascadeCorners[i]     = fullNear + ray * nearT;
            cascadeCorners[i + 4] = fullNear + ray * farT;
        }

        return cascadeCorners;
    }

    static Vec3 ComputeAverageCenter(const std::array<Vec3, 8>& points)
    {
        Vec3 center(0.0f);

        for (const Vec3& p : points)
        {
            center += p;
        }

        return center / 8.0f;
    }

    static float ComputeBoundingSphereRadius(
        const std::array<Vec3, 8>& points,
        const Vec3& center)
    {
        float radius = 0.0f;
        for (const Vec3& p : points)
        {
            radius = Math::Max(radius, Math::Length(p - center));
        }
        return radius;
    }

    // In order to stabilize the radius from slight varies of float point errors
    static float QuantizeRadius(float radius)
    {
        // this 16.0f doesn't contain any meanings
        return std::ceil(radius * 16.0f) / 16.0f;
    }


    bool DirectionalShadowPlanner::BuildPlan(const DirectionalShadowPlanDesc& desc, 
        RenderDirectionalShadowFrameData& outData) const
    {
        if (desc.Light == nullptr)
        {
            XENGINE_LOG_WARN("Light from description is unvalid");
            return false;
        }

        if (desc.Light->Type != RenderLightType::Directional)
        {
            XENGINE_LOG_WARN("Light type is not directional");
            return false;
        }

        if (Math::Length(desc.Light->DirectionToLight) <= 0.0001f)
        {
            return false;
        }

        if (desc.CameraNear <= 0.0f || desc.CameraFar <= desc.CameraNear)
        {
            return false;
        }

        if (desc.CascadeCount == 0 || desc.CascadeCount > MaxShadowCascades)
        {
            return false;
        }

        if (desc.Resolution == 0)
        {
            return false;
        }

        const u32 cascadeCount = desc.CascadeCount;

        outData.Enabled = true;
        outData.CascadeCount = cascadeCount;

        float cascadeSplits[MaxShadowCascades] = {};
        ComputeCascadeSplits(
            desc.CameraNear,
            desc.CameraFar,
            cascadeCount,
            desc.SplitLambda,
            cascadeSplits);

        const auto fullFrustumCorners = GetCameraFrustumCornersWorldSpace(
            desc.CameraView,
            desc.CameraProjection); 

        float previousSplit = desc.CameraNear;
        for (u32 cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex)
        {
            const float splitNear = previousSplit;
            const float splitFar = cascadeSplits[cascadeIndex];

            const auto cascadeCorners = GetCascadeFrustumCornersWorldSpace(
                fullFrustumCorners,
                desc.CameraNear,
                desc.CameraFar,
                splitNear,
                splitFar);
            
            // TODO
        }

        return true;
    }
}