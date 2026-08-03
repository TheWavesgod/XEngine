// New XEngineVulkanRHI library translation unit.
//
// This file is the only entry point of XEngineVulkanRHI that is built
// today. Vulkan / volk / VMA headers must NOT be included here at this
// stage because the new backend is still a skeleton. Later stages will
// either bring volk.h into a Private-only include block behind an
// include guard, or place the actual backend code in additional
// translation units under Private/ — in both cases Vulkan symbols stay
// out of the new public surface.
//
// The functions exposed below match the version probes of XEngineRHI so
// that tests can verify that the in-tree new VulkanRHI target is being
// linked rather than a future shared library or a stale object cache.

#include <XEngine/VulkanRHI/Base.h>

// Version probes, like XEngineRHI's. Defined at file scope (not inside
// the XEngine namespace) so callers can resolve them with `extern "C"`
// linkage as plain C symbols.
extern "C" XENGINE_VULKAN_RHI_API std::uint32_t XEngineVulkanRHI_GetVersionMajor()
{
    return XEngine::VulkanRHIVersionMajor;
}

extern "C" XENGINE_VULKAN_RHI_API std::uint32_t XEngineVulkanRHI_GetVersionMinor()
{
    return XEngine::VulkanRHIVersionMinor;
}

extern "C" XENGINE_VULKAN_RHI_API std::uint32_t XEngineVulkanRHI_GetVersionPatch()
{
    return XEngine::VulkanRHIVersionPatch;
}
