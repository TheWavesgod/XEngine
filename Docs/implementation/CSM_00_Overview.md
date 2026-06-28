# CSM_00 — Overview & Current Code Review

## Goal

Stage 9 V0 ships a directional Cascaded Shadow Map (CSM) pipeline that:

1. Allocates one 2D array depth texture with one layer per cascade.
2. Computes per-cascade light view-projection matrices on the CPU using the left-handed XEngine convention.
3. Writes per-cascade depth to the shadow array.
4. Samples the appropriate cascade in `ForwardPBR.slang` and multiplies the resulting visibility into the direct lighting term.

This is a V0 implementation. It must be simple, follow the architecture in `Docs/Project_Cache.md`, and prepare for future stages (CSM tuning, VSM, spot/point shadows, RenderFeature, RenderGraph V1).

## Scope of Stage 9 V0

In scope:

- One directional light with CSM (texture array storage).
- Up to 4 cascades (`MaxShadowCascades = 4`).
- Per-cascade orthographic projection in light space, ZO depth (0..1).
- Stable cascade texel snapping.
- PCF 3x3 sampling fallback (PCSS deferred to Stage 10+).
- Editor debug toggles: `VisualizeCascades`, `FreezeShadowMatrices`, `ShowShadowMap`, `DebugCascadeLayer`.

Explicit non-goals (deferred):

- Bindless descriptor indexing.
- Ray-traced shadows.
- VSM / EVSM.
- Spot and point light shadows.
- Async compute shadow pass.
- RenderGraph transient resource allocator.
- Full automatic resource state tracker (Vulkan layout transitions remain per-pass explicit).

## Current Code Review (as of HEAD)

### What's already implemented

| Area | State | Notes |
|------|-------|-------|
| `RHIResourceFactory` (texture, view, sampler, shader, bind group, pipeline) | Done | NVI pattern, validates descs. |
| `RHITexture` + `RHITextureView` split | Done | No `GetDefaultView()` on `RHITexture`. |
| `RHIUtils.h/.cpp` (format helpers, validators) | Done | `IsDepthFormat`, `NormalizeTextureViewDesc`, `ValidateTextureDesc`, `ValidateTextureViewDesc`. |
| `VulkanUtils.h/.cpp` (RHI→Vulkan conversions) | Done | All `ToVulkan*` helpers centralized. |
| `VulkanCheckedCast` (debug-checked `static_cast`) | Done | Used in `VulkanCommandList`, `VulkanDescriptor`, `VulkanPipeline`, `VulkanTextureView`. |
| `Math::BuildViewMatrixLH_XForward` / `LookAtLH_XForward` / `PerspectiveLH_ZO` / `OrthographicLH_ZO` | Done | Left-handed, +X forward, ZO depth. |
| `Math::CombineAABB` / `Math::TransformAABB` | Done | `XEngine::AABB` has `Encapsulate` but **no `FromPoints`**. |
| `RendererSettings` / `RendererDebugSettings` (CSM fields) | Done | `DirectionalShadowSettings`, `ShadowDebugSettings` already declared. |
| `RenderShadowType.h` (`RenderDirectionalShadowFrameData`, `RenderShadowCascade`, `GPUShadowData`/`GPUCascadeShadowData`) | Done | See [CSM_02](CSM_02_Settings_Types_GPUData.md). |
| `DirectionalShadowPlanner` | Mostly done | Splits, frustum corners, sphere bounds, texel snap, light basis, ortho projection. **Has a pre-existing `AABB::FromPoints` bug already patched** by replacing the call with `Min/Max` loop. |
| `RenderShadowManager` (header) | Stub interface only | `.cpp` is empty (`void RenderShadowManager::PrepareFrame` not implemented). |
| `ShadowResourceCache` (header) | Stub interface only | `.cpp` is empty. |
| `ShadowDepthPass.h/.cpp` | Empty | File is `// TODO: Add shader code for DepthOnly.slang.` and `#pragma once` only. |
| `DepthOnly.slang` | Empty | Need to write the full depth-only VS/FS. |
| `ShadowTypes.slang` / `ShadowSampling.slang` | Missing | Need to create. |
| `ForwardPBR.slang` shadow sampling | Missing | Currently has no `cascaded` term; needs CSM-aware direct lighting. |
| `GPUFrameData` containing shadow data | Partial | `GPUShadowData` exists in `ShaderInterop/GPUShadowTypes.h` but is **not yet** included in `GPUFrameData`. |
| `RenderSystem` integration of `RenderShadowManager` | Missing | `RenderSystem::Render` does not call `m_ShadowManager.PrepareFrame` yet. |
| `RenderResourceContext` carrying `RenderShadowManager*` | Missing | Need to add the field. |
| Editor `RendererDebugPanel` controls for shadows | Partial | Toggles exist in `RendererDebugSettings` but panel UI is not wired. |

