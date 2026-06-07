#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    constexpr u32 InvalidMeshHandleIndex = 0xffffffffu;

    // Renderer-side handle for GPU mesh resources managed by RenderMeshManager.
    // This is runtime-only and is not persistent asset identity.
    struct MeshHandle
    {
        u32 Index = InvalidMeshHandleIndex;
        u32 Generation = 0;

        bool IsValid() const
        {
            return Index != InvalidMeshHandleIndex;
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        friend bool operator==(const MeshHandle& lhs, const MeshHandle& rhs)
        {
            return lhs.Index == rhs.Index && lhs.Generation == rhs.Generation;
        }

        friend bool operator!=(const MeshHandle& lhs, const MeshHandle& rhs)
        {
            return !(lhs == rhs);
        }
    };
}
