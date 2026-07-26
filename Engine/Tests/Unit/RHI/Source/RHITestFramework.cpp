// Minimal unit test for the RHI layer.
//
// The purpose of this file is to prove that:
//   1. GoogleTest is correctly wired up via Tests/CMakeLists.txt.
//   2. XEngineRHI headers are visible and linkable from a unit test target.
//   3. XEngineCoreRuntime / XEngineFoundation are visible and linkable.
//
// No real GPU / windowing / API initialization happens here. Backend-specific
// tests live under Tests/Integration/VulkanRHI and Tests/Smoke/VulkanRHI.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/Test/TestSupport.h>

// Public backend-agnostic RHI header. Its forward declarations must resolve
// even though we never instantiate any backend objects in a unit test.
#include <XEngine/RHI/RHIDevice.h>

namespace XEngine
{
    namespace
    {
        // Compile-time sanity check that the alias set we rely on in RHI public
        // headers is the same one the test executable sees.
        static_assert(sizeof(XEngine::u32) == 4, "u32 must be 4 bytes");
        static_assert(sizeof(XEngine::u8) == 1, "u8 must be 1 byte");

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