### Conflicts with the cleanup direction (resolved)

| Concern | Status |
|---------|--------|
| `RHITexture::GetDefaultView()` | Not present. |
| Texture-local view cache | Not present. |
| `dynamic_cast<VulkanX*>` in RHI hot paths | Not present. `CheckedVulkanCast` used instead. |
| Generic RHI native handle escape hatch | Replaced by narrow `VulkanNativeContext` / `VulkanNativeTextureBinding` in `RHI/Public/XEngine/RHI/Native/`. |
| Public RHI headers leaking `volk.h` | None. `RHIDevice.h` and `Resources/*.h` are clean. |
| `RHIRenderOutputDesc` with `RHITexture*` | Already uses `RHITextureView*`. |
| `RHIBindingResource` with `RHITexture*` | Already uses `RHITextureView*`. |
| CSM-specific helpers inside `RHITexture` | Not present. |
| RHI knowing about cascades | Not present. |

### Files needing changes for Stage 9 V0

The following files exist and need to be filled in or modified. The remaining CSM documents describe each in detail.

**Add (new):**
- `Engine/Shaders/Lighting/ShadowTypes.slang`
- `Engine/Shaders/Lighting/ShadowSampling.slang`
- `Engine/Shaders/Passes/ShadowDepth.slang` (or replace the existing `DepthOnly.slang`)

**Fill in (currently empty):**
- `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp`
- `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp`
- `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp`
- `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.h`
- `Engine/Shaders/Passes/DepthOnly.slang`

**Modify:**
- `Engine/Source/Runtime/Renderer/Private/Resources/RenderResourceContext.h` — add `RenderShadowManager*`.
- `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.h/.cpp` — add `GPUShadowData` upload.
- `Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUFrameTypes.h` — include shadow field.
- `Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp` — call `RenderShadowManager::PrepareFrame`; add `AddShadowDepthPass` before `AddForwardOpaquePass`.
- `Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp` — own a `RenderShadowManager` and wire it into `RenderResourceContext`.
- `Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp` — bind frame bind group that includes shadow data.
- `Engine/Shaders/Passes/ForwardPBR.slang` — sample CSM and modulate direct lighting.
- `Engine/Shaders/Common/Types.slang` — add `GPUFrameData.Shadow`.
- `Engine/Source/Editor/Private/Panels/RendererDebugPanel.cpp` — wire shadow debug toggles.

## Coordinate System Reminder

XEngine uses a left-handed coordinate system:

```text
+X = Forward
+Y = Right
+Z = Up
```

Camera-space convention:

```text
Camera looks along its local +X (forward).
Camera local +Y is right, local +Z is up.
```

