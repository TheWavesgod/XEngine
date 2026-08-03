// Cross-backend RHI contract placeholder.
//
// After the rebuild this file only proves that an integration test binary
// can be configured, compiled and linked against the new XEngineRHI target.
// No real cross-backend contract is exercised yet — those tests will be
// added once the RHI surface is stable enough that they can express
// backend-independent invariants (resource handles, submission ordering,
// queue-family semantics, etc.).
//
// Note: this binary does NOT currently spin up a real RHI device. That
// happens in the per-backend tests under Tests/Integration/VulkanRHI.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/Test/TestSupport.h>

#include <cstdint>

TEST(RHIIntegration, ContractLayerBuilds)
{
    EXPECT_EQ(XEngine::Test::TestSupportAbiVersion, 0);
}

TEST(RHIIntegration, NewRHIBackendTagExposed)
{
    // Verifies that the public RHI surface exposes the backend tag at
    // link time. If XEngineRHI is ever re-anchored to a different
    // header layout, this test will fail at link time before the
    // contract body is written.
    EXPECT_EQ(static_cast<std::uint32_t>(XEngine::RHIBackend::None), 0u);
}
