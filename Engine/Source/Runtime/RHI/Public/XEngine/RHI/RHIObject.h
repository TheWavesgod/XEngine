// RHIObject — universal base class for every RHI handle type.
//
// Every RHI resource, command list, queue, instance, adapter, device, pipeline,
// and swapchain derives from RHIObject. The base class provides:
//
//   * a (nullable) back-pointer to the owning RHIDevice — null for objects
//     that exist before any device is created (RHIInstance, RHIAdapter),
//   * a RHIBackend tag so backend-specific casts can be safely rejected via
//     CheckedVulkanCast / CheckedD3D12Cast, and
//   * a virtual SetDebugName hook that backends override to forward the
//     name to the native debug API (vkSetDebugUtilsObjectNameEXT,
//     ID3D12Object::SetName, setLabel).
//
// The constructors are protected because the only legitimate way to obtain
// an RHIObject is through the RHIDevice factory methods (NVI on device,
// see plan.md §11) or RHIInstance / RHIAdapter constructors (M2).
//
// All member functions are inline-defined in the header so this file compiles
// even when RHIDevice is only forward-declared.

#pragma once

#include <XEngine/RHI/Base.h>
#include <XEngine/Core/Types.h>

#include <string_view>

namespace XEngine
{
    // Forward declaration — the real RHIDevice lands in M3.
    class RHIDevice;

    // Universal base for every RHI handle type.
    class RHIObject
    {
    public:
        virtual ~RHIObject() = default;

        // Device ownership. May be null for objects that exist before any
        // device is created (RHIInstance, RHIAdapter — M2).
        RHIDevice*       GetOwnerDevice()       noexcept { return m_OwnerDevice; }
        const RHIDevice* GetOwnerDevice() const noexcept { return m_OwnerDevice; }

        // Backend tag — used by CheckedVulkanCast / future CheckedD3D12Cast
        // to reject cross-backend casts at runtime.
        RHIBackend GetBackend() const noexcept { return m_Backend; }

        // Runtime debug naming. Default is no-op; backends override.
        virtual void SetDebugName(std::string_view name) noexcept { (void)name; }

    protected:
        // Default constructor — used by RHIInstance (root object).
        RHIObject() noexcept = default;

        // Backend-tagged constructor — used by RHIAdapter (no device yet).
        explicit RHIObject(RHIBackend backend) noexcept
            : m_Backend(backend)
        {
        }

        // Device-owned constructors — used by RHIDevice and concrete
        // resource types (RHIDevice, RHIBuffer, RHITexture, ...) once M3 lands.
        explicit RHIObject(RHIDevice& device) noexcept
            : m_OwnerDevice(&device)
            , m_Backend(RHIBackend::None)
        {
        }

        RHIObject(RHIDevice& device, RHIBackend backend) noexcept
            : m_OwnerDevice(&device)
            , m_Backend(backend)
        {
        }

        RHIDevice* m_OwnerDevice = nullptr;
        RHIBackend m_Backend     = RHIBackend::None;
    };
}
