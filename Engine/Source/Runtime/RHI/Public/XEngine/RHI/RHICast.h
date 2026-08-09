// RHICast — backend-checked downcast helper for RHI handle types.
//
// Every concrete backend class (D3D12Instance, VulkanAdapter, …) derives
// from one of the RHIObject abstract bases (RHIInstance, RHIAdapter, …).
// Inside a backend's CreateXxxImpl overrides we routinely receive an
// RHI-base reference and need the concrete type — e.g.:
//
//     std::unique_ptr<RHIDevice> D3D12Instance::CreateDeviceImpl(
//         RHIAdapter& adapter, const RHIDeviceDesc&)
//     {
//         auto& d3d12 = XEngine::CheckedCast<D3D12Adapter>(adapter);
//         return D3D12Device::Create(d3d12, …);
//     }
//
// A naked `static_cast` is correct in normal code but silently produces UB
// if a future caller ever passes an RHIObject belonging to a different
// backend. CheckedCast<T> catches that in two ways:
//
//   1. Compile-time static_assert that T derives from RHIObject.
//   2. Compile-time static_assert that T declares
//      `static constexpr RHIBackend ExpectedBackend;` so the helper knows
//      which backend tag the target belongs to.
//   3. Debug-time XENGINE_ASSERT that the runtime m_Backend matches
//      T::ExpectedBackend. In release builds XENGINE_ASSERT is a no-op,
//      so this degrades to plain `static_cast<T&>` with zero cost.
//
// The runtime check is intentionally RTTI-free — the project deliberately
// does not rely on dynamic_cast, and RHIObject already carries the
// backend tag for exactly this purpose.
//
// Adding a new backend (e.g. Metal) requires no change here: every
// concrete Metal* class just declares its own `ExpectedBackend`.

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/Core/Assert.h>

#include <type_traits>

namespace XEngine
{
    // Downcast a non-const RHIObject reference to a concrete backend type.
    //
    // T must satisfy:
    //   * public (or protected-internal) derivation from RHIObject, and
    //   * declare `static constexpr RHIBackend ExpectedBackend;`.
    //
    // In debug builds a backend-tag mismatch triggers an assertion. In
    // release builds the helper degenerates to `static_cast<T&>`.
    template <typename T>
    T& CheckedCast(RHIObject& obj) noexcept
    {
        static_assert(std::is_base_of_v<RHIObject, T>,
            "CheckedCast<T>: T must derive from XEngine::RHIObject.");
        static_assert(requires { T::ExpectedBackend; },
            "CheckedCast<T>: target type must declare "
            "`static constexpr RHIBackend ExpectedBackend;`.");

        XENGINE_ASSERT(obj.GetBackend() == T::ExpectedBackend,
            "CheckedCast<T>: backend tag mismatch — refusing cross-backend cast.");
        return static_cast<T&>(obj);
    }

    // Const overload so callers don't need std::as_const at the call site.
    template <typename T>
    const T& CheckedCast(const RHIObject& obj) noexcept
    {
        static_assert(std::is_base_of_v<RHIObject, T>,
            "CheckedCast<T>: T must derive from XEngine::RHIObject.");
        static_assert(requires { T::ExpectedBackend; },
            "CheckedCast<T>: target type must declare "
            "`static constexpr RHIBackend ExpectedBackend;`.");

        XENGINE_ASSERT(obj.GetBackend() == T::ExpectedBackend,
            "CheckedCast<T>: backend tag mismatch — refusing cross-backend cast.");
        return static_cast<const T&>(obj);
    }
}