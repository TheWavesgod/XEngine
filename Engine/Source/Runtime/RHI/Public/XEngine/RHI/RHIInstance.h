// RHIInstance — top-level RHI entry point. One per process.
//
// M2 surface (after split: RHIAdapter lives in its own header):
//   * Create()                 — static factory (M2 stub returns nullptr)
//   * EnumerateAdapters()      — virtual, backend-specific
//   * RequestAdapter()         — virtual with default implementation
//   * CreateDevice()           — virtual, single-device rule
//   * GetDevice() / GetDesc()  — accessors
//   * ScoreAdapter()           — static helper, unit-tested in isolation
//
// The single-device rule is enforced physically by holding m_Device inside
// the instance. RHIInstance::CreateDevice is the only path that creates a
// device, and it returns nullptr if m_Device is already populated.

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIAdapter.h>  // RHIAdapterInfo + RHIAdapter full definition needed by vector<unique_ptr<RHIAdapter>> return type
#include <XEngine/RHI/RHIDevice.h>

#include <memory>
#include <vector>

namespace XEngine
{
    // Forward declarations — RHIAdapter / RHIAdapterInfo are defined in
    // RHIAdapter.h. Forward decls here keep RHIInstance.h small and avoid
    // a circular include (RHIAdapter.h also forward-declares RHIInstance).
    class RHIAdapter;
    struct RHIAdapterInfo;

    // RHIInstance — top-level entry point.
    class RHIInstance : public RHIObject
    {
    public:
        // Static factory. M2 returns nullptr because no backend target is
        // built yet. M3 dispatches to VulkanRHI / D3D12RHI / MetalRHI based
        // on the desc and the platform's available backends.
        static std::unique_ptr<RHIInstance> Create(const RHIInstanceDesc& desc);

        // Enumerate all physical GPUs visible to this instance. The caller
        // stores the returned vector; adapters outlive any device created
        // from them.
        //
        // Non-const because the implementation allocates new RHIAdapter
        // wrappers (heap side effect), even though it does not change the
        // logical instance state.
        virtual std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() = 0;

        // Pick the best adapter per preference, filtering by required caps.
        // Returns nullptr if no adapter satisfies the filter.
        //
        // Default implementation: EnumerateAdapters() + ScoreAdapter() + pick highest.
        // Backends can override if they have a more efficient backend-specific
        // path (e.g., Vulkan's vkEnumeratePhysicalDevices with a filtering hint).
        //
        // Non-const because it calls EnumerateAdapters which allocates new
        // RHIAdapter wrappers.
        virtual std::unique_ptr<RHIAdapter> RequestAdapter(
            RHIAdapterPreference preference,
            const RHICapabilities& required = RHICapabilities{});

        // Single-device rule. Returns nullptr if a device already exists.
        // Otherwise creates and OWNS the device; user gets a non-owning pointer.
        virtual RHIDevice* CreateDevice(
            RHIAdapter& adapter,
            const RHIDeviceDesc& desc = RHIDeviceDesc{}) = 0;

        RHIDevice* GetDevice() const noexcept { return m_Device.get(); }
        const RHIInstanceDesc& GetDesc() const noexcept { return m_Desc; }

        // Returns 0 if the adapter is unsuitable. Higher = better.
        // Public so unit tests can verify the scoring algorithm in isolation.
        static u32 ScoreAdapter(const RHIAdapterInfo& info, RHIAdapterPreference preference);

        virtual ~RHIInstance() override = default;

    protected:
        explicit RHIInstance(const RHIInstanceDesc& desc, RHIBackend backend) noexcept
            : RHIObject(backend)
            , m_Desc(desc)
        {
        }

        RHIInstanceDesc m_Desc;
        std::unique_ptr<RHIDevice> m_Device;
    };
}
