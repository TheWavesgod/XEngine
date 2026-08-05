// VulkanRHI public backend factory API.
//
// Phase 1 (M0-M3 backend): exposes VulkanInstance creation so consumers
// (apps, integration tests) can get a Vulkan-backed RHIInstance without
// reaching into the Private/ directory. The factory returns nullptr if
// the Vulkan SDK is unavailable or instance creation fails.
//
// Future phases (M4+, M11+) will add factories for other backend types
// (physical-device enumeration via the instance, etc.).

#pragma once

#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>

#include <memory>

namespace XEngine
{
    // Concrete RHIInstance implementation for the Vulkan backend.
    class VulkanInstance;

    namespace VulkanRHI
    {
        // Creates a VkInstance via volk, wraps it in a VulkanInstance
        // (which is an RHIInstance subclass), and returns it.
        // Returns nullptr if Vulkan SDK is unavailable or instance creation fails.
        std::unique_ptr<RHIInstance> CreateInstance(const RHIInstanceDesc& desc);
    }
}
