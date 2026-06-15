#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/AABB.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/Renderer/Material.h>
#include <XEngine/Renderer/Mesh.h>

#include <vector>

namespace XEngine
{
    // Renderer-facing object generated from Scene components.
    // This is not an ECS component and should not be stored in Scene.
    struct RenderObject
    {
        Mat4 WorldMatrix { 1.0f };
        Mat4 PreviousWorldMatrix { 1.0f };

        MeshHandle Mesh;
        MaterialHandle Material;

        AABB WorldBounds {};

        u32 ObjectId = 0;
        u32 Flags = 0;
    };

    enum class RenderLightType
    {
        Directional,
        Point,
        Spot
    };

    struct RenderLight
    {
        RenderLightType Type = RenderLightType::Directional;

        Vec3 Position { 0.0f, 0.0f, 0.0f };
        float Range = 0.0f;

        Vec3 DirectionToLight { 0.0f, 0.0f, 1.0f };
        float Intensity = 1.0f;

        Vec3 Color { 1.0f, 1.0f, 1.0f };

        float InnerConeAngleRadians = 0.0f;
        float OuterConeAngleRadians = 0.0f;

        bool CastShadow = false;
        bool Enabled = true;
    };

    // Renderer-facing scene data consumed by render passes.
    struct RenderScene
    {
        std::vector<RenderObject> OpaqueObjects;
        std::vector<RenderLight> Lights;

        void Clear()
        {
            OpaqueObjects.clear();
            Lights.clear();
        }
    };
}
