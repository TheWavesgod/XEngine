# CSM_01 — Detailed Current Code Review

## Goal

This document captures the **review** portion of the Stage 9 V0 task. It answers the eight review questions explicitly requested in `Docs/prompts.md`.

## 1. What is already implemented?

Most of the CSM math and the data shape is already in place. Specifically:

- `RenderShadowCascade`, `RenderDirectionalShadowFrameData`, `RenderShadowFrameData` declared in `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h`.
- `GPUCascadeShadowData` and `GPUShadowData` declared in `Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h` with explicit `static_assert` size checks.
- `DirectionalShadowSettings`, `ShadowSettings`, `RendererSettings` in `Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererSettings.h`.
- `ShadowDebugSettings` in `Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererDebugSettings.h`. `RendererDebugSettings::Shadows` field is reserved for Stage 9.
- `DirectionalShadowPlanDesc` and `DirectionalShadowPlanner::BuildPlan` are fully implemented in `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.{h,cpp}`:
  - Cascade split calculation with practical `(log, uniform, lerp)` scheme.
  - World-space camera frustum corners via inverse `Projection * View` and Vulkan `z ∈ [0,1]` NDC.
  - Per-cascade sub-frustum corners by linear interpolation along `near → far` rays.
  - Bounding-sphere center + radius (with `ceil(r * 16) / 16` quantization).
  - Light basis (forward, right, up) from the light direction, with a fallback to `+Y` if the light is nearly vertical.
  - Texel-snap offset that adjusts the light view's translation column.
  - Reverse-Z orthographic projection.
  - Filling all fields of `RenderShadowCascade` (matrices, splits, layer index, resolution, `ShadowMapSize`, `BiasParams`, `WorldBounds`, `LightSpaceBounds`).
- `RenderShadowManager` and `ShadowResourceCache` interfaces declared in their respective headers with full method signatures.
- `ShadowResourceCache::DirectionalShadowResources` already models the recommended shape: `Texture` + `SampledView` + `LayerDepthViews[MaxShadowCascades]` + `Sampler`.

The RHI layer is already cleaned up. All the prerequisites from the RHI cleanup task are in place. There is no `GetDefaultView`, no `dynamic_cast<VulkanX>`, no public Vulkan types in RHI headers, and `VulkanNativeContext` is the only narrow bridge.

## 2. What is correct and should be kept?

| Area | Why keep |
|------|----------|
| `RenderShadowCascade` 16-byte alignment + standard layout | GPU-visible struct correctness. |
| `MaxShadowCascades = 4` constant in `RenderShadowType.h` | Small enough for uniform-array indexing in Slang. |
| `RenderResourceContext` not yet holding `RenderShadowManager*` | Correct — kept in `RenderSystem` as a long-lived manager and exposed through context, not owned by `RenderResourceContext`. (Will be added with a forward declaration, not a value field.) |
| `DirectionalShadowPlanDesc::ReverseZ = true` default | Matches Vulkan best practice and the existing `OrthographicLH_ZO` near/far swap. |
| `DirectionalShadowPlanner` sphere-bounds approach | Simpler and more numerically stable than a tight AABB-fit. Texel snap remains stable. |
| `RenderShadowFrameData::Directional` (single direction) | V0 supports one shadow-casting directional. Spot/point deferred. |
| Texel snap with `ceil(r * 16) / 16` quantization | Prevents sub-pixel swimming. Conservative but cheap. |

## 3. What is incomplete?

