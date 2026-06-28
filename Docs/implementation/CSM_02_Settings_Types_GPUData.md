# CSM_02 — Settings, Types, and GPU Data

## Goal

Define the **shape** of every data structure used by Stage 9. The data is split into three layers:

1. **Settings** (in-memory, editor-controlled) — `DirectionalShadowSettings`, `ShadowSettings`, `ShadowDebugSettings`, `RendererSettings`, `RendererDebugSettings`.
2. **CPU renderer frame data** (in-memory, recomputed per frame) — `RenderDirectionalShadowFrameData`, `RenderShadowFrameData`, `RenderShadowCascade`, `DirectionalShadowPlanDesc`.
3. **GPU-visible shadow data** (uploaded each frame as part of `GPUFrameData`) — `GPUShadowData`, `GPUCascadeShadowData`.

This document describes the final shape and the fields that already exist vs. fields that need to be added.

## Settings Layer

### Already implemented (no changes needed)

Files:

- `Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererSettings.h`
- `Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererDebugSettings.h`

```cpp
// RendererSettings.h
enum class DirectionalShadowTechnique : u8 { None, CascadedShadowMaps };
enum class ShadowMapStorageMode : u8   { Texture2DArray, Atlas };
enum class ShadowFilterMode : u8       { Hard, PCF3x3, PCF5x5, PCSS };

struct DirectionalShadowSettings
{
    bool Enabled = true;
    DirectionalShadowTechnique Technique = DirectionalShadowTechnique::CascadedShadowMaps;
    ShadowMapStorageMode       StorageMode = ShadowMapStorageMode::Texture2DArray;
    ShadowFilterMode           FilterMode  = ShadowFilterMode::PCF3x3;

    u32  CascadeCount = 4;
    u32  Resolution   = 2048;
    float SplitLambda = 0.5f;
    float DepthBias   = 0.003f;
    float NormalBias  = 0.0f;
    bool  StabilizeCascades = true;
};

struct ShadowSettings
{
    DirectionalShadowSettings Directional;
};

struct RendererSettings
{
    ShadowSettings Shadows;
};
```

```cpp
// RendererDebugSettings.h
struct ShadowDebugSettings
{
    bool VisualizeCascades = false;
    bool FreezeShadowMatrices = false;
    bool ShowShadowMap = false;
    u32  DebugCascadeLayer = 0;
};

struct RendererDebugSettings
{
    // existing fields ...
    bool VisualizeCascades = false;     // already mirrored at top level
    bool FreezeShadowMatrices = false; // already mirrored at top level
    ShadowDebugSettings Shadows;
};
```

The top-level `VisualizeCascades` and `FreezeShadowMatrices` mirror is intentional: it lets editor code read or write either the `RendererDebugSettings::Shadows` block or the top-level shortcut. The shader only needs one path. **Recommendation:** keep both, set both in the same place. Renderer pipeline reads `RendererDebugSettings::Shadows` exclusively.

### `DirectionalShadowSettings::StabilizeCascades` is already present. It maps to `DirectionalShadowPlanDesc::StabilizeCascades` and is forwarded into `DirectionalShadowPlanner::BuildPlan`.

## CPU Frame Data Layer

### Already implemented (no changes needed)

File: `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h`

```cpp
static constexpr u32 MaxShadowCascades = 4;

struct RenderShadowCascade
{
    Mat4 LightView         = Mat4(1.0f);
    Mat4 LightProjection   = Mat4(1.0f);
    Mat4 LightViewProjection = Mat4(1.0f);

    float SplitNear = 0.0f;
    float SplitFar  = 0.0f;

    u32   LayerIndex = 0;
    u32   Resolution = 0;

    Vec4  ShadowMapSize = Vec4(0.0f);
    // x = depth bias, y = normal bias, z = slope-scaled bias, w = reserved.
    Vec4  BiasParams = Vec4(0.0f);

    AABB  WorldBounds;
    AABB  LightSpaceBounds;
};

struct RenderDirectionalShadowFrameData
{
    bool  Enabled      = false;
    u32   CascadeCount = 0;

    RHITexture*     ShadowTexture      = nullptr;
    RHITextureView* SampledView        = nullptr;
    std::array<RHITextureView*, MaxShadowCascades> CascadeDepthViews {};
    RHISampler*     Sampler            = nullptr;

    std::array<RenderShadowCascade, MaxShadowCascades> Cascades;
};

struct RenderShadowFrameData
{
    RenderDirectionalShadowFrameData Directional;
};
```

