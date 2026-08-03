// Unit tests for RHIAdapterInfo struct.
//
// RHIAdapterInfo is a POD-style data carrier. Tests verify field defaults,
// assignment, and ABI / size stability.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIAdapter.h>      // brings RHIAdapterInfo, RHIAdapterType
#include <XEngine/RHI/RHIEnums.h>        // brings RHIAdapterPreference

#include <type_traits>

namespace
{
    using namespace XEngine;

    // ABI-size stability: both enums stay 1 byte so the struct layout
    // doesn't grow when M3 adds fields.
    static_assert(sizeof(RHIAdapterPreference) == 1,
                  "RHIAdapterPreference must stay 1 byte");
    static_assert(sizeof(RHIAdapterType) == 1,
                  "RHIAdapterType must stay 1 byte");

    // ---------------------------------------------------------------------
    TEST(RHIAdapterInfo, DefaultsAreEmpty)
    {
        RHIAdapterInfo info;
        EXPECT_TRUE(info.VendorName.empty());
        EXPECT_TRUE(info.AdapterName.empty());
        EXPECT_TRUE(info.DriverVersion.empty());
        EXPECT_TRUE(info.APIInfo.empty());
        EXPECT_EQ(info.VendorID, 0u);
        EXPECT_EQ(info.DeviceID, 0u);
        EXPECT_EQ(info.Type, RHIAdapterType::Unknown);
        EXPECT_EQ(info.DedicatedMemoryBytes, 0u);
        EXPECT_EQ(info.SharedMemoryBytes, 0u);
    }

    TEST(RHIAdapterInfo, AssignableFromLiterals)
    {
        RHIAdapterInfo info{
            .VendorName            = "NVIDIA",
            .AdapterName           = "GeForce RTX 4090",
            .DriverVersion         = "552.22",
            .APIInfo               = "Vulkan 1.3.0",
            .VendorID              = 0x10DE,
            .DeviceID              = 0x2684,
            .Type                  = RHIAdapterType::Discrete,
            .DedicatedMemoryBytes  = 24ull * 1024 * 1024 * 1024,
            .SharedMemoryBytes     = 0,
        };

        EXPECT_EQ(info.VendorName, "NVIDIA");
        EXPECT_EQ(info.AdapterName, "GeForce RTX 4090");
        EXPECT_EQ(info.Type, RHIAdapterType::Discrete);
        EXPECT_EQ(info.DedicatedMemoryBytes, 24ull * 1024 * 1024 * 1024);
    }

    TEST(RHIAdapterInfo, PreferenceValuesAreContiguous)
    {
        // Enum values are part of the engine's ABI; documenting them here
        // so any future reordering is caught by the test.
        EXPECT_EQ(static_cast<u8>(RHIAdapterPreference::Automatic), 0);
        EXPECT_EQ(static_cast<u8>(RHIAdapterPreference::HighPerformance), 1);
        EXPECT_EQ(static_cast<u8>(RHIAdapterPreference::LowPower), 2);
        EXPECT_EQ(static_cast<u8>(RHIAdapterPreference::Explicit), 3);
    }

    TEST(RHIAdapterInfo, AdapterTypeValuesAreContiguous)
    {
        EXPECT_EQ(static_cast<u8>(RHIAdapterType::Discrete), 0);
        EXPECT_EQ(static_cast<u8>(RHIAdapterType::Integrated), 1);
        EXPECT_EQ(static_cast<u8>(RHIAdapterType::CPU), 2);
        EXPECT_EQ(static_cast<u8>(RHIAdapterType::Unknown), 3);
    }
}
