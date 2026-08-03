#pragma once

// Minimal export macro for the new XEngineRHI target.
//
// The new RHI is currently built only as a static library / header-only
// declaration surface, so the export macro below is intentionally a no-op.
// It is kept here so that future shared-library / ABI work does not need to
// introduce a public macro after the fact. D3D12 / Metal / additional
// protocols will pick this up unchanged.
//
// Do NOT add Vulkan, volk, VMA, SDL3, Shader, or Renderer includes here.

#if defined(_WIN32)
    #if defined(XENGINE_RHI_STATIC)
        #define XENGINE_RHI_API
    #else
        #if defined(XENGINE_RHI_BUILDING)
            #define XENGINE_RHI_API __declspec(dllexport)
        #else
            #define XENGINE_RHI_API __declspec(dllimport)
        #endif
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define XENGINE_RHI_API __attribute__((visibility("default")))
#else
    #define XENGINE_RHI_API
#endif