### Field-by-field rationale

| Field | Why |
|-------|-----|
| `ShadowTexture` | Whole shadow array; held for lifetime checks; not used for sampling directly. |
| `SampledView` | Whole-array `Texture2DArray` sampled view. Used by `ForwardPBR.slang`. |
| `CascadeDepthViews[i]` | One `Texture2DArray` view per cascade with `ArrayLayerCount = 1, BaseArrayLayer = i`. Used as depth attachment by `ShadowDepthPass`. |
| `Sampler` | Comparison sampler (`CompareEnable = VK_TRUE`, `CompareOp = LessOrEqual`). Held once for all cascades. |
| `Cascades[i].ShadowMapSize` | `xy = (res, res)`, `zw = (1/res, 1/res)`. |
| `Cascades[i].BiasParams` | Mirrored on the CPU so `RenderShadowManager::FillGPUShadowData` can copy without re-deriving. |
| `Cascades[i].LightSpaceBounds` | Useful for debug visualization and future frustum culling. Not used by `ForwardPBR.slang` directly. |
| `Cascades[i].WorldBounds` | Useful for debug visualization and future culling. |

### Ownership

- `ShadowResourceCache` owns the `shared_ptr<RHITexture>`, the `shared_ptr<RHITextureView>` sampled view, the per-layer `shared_ptr<RHITextureView>` array, and the `shared_ptr<RHISampler>`. It exposes them as raw pointers via `DirectionalShadowResources` to `RenderShadowFrameData` (a non-owning view).
- `RenderShadowManager` owns the `ShadowResourceCache` and stores a `RenderShadowFrameData` member for the current frame, plus a `RenderShadowFrameData m_FrozenFrameData` for `FreezeShadowMatrices`.

## GPU Data Layer

### Already implemented

File: `Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h`

```cpp
struct alignas(16) GPUCascadeShadowData
{
    Mat4 LightViewProjection = Mat4(1.0f);
    // x = split far in view-space depth
    // y = depth bias
    // z = normal bias
    // w = texel size
    Vec4 Params = Vec4(0.0f);
};

struct alignas(16) GPUShadowData
{
    // x = enabled
    // y = cascade count
    // z = shadow resolution
    // w = visualize cascades
    Vec4 ShadowParams = Vec4(0.0f);
    std::array<GPUCascadeShadowData, MaxShadowCascades> Cascades;
};
```

Static asserts:

- `sizeof(GPUCascadeShadowData) == 80` (Mat4 64 + Vec4 16).
- `sizeof(GPUShadowData) == 16 + MaxShadowCascades * 80`.

These **must remain** and **must match** the Slang-side definitions in `Engine/Shaders/Lighting/ShadowTypes.slang` (to be created).

### Required modifications

#### `Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUFrameTypes.h`

Current shape (per the current code; verify by reading the file before editing):

```cpp
struct alignas(16) GPUFrameData
{
    GPUCameraData Camera;
    GPULightingData Lighting;
};
```

Required new shape:

```cpp
struct alignas(16) GPUFrameData
{
    GPUCameraData  Camera;
    GPULightingData Lighting;
    GPUShadowData  Shadow;   // NEW
};
```

The size of `GPUFrameData` increases by `sizeof(GPUShadowData)`. The `RenderFrameResources` `m_FrameBuffers[index]` are already created with `bufferDesc.Size = sizeof(GPUFrameData)`, so they auto-resize — but the recompute is required because the `static_assert(sizeof(GPUFrameData) % 16 == 0)` must still pass.

