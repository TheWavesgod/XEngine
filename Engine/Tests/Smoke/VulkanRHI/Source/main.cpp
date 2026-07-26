// Smoke executable skeleton for the Vulkan RHI path.
//
// Right now this program does the smallest possible thing: it prints a banner
// and returns 0. The real smoke flow (SDL3 window -> Vulkan instance ->
// surface -> swapchain -> clear -> present -> shutdown) will be added in a
// later stage once XEngineVulkanRHI exists as a separate target with a
// stable initialization API.
//
// The exit code is intentionally checked by CTest so this skeleton target is
// registered as a passing test.

#include <XEngine/Core/Types.h>

#include <cstdio>

int main()
{
    std::printf("XEngineVulkanRHISmokeTest skeleton OK.\n");
    return 0;
}