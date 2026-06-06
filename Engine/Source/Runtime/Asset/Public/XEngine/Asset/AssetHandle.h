#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    constexpr u32 InvalidAssetHandleIndex = 0xffffffffu;

    // Runtime handle into AssetSystem metadata. It is not a persistent asset id.
    struct AssetHandle
    {
        u32 Index = InvalidAssetHandleIndex;
        u32 Generation = 0;

        bool IsValid() const
        {
            return Index != InvalidAssetHandleIndex;
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        friend bool operator==(const AssetHandle& lhs, const AssetHandle& rhs)
        {
            return lhs.Index == rhs.Index && lhs.Generation == rhs.Generation;
        }

        friend bool operator!=(const AssetHandle& lhs, const AssetHandle& rhs)
        {
            return !(lhs == rhs);
        }
    };
}
