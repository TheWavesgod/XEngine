// Unit tests for RHIAdapter abstract interface.
//
// M2 adapter tests work on a stub implementation — the real backend
// adapters (Vulkan / D3D12 / Metal) land in M3 and replace this stub.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIInstance.h>  // brings RHIInstance, RHIInstanceDesc
#include <XEngine/RHI/RHIAdapter.h>   // brings RHIAdapter, RHIAdapterInfo
#include <XEngine/RHI/RHIDevice.h>    // brings RHIDevice, RHICapabilities

#include "RHITestStubs.h"

#include <string_view>

namespace
{
    using namespace XEngine;
    using StubInstance = Test::StubInstance;

    // Concrete adapter stub that records the info it was constructed with
    // and lets tests toggle the capability filter.
    class StubAdapter : public RHIAdapter
    {
    public:
        StubAdapter(RHIInstance& owner, RHIAdapterInfo info)
            : RHIAdapter(owner, RHIBackend::Vulkan)
            , m_Info(info)
        {
        }

        RHIAdapterInfo GetInfo() const override { return m_Info; }
        RHIFeature GetSupportedFeatures() const noexcept override { return m_SupportedFeatures; }

        bool SupportsRequiredCapabilities(const RHICapabilities&) const override
        {
            return m_CapsSupported;
        }

        // Test harness: switch the caps filter on / off.
        void SetCapabilitiesSupported(bool supported) noexcept
        {
            m_CapsSupported = supported;
        }

        void SetSupportedFeatures(RHIFeature f) noexcept { m_SupportedFeatures = f; }

    private:
        RHIAdapterInfo m_Info;
        bool m_CapsSupported = true;
        RHIFeature m_SupportedFeatures = RHIFeature::None;
    };

    // ---------------------------------------------------------------------
    TEST(RHIAdapter, OwnerDeviceIsNull)
    {
        // RHIAdapter has no device owner — its owner is the instance.
        StubInstance instance;
        RHIAdapterInfo info{ .Type = RHIAdapterType::Discrete };
        StubAdapter adapter(instance, info);

        EXPECT_EQ(adapter.GetOwnerDevice(), nullptr);
        EXPECT_EQ(adapter.GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHIAdapter, GetInfoReturnsConstructedValues)
    {
        StubInstance instance;
        RHIAdapterInfo info{
            .VendorName   = "NVIDIA",
            .AdapterName  = "RTX 4090",
            .Type         = RHIAdapterType::Discrete,
            .DedicatedMemoryBytes = 8ull * 1024 * 1024 * 1024,
        };

        StubAdapter adapter(instance, info);
        const RHIAdapterInfo got = adapter.GetInfo();

        EXPECT_EQ(got.VendorName, "NVIDIA");
        EXPECT_EQ(got.AdapterName, "RTX 4090");
        EXPECT_EQ(got.Type, RHIAdapterType::Discrete);
        EXPECT_EQ(got.DedicatedMemoryBytes, 8ull * 1024 * 1024 * 1024);
    }

    TEST(RHIAdapter, SupportsRequiredCapabilitiesControlledByStub)
    {
        StubInstance instance;
        RHIAdapterInfo info;
        StubAdapter adapter(instance, info);

        RHICapabilities caps;
        EXPECT_TRUE(adapter.SupportsRequiredCapabilities(caps));

        adapter.SetCapabilitiesSupported(false);
        EXPECT_FALSE(adapter.SupportsRequiredCapabilities(caps));

        adapter.SetCapabilitiesSupported(true);
        EXPECT_TRUE(adapter.SupportsRequiredCapabilities(caps));
    }

    TEST(RHIAdapter, IsPolymorphic)
    {
        // RHIAdapter must be polymorphic (virtual dtor) so RHIInstance can
        // delete adapters through the base pointer.
        static_assert(std::is_polymorphic_v<RHIAdapter>,
                      "RHIAdapter must be polymorphic");

        StubInstance instance;
        RHIAdapterInfo info;
        StubAdapter adapter(instance, info);

        RHIAdapter* base = &adapter;
        EXPECT_EQ(base->GetInfo().Type, RHIAdapterType::Unknown);
    }
}
