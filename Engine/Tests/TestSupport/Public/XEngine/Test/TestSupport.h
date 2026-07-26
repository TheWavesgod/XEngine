#pragma once

// TestSupport is a skeleton target. Real helpers (path helpers, temp-file
// helpers, GPU validation message collectors, Vulkan test contexts, readback
// helpers) will land here in later stages. For now this header only exists so
// other test targets can already include <XEngine/Test/TestSupport.h> and link
// against XEngineTestSupport.

namespace XEngine::Test
{
    // Version sentinel so test consumers can assert they are linking against
    // the in-tree TestSupport rather than an accidental third-party one.
    inline constexpr int TestSupportAbiVersion = 0;
} // namespace XEngine::Test