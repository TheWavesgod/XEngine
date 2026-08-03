// Minimal unit test for the new RHI layer.
//
// The purpose of this file is to prove that:
//   1. GoogleTest is correctly wired up via Tests/CMakeLists.txt.
//   2. XEngineRHI headers are visible and linkable from a unit test target.
//   3. XEngineFoundation is visible and linkable.
//   4. The version probe defined in XEngineRHI/Private/RHI.cpp resolves
//      at link time, proving the in-tree XEngineRHI target is being
//      consumed (and not a future shared library or a stale build).
//
// No real GPU / windowing / API initialization happens here. Backend-specific
// tests live under Tests/Integration/VulkanRHI and Tests/Smoke/VulkanRHI.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/Test/TestSupport.h>

#include <cstdint>

namespace XEngine
{
    namespace
    {
        // Compile-time sanity check that the alias set we rely on in RHI
        // public headers is the same one the test executable sees.
        static_assert(sizeof(XEngine::u32) == 4, "u32 must be 4 bytes");
        static_assert(sizeof(XEngine::u8) == 1, "u8 must be 1 byte");
        static_assert(sizeof(XEngine::RHIBackend) == 1, "RHIBackend must be 1 byte");

        static_assert(XEngine::Test::TestSupportAbiVersion == 0,
                      "TestSupport ABI version drifted; bump intentionally.");
    } // namespace
} // namespace XEngine

TEST(RHITestFramework, StartsSuccessfully)
{
    EXPECT_TRUE(true);
}

TEST(RHITestFramework, TestSupportLinked)
{
    // If TestSupport is not linked, the static_assert above would already have
    // failed at compile time. The runtime check is here so the test name shows
    // up in the CTest output even on stripped builds.
    EXPECT_EQ(XEngine::Test::TestSupportAbiVersion, 0);
}

TEST(RHITestFramework, NewRHIVersionTriplesExposed)
{
    // If XEngineRHI is not actually linked, the linker would fail during the
    // build. The runtime check is here so the version triple values appear
    // in the CTest output and any future regression (e.g. accidental
    // definition drift) is caught by the test framework.
    EXPECT_EQ(XEngineRHI_GetVersionMajor(), XEngine::RHIVersionMajor);
    EXPECT_EQ(XEngineRHI_GetVersionMinor(), XEngine::RHIVersionMinor);
    EXPECT_EQ(XEngineRHI_GetVersionPatch(), XEngine::RHIVersionPatch);
}
