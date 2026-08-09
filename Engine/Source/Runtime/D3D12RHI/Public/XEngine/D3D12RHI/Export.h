#pragma once

// Minimal export macro for the new XEngineD3D12RHI target.
//
// Like XEngine/RHI/Export.h this is intentionally a no-op today because
// the new D3D12RHI is built as a static library / header-only
// declaration surface. It is here so that the public surface already
// reserves the symbol-export macro namespace before ABI work begins.
//
// D3D12 / DXGI / WRL headers MUST NOT be reachable through this header —
// they belong in the Private/ tree only.

#if defined(_WIN32)
    #if defined(XENGINE_D3D12_RHI_STATIC)
        #define XENGINE_D3D12_RHI_API
    #else
        #if defined(XENGINE_D3D12_RHI_BUILDING)
            #define XENGINE_D3D12_RHI_API __declspec(dllexport)
        #else
            #define XENGINE_D3D12_RHI_API __declspec(dllimport)
        #endif
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define XENGINE_D3D12_RHI_API __attribute__((visibility("default")))
#else
    #define XENGINE_D3D12_RHI_API
#endif