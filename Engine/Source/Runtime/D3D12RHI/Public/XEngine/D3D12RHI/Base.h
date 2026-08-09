#pragma once

// Public surface of the new XEngineD3D12RHI target.
//
// Mirrors XEngineVulkanRHI/Public/XEngine/VulkanRHI/Base.h. The D3D12
// backend establishes the same boundary as the Vulkan one: a clean public
// header surface that contains zero D3D12 / DXGI / WRL symbols, leaving
// those to the Private/ tree.
//
// This header is deliberately small. It does NOT include <d3d12.h>,
// <dxgi1_4.h>, <wrl/client.h>, or any D3D12 / DXGI definitions. Those
// are confined to the Private/ directory of this target.

#include <XEngine/Core/Types.h>

#include <XEngine/D3D12RHI/Export.h>

#include <cstdint>

namespace XEngine
{
    // Version sentinel for the new XEngineD3D12RHI target. Bumping this
    // triple is the explicit signal that an ABI-incompatible change has
    // landed in the backend. Skeleton-phase value matches the protocol
    // M3 sentinel so the in-tree targets stay aligned.
    inline constexpr std::uint32_t D3D12RHIVersionMajor = 0;
    inline constexpr std::uint32_t D3D12RHIVersionMinor = 3;  // M3
    inline constexpr std::uint32_t D3D12RHIVersionPatch = 0;
} // namespace XEngine

// Version-probe symbol exports with explicit C linkage, mirroring
// XEngineVulkanRHI's. Implementations live in Private/D3D12RHI.cpp.
extern "C" XENGINE_D3D12_RHI_API std::uint32_t XEngineD3D12RHI_GetVersionMajor();
extern "C" XENGINE_D3D12_RHI_API std::uint32_t XEngineD3D12RHI_GetVersionMinor();
extern "C" XENGINE_D3D12_RHI_API std::uint32_t XEngineD3D12RHI_GetVersionPatch();