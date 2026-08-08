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
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/Logging/Log.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class RHIAdapter;
    struct RHIAdapterInfo;

    // RHIInstance — top-level entry point.
    class RHIInstance : public RHIObject
    {
    public:
        // Static factory.
        static std::unique_ptr<RHIInstance> Create(const RHIInstanceDesc& desc);

        // Enumerate all physical GPUs visible to this instance. The caller
        // stores the returned vector; adapters outlive any device created
        // from them.
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

        // NVI wrapper around CreateDeviceImpl. Enforces:
        //   1. Single-device rule: returns nullptr if m_Device is set.
        //   2. RequiredFeatures ⊆ adapter.GetSupportedFeatures().
        //   3. On success, stores the device in m_Device and returns a
        //      non-owning pointer; the caller does NOT free the device
        //      (the instance owns it).
        //
        // Backends override CreateDeviceImpl, not CreateDevice.
        RHIDevice* CreateDevice(
            RHIAdapter& adapter,
            const RHIDeviceDesc& desc = RHIDeviceDesc{});

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

        // Backend hook. Returns nullptr on failure (e.g. feature mismatch
        // surfaced at a layer the NVI wrapper cannot pre-check). The wrapper
        // stores the returned unique_ptr in m_Device; backends must NOT
        // keep their own copy.
        virtual std::unique_ptr<RHIDevice> CreateDeviceImpl(
            RHIAdapter& adapter,
            const RHIDeviceDesc& desc) = 0;

        RHIInstanceDesc m_Desc;
        std::unique_ptr<RHIDevice> m_Device;
    };
}
