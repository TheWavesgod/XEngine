// Smoke executable skeleton for the Vulkan RHI path.
//
// Per Docs/AI helper/prompts.md this program is currently allowed to be
// a bare executable skeleton — it must NOT create a window or a Vulkan
// instance until XEngineVulkanRHI exposes a stable initialization entry
// point. What this skeleton DOES verify today:
//
//   * XEngineVulkanRHI's public header (XEngine/VulkanRHI/Base.h) is
//     visible from this executable.
//   * XEngineRHI's public header (XEngine/RHI/Base.h) is still visible.
//   * The XEngineVulkanRHI version probe symbols resolve at link time.
//
// The exit code is intentionally checked by CTest so this skeleton
// target is registered as a passing test.

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/VulkanRHI/Base.h>

#include <cstdio>
#include <cstdlib>

int main()
{
    // Pull the version probe through the linker so a future regression
    // that breaks XEngineVulkanRHI's symbol set is caught at link time,
    // not at run time.
    const std::uint32_t major = XEngineVulkanRHI_GetVersionMajor();
    const std::uint32_t minor = XEngineVulkanRHI_GetVersionMinor();
    const std::uint32_t patch = XEngineVulkanRHI_GetVersionPatch();

    std::printf("XEngineVulkanRHISmokeTest skeleton OK. "
                "Backend=%u Version=%u.%u.%u\n",
                static_cast<unsigned>(XEngine::RHIBackend::None),
                static_cast<unsigned>(major),
                static_cast<unsigned>(minor),
                static_cast<unsigned>(patch));
    return 0;
}
