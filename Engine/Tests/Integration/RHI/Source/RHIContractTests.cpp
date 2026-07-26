// Cross-backend RHI contract placeholder.
//
// This file currently only proves that an integration test binary can be
// configured and linked. Real contract tests will be added once the RHI
// surface is stable enough that they can express backend-independent
// invariants (resource handles, submission ordering, queue-family semantics,
// etc.).
//
// Note: this binary does NOT currently spin up a real RHI device. That
// happens in the per-backend tests under Tests/Integration/VulkanRHI.

#include <gtest/gtest.h>

#include <XEngine/Test/TestSupport.h>

TEST(RHIIntegration, ContractLayerBuilds)
{
    EXPECT_EQ(XEngine::Test::TestSupportAbiVersion, 0);
}