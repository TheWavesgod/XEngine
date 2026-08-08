#pragma once

// Public surface of the new XEngineVulkanRHI target.
//
// The new VulkanRHI target is, at this stage, a minimal skeleton. It
// establishes that a Vulkan backend implementation will live behind
// XEngineVulkanRHI, separately from XEngineRHI, and that consumers must
// go through the (future) backend factory entry point rather than
// reaching into XEngineRHI for any Vulkan-specific surface.
//
// This header is deliberately small. It does NOT include <volk.h>,
// <vulkan/vulkan.h>, <vk_mem_alloc.h>, or any Vulkan / volk / VMA
// definitions. Those are confined to the Private/ directory of this
// target.

#include <XEngine/Core/Types.h>

#include <XEngine/VulkanRHI/Export.h>

#include <cstdint>

namespace XEngine
{
    // Version sentinel for the new XEngineVulkanRHI target. Bumping this
    // triple is the explicit signal that an ABI-incompatible change has
    // landed in the backend.
    inline constexpr std::uint32_t VulkanRHIVersionMajor = 0;
    inline constexpr std::uint32_t VulkanRHIVersionMinor = 3;
    inline constexpr std::uint32_t VulkanRHIVersionPatch = 0;
} // namespace XEngine

// Version-probe symbol exports with explicit C linkage, mirroring
// XEngineRHI's. Implementations live in Private/VulkanRHI.cpp.
extern "C" XENGINE_VULKAN_RHI_API std::uint32_t XEngineVulkanRHI_GetVersionMajor();
extern "C" XENGINE_VULKAN_RHI_API std::uint32_t XEngineVulkanRHI_GetVersionMinor();
extern "C" XENGINE_VULKAN_RHI_API std::uint32_t XEngineVulkanRHI_GetVersionPatch();
