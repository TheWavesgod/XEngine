#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    constexpr u32 InvalidEntityIndex = 0xffffffffu;

    // Lightweight runtime entity handle owned by Scene.
    // It is valid only for the lifetime of its Scene and is not a persistent scene id.
    struct Entity
    {
        u32 Index = InvalidEntityIndex;
        u32 Generation = 0;

        bool IsValid() const
        {
            return Index != InvalidEntityIndex;
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        friend bool operator==(const Entity& lhs, const Entity& rhs)
        {
            return lhs.Index == rhs.Index && lhs.Generation == rhs.Generation;
        }

        friend bool operator!=(const Entity& lhs, const Entity& rhs)
        {
            return !(lhs == rhs);
        }
    };
}
