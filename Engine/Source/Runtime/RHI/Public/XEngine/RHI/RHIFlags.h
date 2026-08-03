// RHIFlags — generic templated flag-bit utilities for RHI enum types.
//
// The RHI defines several flag-style enums (RHIBufferUsage, RHITextureUsage,
// RHIShaderStage, ...) that all need the same operations: HasFlag, HasAnyFlag,
// EnableFlag, DisableFlag, plus operator overloads (|, &, ~, |=, &=) for
// natural C++ syntax. These utilities are templated on the enum type so
// they work for any flag enum with a None = 0 entry and underlying type
// std::underlying_type_t<T>.
//
// Usage:
//   enum class RHIBufferUsage : u32 {
//       None        = 0,
//       Vertex      = 1 << 0,
//       Index       = 1 << 1,
//       Storage     = 1 << 2,
//   };
//
//   if (HasFlag(usage, RHIBufferUsage::Storage)) { ... }
//   auto combined = RHIBufferUsage::Vertex | RHIBufferUsage::Index;
//   auto without  = combined & ~RHIBufferUsage::Index;

#pragma once

#include <XEngine/Core/Types.h>

#include <type_traits>

namespace XEngine
{
    // HasFlag: returns true if every bit in `flag` is set in `value`.
    // `flag = T::None` returns false (no flag to test).
    template <typename T>
    constexpr bool HasFlag(T value, T flag) noexcept
    {
        static_assert(std::is_enum_v<T>, "HasFlag<T>: T must be an enum type");
        using U = std::underlying_type_t<T>;
        const U uflag = static_cast<U>(flag);
        return uflag != U{}
            && (static_cast<U>(value) & uflag) == uflag;
    }

    // HasAnyFlag: returns true iff any bit shared between `value` and `flags`.
    // `flags = T::None` returns false.
    template <typename T>
    constexpr bool HasAnyFlag(T value, T flags) noexcept
    {
        static_assert(std::is_enum_v<T>, "HasAnyFlag<T>: T must be an enum type");
        using U = std::underlying_type_t<T>;
        return (static_cast<U>(value) & static_cast<U>(flags)) != U{};
    }

    // EnableFlag: returns `value | flag`.
    template <typename T>
    constexpr T EnableFlag(T value, T flag) noexcept
    {
        static_assert(std::is_enum_v<T>, "EnableFlag<T>: T must be an enum type");
        using U = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<U>(value) | static_cast<U>(flag));
    }

    // DisableFlag: returns `value & ~flag`.
    template <typename T>
    constexpr T DisableFlag(T value, T flag) noexcept
    {
        static_assert(std::is_enum_v<T>, "DisableFlag<T>: T must be an enum type");
        using U = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<U>(value) & ~static_cast<U>(flag));
    }

    // CombineFlags: variadic OR of all flags. Useful for declarations.
    template <typename T, typename... Args>
    constexpr T CombineFlags(T first, Args... rest) noexcept
    {
        static_assert(std::is_enum_v<T>, "CombineFlags<T>: T must be an enum type");
        if constexpr (sizeof...(rest) == 0)
        {
            return first;
        }
        else
        {
            return EnableFlag(first, CombineFlags(rest...));
        }
    }

    // ---------------------------------------------------------------------
    // Operator overloads. These are the natural C++ syntax for combining
    // flag-style enums. They are defined as free templates so any flag enum
    // gets them automatically. The static_assert enforces enum-only usage
    // at the call site.
    // ---------------------------------------------------------------------

    template <typename T>
    constexpr T operator|(T a, T b) noexcept
    {
        static_assert(std::is_enum_v<T>, "operator|<T>: T must be an enum type");
        return EnableFlag(a, b);
    }

    template <typename T>
    constexpr T operator&(T a, T b) noexcept
    {
        static_assert(std::is_enum_v<T>, "operator&<T>: T must be an enum type");
        using U = std::underlying_type_t<T>;
        return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));
    }

    template <typename T>
    constexpr T operator~(T a) noexcept
    {
        static_assert(std::is_enum_v<T>, "operator~<T>: T must be an enum type");
        using U = std::underlying_type_t<T>;
        return static_cast<T>(~static_cast<U>(a));
    }

    template <typename T>
    constexpr T& operator|=(T& a, T b) noexcept
    {
        static_assert(std::is_enum_v<T>, "operator|=<T>: T must be an enum type");
        a = a | b;
        return a;
    }

    template <typename T>
    constexpr T& operator&=(T& a, T b) noexcept
    {
        static_assert(std::is_enum_v<T>, "operator&=<T>: T must be an enum type");
        a = a & b;
        return a;
    }
}