| File | Status | Required for V0 |
|------|--------|-----------------|
| `RenderShadowManager.cpp` | Empty | Implement `PrepareFrame`, `FillGPUShadowData`, `GetFrameData`, `HasDirectionalShadow`. |
| `ShadowResourceCache.cpp` | Empty | Implement texture-array creation, view creation, sampler creation, recreation on desc change. |
| `ShadowDepthPass.{h,cpp}` | Header empty, cpp missing | Implement `AddShadowDepthPass` that loops cascades, binds cascade `LightViewProjection`, draws shadow-casting objects. |
| `DepthOnly.slang` | Empty | Implement the depth-only VS/FS for the shadow pass. |
| `ShadowTypes.slang` | Missing | Create. Mirror `GPUShadowData` in Slang. |
| `ShadowSampling.slang` | Missing | Create. PCF3x3 + cascade selection. |
| `ForwardPBR.slang` | No CSM | Apply `1 - shadowFactor` to direct lighting. |
| `GPUFrameData` | Does not include shadow | Add `GPUShadowData Shadow;` to `Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUFrameTypes.h` and `Engine/Shaders/Common/Types.slang`. |
| `RenderResourceContext` | Missing `RenderShadowManager*` | Add pointer field. |
| `ForwardRenderPipeline` | Does not call `RenderShadowManager::PrepareFrame` or add `ShadowDepthPass` | Wire in. |
| `RenderSystem` | Does not own `RenderShadowManager` | Own + initialize + shutdown. |
| Editor debug panel | No shadow UI | Add toggles. |
| `RendererDebugSettings::VisualizeCascades` | Not yet plumbed into shader | Add `g_FrameData.Shadow.ShadowParams.w` to the shader, fragment returns cascade color when set. |
| `RenderFrameResources` `Update` | Does not include shadow data | Add `shadow` build and `shadow.Update` step (or call `RenderShadowManager::FillGPUShadowData` after `BuildGPUFrameData`). |

## 4. What is in the wrong layer?

- **None currently.** The cleanup direction is followed. RHI knows nothing about cascades. The shadow files are all in the Renderer module.

## 5. What conflicts with the current RHI cleanup direction?

- **Nothing architectural.** The only outstanding concerns are:
  - `ShadowResourceCache` will own a `RHITexture` + `RHITextureView` array. This is the **recommended** pattern in the cleanup spec — explicitly the `DirectionalShadowResources` struct. It is not the forbidden `RHITexture`-internal view cache. No conflict.
  - `RHITextureView` is created via `RHIResourceFactory::CreateTextureView`. No `GetDefaultView` is used. No conflict.

## 6. What should be deleted or simplified?

- **Already done** in the RHI cleanup pass:
  - Removed dead `XE_AssertBackendMatches` from `RHIResource.h/.cpp`.
  - Removed unused `m_Desc = {}` from `VulkanTextureView` destructor.
  - Removed legacy commented-out struct + `static_assert` block from `RenderShadowType.h`.
  - Improved `RHIResourceFactory::CreateBuffer` error message.
- **Cleanup candidates** during Stage 9 implementation (do these as you go, not as a separate pass):
  - The `RenderShadowManager::PrepareDirectionalShadow` private helper can be the only entry point for shadow work. Do not add `PrepareSpotShadow` etc. in V0.
  - The `ShadowResourceCache` should not own a `RenderScene` reference. It only manages GPU resources. (Already correct in the header.)

## 7. What should be fixed before continuing CSM?

- **The `AABB::FromPoints` bug** in `DirectionalShadowPlanner.cpp` line 376 has been replaced with a `Min/Max` loop. The build now compiles. The current code is the correct baseline.
- **No other pre-existing issues** block CSM work.

## 8. What files are risky and need careful manual implementation?

| File | Risk | Reason |
|------|------|--------|
| `DirectionalShadowPlanner.cpp` | High (math) | Coordinate-convention mistakes silently produce wrong shadows. Keep using `Math::OrthographicLH_ZO` and the existing basis code. |
| `ShadowDepthPass.cpp` | Medium | Must use `RHIRenderOutputDesc` with `HasColorAttachment = false`, `DepthFormat = D32Float`, and bind per-layer depth views. |
| `DepthOnly.slang` | Medium | Must output `SV_Depth` only. No color. Push constants for `Model` + `LightViewProjection`. |
| `ShadowSampling.slang` | High (sampling math) | Reverse-Z, PCF tap count, normal-bias slope. |
| `ForwardPBR.slang` | Medium | Add CSM application without breaking existing PBR. |
| `RenderShadowManager.cpp` | Medium | Must call `RenderShadowManager::PrepareDirectionalShadow` only when a valid shadow-casting directional light exists; fall back to `m_FrameData.Enabled = false` otherwise. |
| `RenderFrameResources.cpp` | Low | Extend `BuildGPUFrameData` to set `data.Shadow` from `m_ShadowManager` (passed via `RenderResourceContext` or a direct method). |

## Summary Verdict

The current RHI provides everything CSM needs. The math is already there. The remaining work is: connect the planner to a real GPU resource cache, render one depth pass per cascade, upload `GPUShadowData` as part of the frame buffer, and add cascade-aware direct lighting in `ForwardPBR.slang`. None of the remaining work requires changes to the RHI public surface or to module boundaries.