`BuildViewMatrixLH_XForward(position, rotation)` returns the view matrix for this convention. Light-space convention (used by `DirectionalShadowPlanner`) is the same: the light "camera" looks along its +X (the light's forward direction). This matches `BuildViewMatrixLH_XForward` and the orthographic projection `OrthographicLH_ZO` exactly. **Do not** use `glm::lookAtRH` or any right-handed `lookAt` without inverting the Z basis.

The existing code in `DirectionalShadowPlanner.cpp` already builds the light view matrix from the basis `[Right, Up, Forward, Position]` and inverts it. This is functionally equivalent to using `BuildViewMatrixLH_XForward` if you feed it a `Quat` whose forward maps to the light basis. See [CSM_04](CSM_04_DirectionalShadowPlanner_Coordinates.md) for the recommended formulation.

## Important Math Helpers Already Available

- `Math::LookAtLH_XForward(eye, target, up)` — `Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h`.
- `Math::BuildViewMatrixLH_XForward(position, rotation)` — same file.
- `Math::PerspectiveLH_ZO(fov, aspect, near, far)` — same file.
- `Math::OrthographicLH_ZO(left, right, bottom, top, near, far)` — same file.
- `Math::Inverse(m)`, `Math::Lerp(a, b, t)`, `Math::Clamp(v, lo, hi)`, `Math::Min(a, b)`, `Math::Max(a, b)`, `Math::Length(v)`, `Math::Normalize(v)`, `Math::Cross(a, b)`.
- `XEngine::AABB { Vec3 Min, Max; }` with `Encapsulate(point)`, `Encapsulate(other)`, `GetCenter()`, `GetExtents()`, `GetRadius()`.
- `Math::TransformAABB(bounds, matrix)`, `Math::CombineAABB(a, b)`.

There is **no** `AABB::FromPoints`. Use the `Min/Max` loop pattern documented in [CSM_04](CSM_04_DirectionalShadowPlanner_Coordinates.md).

## Risks and Watch-Outs

1. **Texture array depth attachments** require a `VK_KHR_dynamic_rendering` pipeline with no color attachment and `pDepthAttachment != nullptr`. `RHIGraphicsPipelineDesc::HasColorAttachment = false` plus `DepthFormat = D32Float` plus `EnableDepthTest/EnableDepthWrite = true` is the right combination. Verify with RenderDoc that each cascade writes one layer.

2. **Per-layer depth attachment views** must be created with `viewDesc.ViewDimension = Texture2DArray` and `viewDesc.ArrayLayerCount = 1` and `viewDesc.BaseArrayLayer = cascadeIndex`. The validation in `RHIResourceFactory::CreateTextureView` already covers this. Whole-array sampled view uses `ArrayLayerCount = 0` (all layers).

3. **Reverse-Z** (Vulkan default) means `OrthographicLH_ZO` must be called with `near > far` to put the close depth at 1.0 and the far depth at 0.0. The current `DirectionalShadowPlanner` already handles this. Sampling code in the shader compares with `>=` and treats the depth as `[0..1]` directly. **Do not** invert the depth comparison or sample into `[-1..1]` (OpenGL) without first applying the projection's reverse Z explicitly.

4. **Texel snap must be done on the light view, not the world translation.** The current `ComputeTexelSnapOffset` returns a world-space offset that is subtracted from the light view's translation column. The math is already correct in the planner. Do not reintroduce the `SnapToTexelGrid` variant that just returns `worldCenter` unchanged.

5. **PCF3x3** is hard-coded for V0. It samples 9 taps with offsets of ±1 texel from the projected UV, divides by 9. It must use `SampleShadow`/`SampleCmpLevel` (`Texture2DArray::SampleCmp`) — not `Sample` then compare in HLSL — so that the GPU does the depth comparison with PCF. The Slang intrinsic is `g_ShadowMap.SampleCmpLevel(g_ShadowSampler, float3(uv, layer), refDepth)`.

6. **Cascade selection** is `for (uint i = 0; i < cascadeCount; ++i) { if (viewDepth < cascade[i].SplitFar) return i; }`. The split distances are stored as positive camera-view-space depth. The first matching cascade is used. Past the last cascade the final cascade is used.

7. **Bias application order** is `effectiveDepth = refDepth + depthBias + slope * normalBias`. The shadow test is `shadowMap[uv,layer] >= effectiveDepth`. The slope is `dot(N, lightDir)` (for normal bias). Use light-space normal for normal bias to avoid world-space ambiguity.

8. **Editor toggles** must not save to `.xscene`. They live in `RendererDebugSettings`, which is in-memory only.

9. **FreezeShadowMatrices** keeps the last computed cascade matrices across frames. Implementation: copy `m_FrameData` into `m_FrozenFrameData` on the frame the toggle flips on, and use `m_FrozenFrameData` for `FillGPUShadowData` while the toggle is on. Clear the snapshot when the toggle goes off.

10. **Do not** add `RHIBufferView`, full automatic resource state tracking, or bindless indexing. The RHI does not know what a "cascade" is.

## What This Document Does Not Cover

- Specific file/function signatures for each Stage 9 component — see the per-component documents.
- Shader code — see [CSM_08](CSM_08_Shaders.md).
- Editor UI for the debug panel — see [CSM_09](CSM_09_Editor_Debug_Validation.md).
- The order in which to implement — see [CSM_10](CSM_10_Implementation_Order_Checklist.md).
