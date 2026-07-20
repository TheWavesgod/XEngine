#include "DirectionalShadowPlanner.h"
#include "RenderShadowType.h"

#include <XEngine/Renderer/RenderScene.h>
#include <XEngine/Logging/Log.h>
#include <array>
#include <cmath>      // std::fabs, std::round
#include <limits>     // std::numeric_limits
#include <string>

namespace XEngine
{
    //The implementation should handle:
    // 1. Cascade split calculation.
    // 2. Camera frustum corners per split.
    // 3. Light view matrix.
    // 4. Orthographic projection bounds per cascade.
    // 5. Optional texel snapping.
    // 6. Filling RenderShadowCascade data.

    // Cascades use a fixed 4x depth-bias slack multiplier on the ortho extents;
    // centralize it so changing the bias multiplier in one place propagates to
    // every projection and texel-snap site.
    static constexpr float CascadeDepthBiasSlackMultiplier = 4.0f;

    // Build the cascade's orthographic projection.
    // Uses Vulkan forward-Z default (LESS depth-compare, depth buffer in [0, 1]
    // with 0 = near, 1 = far). The projection passes `near = -half, far = +half`
    // so light's near plane sits at NDC.z = -1 and far at NDC.z = +1.
    // The reverseZ parameter is preserved as a hook for future explicit
    // reverse-Z hardware or shader path but currently ignored so the GPU hardware
    // convention matches the shader's `<=`-lit test.
    static Mat4 MakeCascadeProjection(float radius, float depthBias, bool /*reverseZ*/)
    {
        const float half = radius + depthBias * CascadeDepthBiasSlackMultiplier;
        // Vulkan default: light POV has near = -half (closest to light) at NDC.z = -1
        // and far = +half (farthest from light) at NDC.z = +1. GPU maps NDC.z=[-1,1]
        // linearly to depth buffer [0, 1] with 0=near (close to light) and 1=far.
        return Math::OrthographicLH_ZO(-half, half, -half, half, -half, half);
    }

