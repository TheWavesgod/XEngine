// RHIAdapter — physical GPU descriptor.
//
// M2 surface:
//   * GetInfo() — adapter metadata (returns RHIAdapterInfo)
//   * SupportsRequiredCapabilities — capability filter (M3 fills in)
//   * owned by RHIInstance (m_OwnerInstance back-pointer)
//
// RHIAdapterInfo is colocated here (Option A1) so callers that need
// "everything about an RHIAdapter" pick up one header.

#pragma once

#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIEnums.h>

#include <string_view>

namespace XEngine
{
    // Forward declarations — RHIAdapter's constructor takes RHIInstance&;
    // SupportsRequiredCapabilities takes const RHICapabilities& (defined in RHIDevice.h).
    class RHIInstance;
    struct RHICapabilities;

    // RHIAdapterInfo — query result of RHIAdapter::GetInfo().
    struct RHIAdapterInfo
    {
        std::string_view VendorName;
        std::string_view AdapterName;
        std::string_view DriverVersion;
        std::string_view APIInfo;                 // e.g., "Vulkan 1.3.0", "DirectX 12.2", "Metal 3.0"
        u32              VendorID             = 0;
        u32              DeviceID             = 0;
        RHIAdapterType   Type                 = RHIAdapterType::Unknown;
        u64              DedicatedMemoryBytes = 0;
        u64              SharedMemoryBytes    = 0;
    };

    // RHIAdapter — physical GPU descriptor.
    class RHIAdapter : public RHIObject
    {
    public:
        virtual ~RHIAdapter() override = default;

        // Returns the adapter's metadata. By value because the info is small
        // and the string_view fields point into the adapter's owned storage.
        virtual RHIAdapterInfo GetInfo() const = 0;

        // Returns true iff every required capability is supported.
        // M2: stub returns true; M3 fills in the real capability comparator.
        virtual bool SupportsRequiredCapabilities(const RHICapabilities& required) const = 0;

    protected:
        explicit RHIAdapter(RHIInstance& owner) noexcept
            : RHIObject()
            , m_OwnerInstance(&owner)
        {
        }

        RHIAdapter(RHIInstance& owner, RHIBackend backend) noexcept
            : RHIObject(backend)
            , m_OwnerInstance(&owner)
        {
        }

    private:
        RHIInstance* m_OwnerInstance = nullptr;
    };
}