#### `Engine/Shaders/Common/Types.slang`

Mirror the C++ layout:

```hlsl
// In Types.slang, after the existing GPUFrameData struct:

#include "Lighting/ShadowTypes.slang"

struct GPUFrameData
{
    GPUCameraData   Camera;
    GPULightingData Lighting;
    GPUShadowData   Shadow;
};
```

The new include is at the file scope, not inside any namespace.

## `DirectionalShadowPlanDesc` (already implemented)

File: `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.h`

```cpp
struct DirectionalShadowPlanDesc
{
    const RenderLight* Light = nullptr;

    Mat4 CameraView       = Mat4(1.0f);
    Mat4 CameraProjection = Mat4(1.0f);
    float CameraNear      = 0.1f;
    float CameraFar       = 1000.0f;
    Vec3  CameraPosition  = Vec3(0.0f);

    // Aggregate world-space AABB of shadow-casting OpaqueObjects.
    AABB  SceneBounds;

    u32   CascadeCount = 4;
    u32   Resolution   = 2048;

    float SplitLambda = 0.5f;
    float DepthBias   = 0.003f;
    float NormalBias  = 0.0f;

    bool StabilizeCascades = true;
    bool ReverseZ          = true;   // Vulkan default; do not change in V0.
};
```

This is filled by `RenderShadowManager::PrepareDirectionalShadow` from the active `RenderLight`, `RenderFrameContext`, the `RenderScene`, and `DirectionalShadowSettings`. No changes needed to the struct itself.

## `DirectionalShadowSettings` ↔ `DirectionalShadowPlanDesc` Mapping

| `DirectionalShadowSettings` | `DirectionalShadowPlanDesc` field |
|-----------------------------|-------------------------------------|
| `CascadeCount` | `CascadeCount` |
| `Resolution` | `Resolution` |
| `SplitLambda` | `SplitLambda` |
| `DepthBias` | `DepthBias` |
| `NormalBias` | `NormalBias` |
| `StabilizeCascades` | `StabilizeCascades` |

`FilterMode` and `StorageMode` are not used by the planner itself — they are applied at the `ShadowResourceCache` and the shader level:

- `FilterMode == PCF3x3` is the only mode implemented in V0. `Hard` is implemented as a degenerate PCF (single tap). `PCF5x5` and `PCSS` are deferred.
- `StorageMode == Texture2DArray` is the only storage implemented in V0. `Atlas` is a future optimization for multiple shadow-casting lights and is rejected at runtime with a `XENGINE_LOG_ERROR` for V0.

## Where Each Struct Lives

```text
Renderer/Public/XEngine/Renderer/RendererSettings.h         (public settings)
Renderer/Public/XEngine/Renderer/RendererDebugSettings.h    (public debug settings)

Renderer/Private/Shadows/RenderShadowType.h                 (CPU frame data)
Renderer/Private/Shadows/DirectionalShadowPlanner.h         (plan desc)
Renderer/Private/ShaderInterop/GPUShadowTypes.h             (GPU data, C++)

Engine/Shaders/Lighting/ShadowTypes.slang                   (GPU data, Slang) -- NEW
Engine/Shaders/Common/Types.slang                           (frame data aggregate)
```

Do not introduce a parallel hierarchy in `Public/`. `RenderShadowType.h` is internal because it holds raw RHI pointers and is consumed by `RenderShadowManager`/`ShadowResourceCache`.

## What This Document Does Not Do

- It does not describe how matrices are computed — see [CSM_04](CSM_04_DirectionalShadowPlanner_Coordinates.md).
- It does not describe how GPU data is uploaded — see [CSM_07](CSM_07_FrameResources_And_ForwardSampling.md).
- It does not describe shader-side definitions in detail — see [CSM_08](CSM_08_Shaders.md).