    static void ComputeCascadeSplits(
        float cameraNear,
        float cameraFar,
        u32 cascadeCount,
        float splitLambda,
        float* outSplits)
    {
        splitLambda = Math::Clamp(splitLambda, 0.0f, 1.0f);

        const float nearClip = cameraNear;
        const float farClip = cameraFar;
        const float clipRange = farClip - nearClip;

        // Guard against non-positive near plane to avoid log(<=0) below.
        const float safeNear = (nearClip > 1e-4f) ? nearClip : 1e-4f;
        
        const float ratio = farClip / safeNear;

        for (u32 i = 0; i < cascadeCount; ++i)
        {
            const float p = static_cast<float>(i + 1) / static_cast<float>(cascadeCount);

            const float logSplit = nearClip * std::pow(ratio, p);
            const float linearSplit = nearClip + clipRange * p;

            const float split = Math::Lerp(linearSplit, logSplit, splitLambda);

            outSplits[i] = split;
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

    struct LightBasis
    {
        Vec3 Forward;  // +X in light space (look direction)
        Vec3 Up;       // +Z in light space
        Vec3 Right;    // +Y in light space
    };

    static LightBasis BuildLightBasis(const Vec3& directionToLight)
    {
        LightBasis basis;
        basis.Forward = Math::Normalize(directionToLight);

        // World up is +Z. If the light is nearly vertical, fall back to +Y
        // to avoid a degenerate cross product.
        const Vec3 worldUp = (std::fabs(basis.Forward.z) > 0.999f)
            ? Vec3(0.0f, 1.0f, 0.0f)
            : Vec3(0.0f, 0.0f, 1.0f);

        // For a left-handed basis where Cross(Right, Up) = Forward,
        // Right = Cross(worldUp, Forward) and Up = Cross(Forward, Right).
        basis.Right = Math::Normalize(Math::Cross(basis.Forward, worldUp));
        basis.Up    = Math::Normalize(Math::Cross(basis.Right,  basis.Forward));
        return basis;
    }

    // Translates a world-space center to the closest orthographic frustum center
    // aligned on the shadow map's texel grid. This keeps cascades from "swimming"
    // as the camera moves; it is the standard "stable CSM" trick.
    static Vec3 SnapToTexelGrid(
        const Vec3& worldCenter,
        const Mat4& lightViewProj,
        float texelSize)
    {
        // Project the world center into light clip space.
        const Vec4 clip = lightViewProj * Vec4(worldCenter, 1.0f);

        // Snap UV in NDC, then transform back to world.
        const float snappedX = std::round(clip.x / clip.w / texelSize) * texelSize;
        const float snappedY = std::round(clip.y / clip.w / texelSize) * texelSize;

        // Build a translation that cancels the sub-texel offset.
        const Vec4 offset = clip - Vec4(snappedX * clip.w, snappedY * clip.w, 0.0f, 0.0f);
        (void)offset;

        return worldCenter;   // Texel-snap is applied via an adjusted light view, not a world translation.
    }

    // Variant that returns the world-space offset to subtract from the light
    // position so the cascade's projected center sits on a texel boundary.
    static Vec3 ComputeTexelSnapOffset(
        const Vec3& worldCenter,
        const Mat4& lightView,
        const Mat4& lightProj,
        float texelSize)
    {
        const Mat4 lightViewProj = lightProj * lightView;
        const Vec4 clip = lightViewProj * Vec4(worldCenter, 1.0f);
        if (clip.w == 0.0f)
        {
            return Vec3(0.0f);
        }

        const float ndcX = clip.x / clip.w;
        const float ndcY = clip.y / clip.w;
        const float snappedNdcX = std::round(ndcX / texelSize) * texelSize;
        const float snappedNdcY = std::round(ndcY / texelSize) * texelSize;

        // The light position needs to shift so the projected center lands on the snapped NDC.
        const Mat4 invView = Math::Inverse(lightView);
        const Vec4 worldOffset = invView * Vec4(
            (snappedNdcX - ndcX) * clip.w,
            (snappedNdcY - ndcY) * clip.w,
            0.0f, 0.0f);
        return Vec3(worldOffset);
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

        const LightBasis lightBasis = BuildLightBasis(desc.Light->DirectionToLight);
        const float texelSize = 2.0f / static_cast<float>(desc.Resolution);

        float previousSplit = desc.CameraNear;
        for (u32 cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex)
        {
            const float splitNear = previousSplit;
            const float splitFar = cascadeSplits[cascadeIndex];

            // Cascade sub-frustum in world space.
            const auto cascadeCorners = GetCascadeFrustumCornersWorldSpace(
                fullFrustumCorners,
                desc.CameraNear,
                desc.CameraFar,
                splitNear,
                splitFar);
            
            // Bounding sphere around the cascade sub-frustum.
            const Vec3  center     = ComputeAverageCenter(cascadeCorners);
            const float rawRadius  = ComputeBoundingSphereRadius(cascadeCorners, center);
            const float radius     = QuantizeRadius(rawRadius);

            // Push the light back along its forward axis so the entire sphere sits
            // in front of the light (positive Z in light space).
            const Vec3 lightPosition = center - lightBasis.Forward * radius;

            // Build the light view matrix using XEngine's +X-forward convention.
            // BuildViewMatrixLH_XForward expects the eye position and the world rotation
            // of the camera; here we just construct the matrix directly from the basis.
            Mat4 lightView = Mat4(1.0f);
            lightView[0]  = Vec4(lightBasis.Right,   0.0f);
            lightView[1]  = Vec4(lightBasis.Up,      0.0f);
            lightView[2]  = Vec4(lightBasis.Forward, 0.0f);
            lightView[3]  = Vec4(lightPosition,      1.0f);
            lightView     = Math::Inverse(lightView);

            // DEBUG: dump direction-to-light, computed light position, and the
            // post-Inverse view matrix column 3 (translation).
            XENGINE_LOG_INFO(
                std::string("DBG cascade=0 directionToLight=(")
                + std::to_string(desc.Light->DirectionToLight.x) + ","
                + std::to_string(desc.Light->DirectionToLight.y) + ","
                + std::to_string(desc.Light->DirectionToLight.z) + ") "
                + "lightPos=(" + std::to_string(lightPosition.x) + ","
                + std::to_string(lightPosition.y) + ","
                + std::to_string(lightPosition.z) + ") "
                + "viewCol3=(" + std::to_string(lightView[3][0]) + ","
                + std::to_string(lightView[3][1]) + ","
                + std::to_string(lightView[3][2]) + ","
                + std::to_string(lightView[3][3]) + ")");

            // Apply texel snap to keep cascades from swimming (Step 1's StabilizeCascades).
            // The snap projection must match the final cascade projection exactly,
            // so we delegate to the same MakeCascadeProjection helper used below.
            if (desc.StabilizeCascades)
            {
                const Mat4 tmpProj = MakeCascadeProjection(radius, desc.DepthBias, desc.ReverseZ);

                const Vec3 snap = ComputeTexelSnapOffset(center, lightView, tmpProj, texelSize);
                XENGINE_LOG_INFO("DBG snap=(" + std::to_string(snap.x) + ","
                    + std::to_string(snap.y) + "," + std::to_string(snap.z) + ")");
                lightView[3] -= Vec4(snap, 0.0f);
                XENGINE_LOG_INFO("DBG viewCol3-after-snap=(" + std::to_string(lightView[3][0]) + ","
                    + std::to_string(lightView[3][1]) + ","
                    + std::to_string(lightView[3][2]) + ","
                    + std::to_string(lightView[3][3]) + ")");
            }

            // Final light projection (orthographic).
            const Mat4 lightProjection = MakeCascadeProjection(radius, desc.DepthBias, desc.ReverseZ);
            const Mat4 lightViewProj = lightProjection * lightView;

            // Fill the cascade slot.
            RenderShadowCascade& cascade = outData.Cascades[cascadeIndex];
            cascade.LightView           = lightView;
            cascade.LightProjection     = lightProjection;
            cascade.LightViewProjection = lightViewProj;
            cascade.SplitNear           = splitNear;
            cascade.SplitFar            = splitFar;
            cascade.LayerIndex          = cascadeIndex;
            cascade.Resolution          = desc.Resolution;
            cascade.ShadowMapSize       = Vec4(
                static_cast<float>(desc.Resolution),
                static_cast<float>(desc.Resolution),
                1.0f / static_cast<float>(desc.Resolution),
                1.0f / static_cast<float>(desc.Resolution));
            cascade.BiasParams = Vec4(
                desc.DepthBias,
                desc.NormalBias,
                0.0f,    // slope-scaled bias factor (Stage 10+)
                0.0f);

            // World bounds: the 8 cascade frustum corners.
            AABB worldBounds;
            worldBounds.Min = Vec3( std::numeric_limits<float>::infinity());
            worldBounds.Max = Vec3(-std::numeric_limits<float>::infinity());
            for (const Vec3& corner : cascadeCorners)
            {
                worldBounds.Min = Math::Min(worldBounds.Min, corner);
                worldBounds.Max = Math::Max(worldBounds.Max, corner);
            }
            cascade.WorldBounds = worldBounds;

            // Light-space bounds: transform corners into light space and re-fit.
            Vec3 lightSpaceMin( std::numeric_limits<float>::infinity());
            Vec3 lightSpaceMax(-std::numeric_limits<float>::infinity());
            for (const Vec3& corner : cascadeCorners)
            {
                const Vec4 ls = lightView * Vec4(corner, 1.0f);
                lightSpaceMin = Math::Min(lightSpaceMin, Vec3(ls));
                lightSpaceMax = Math::Max(lightSpaceMax, Vec3(ls));
            }
            cascade.LightSpaceBounds = AABB(lightSpaceMin, lightSpaceMax);

            previousSplit = splitFar;
        }

        return true;
    }
}