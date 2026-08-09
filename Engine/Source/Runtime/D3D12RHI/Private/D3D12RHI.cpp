// New XEngineD3D12RHI library translation unit.
//
// Mirrors XEngineVulkanRHI/Private/VulkanRHI.cpp. This file is the only
// entry point of XEngineD3D12RHI that is built today. D3D12 / DXGI / WRL
// headers must NOT be included here at this stage because the new backend
// is still a skeleton. Later stages will bring d3d12.h / dxgi1_4.h into a
// Private-only include block behind an include guard, or place the actual
// backend code in additional translation units under Private/ — in both
// cases D3D12 / DXGI symbols stay out of the new public surface.
//
// The functions exposed below match the version probes of XEngineRHI so
// that tests can verify that the in-tree new D3D12RHI target is being
// linked rather than a future shared library or a stale object cache.

#include <XEngine/D3D12RHI/Base.h>

// Version probes, like XEngineRHI's. Defined at file scope (not inside
// the XEngine namespace) so callers can resolve them with `extern "C"`
// linkage as plain C symbols.
extern "C" XENGINE_D3D12_RHI_API std::uint32_t XEngineD3D12RHI_GetVersionMajor()
{
    return XEngine::D3D12RHIVersionMajor;
}

extern "C" XENGINE_D3D12_RHI_API std::uint32_t XEngineD3D12RHI_GetVersionMinor()
{
    return XEngine::D3D12RHIVersionMinor;
}

extern "C" XENGINE_D3D12_RHI_API std::uint32_t XEngineD3D12RHI_GetVersionPatch()
{
    return XEngine::D3D12RHIVersionPatch;
}