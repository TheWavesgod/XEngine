#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    constexpr u32 InvalidHandleIndex = 0xFFFFFFFFu;

    template<typename Tag>
    struct Handle
    {
        u32 Index = InvalidHandleIndex;
        u32 Generation = 0;

        bool IsValid() const
        {
            return Index != InvalidHandleIndex;
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        friend bool operator==(const Handle& lhs, const Handle& rhs)
        {
            return lhs.Index == rhs.Index && lhs.Generation == rhs.Generation;
        }

        friend bool operator!=(const Handle& lhs, const Handle& rhs)
        {
            return !(lhs == rhs);
        }
    };
}
