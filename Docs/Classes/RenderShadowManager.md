# RenderShadowManager

## 1. Role

Owns the per-frame directional shadow data, drives the cascade plan via
`DirectionalShadowPlanner`, and lazily allocates the cascade texture
array + sampled view + per-layer depth views + sampler through
`ShadowResourceCache`. Supports `DebugSettings.FreezeShadowMatrices` to
keep using the last good cascade.

## 2. Source Location

- Header: `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.h`
- Implementation: `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp`
- Plan data: `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h`
- Planner: `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.{h,cpp}`
- Cache: `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.{h,cpp}`

## 3. Owned State

```cpp
RenderShadowFrameData      m_FrameData;
DirectionalShadowPlanner   m_DirectionalPlanner;
ShadowResourceCache       m_ResourceCache;
bool                      m_HasFrozenData = false;
RenderShadowFrameData      m_FrozenFrameData;
```

`RenderShadowFrameData { RenderDirectionalShadowFrameData Directional; }`.

`RenderDirectionalShadowFrameData`:

```cpp
bool                          Enabled;
u32                           CascadeCount;
RHITexture*                   ShadowTexture;
RHITextureView*               SampledView;
std::array<RHITextureView*, MaxShadowCascades>  CascadeDepthViews;
RHISampler*                   Sampler;
std::array<RenderShadowCascade, MaxShadowCascades> Cascades;
```

`RenderShadowCascade`:

```cpp
Mat4 LightView, LightProjection, LightViewProjection;
float SplitNear, SplitFar;
u32  LayerIndex, Resolution;
Vec4 ShadowMapSize, BiasParams;
AABB WorldBounds, LightSpaceBounds;
```

`MaxShadowCascades = 4`.

## 4. Borrowed Dependencies

- `RHIDevice*` (passed to `PrepareFrame` and `Initialize`).
- The active `RenderScene` (read by `PrepareFrame` to find the first
  directional cast-shadow light).

## 5. Lifetime

- `Initialize` boots the cache and resets `m_FrameData`.
- `PrepareFrame` builds per-frame cascade matrices once; if
  `FreezeShadowMatrices` is on, it switches to `m_FrozenFrameData`
  instead.
- `Shutdown` closes the cache and clears state.

## 6. Callers and Used By

- `RenderSystem::Render` calls `PrepareFrame` per frame.
- `RenderFrameResources::Update` reads `m_FrameData.Directional` to fill
  `GPUShadowData` and feed `SetShadowBindings`.
- `ShadowDepthPass::Execute` reads `GetFrameData()` once per cascade.

## 7. Main Collaborators

- `DirectionalShadowPlanner` (pure math).
- `ShadowResourceCache` (texture array + views + sampler).
- `RHIDevice` (texture creation through resource factory).

## 8. Runtime Sequence

```mermaid
sequenceDiagram
    participant Sys as RenderSystem
    participant Mgr as RenderShadowManager
    participant Pl as DirectionalShadowPlanner
    participant Cache as ShadowResourceCache

    Sys->>Mgr: PrepareFrame(device, scene, frame, settings, debug)
    alt debug.FreezeShadowMatrices && m_HasFrozenData
        Mgr-->>Sys: (use m_FrozenFrameData)
    else normal
        Mgr->>Mgr: PrepareDirectionalShadow(device, ...)
        Mgr->>Cache: GetOrCreateDirectionalShadowResources(desc)
        Mgr->>Pl: BuildPlan(planDesc, m_FrameData.Directional)
        alt debug.FreezeShadowMatrices
            Mgr->>Mgr: m_FrozenFrameData = m_FrameData; m_HasFrozenData = true
        end
    end
    Mgr-->>Sys: ready
```

## 9. Important Invariants

- Cascades always refer to `cascadeCount` active layers out of
  `MaxShadowCascades = 4`. Inactive layers keep stale data; the shader
  path reads only `Cascades[shadowParams.y]`.
- `LightViewProjection` is the only cascade matrix that reaches the
  shader today (`ShadowTypes.slang::GPUCascadeShadowData`).
- The freeze mode snapshots one frame; toggling off restores live
  updates.

## 10. Invalid States and Failure Modes

- Resource cache failures (texture/sampler creation) propagate through
  `ShadowResourceCache::GetOrCreateDirectionalShadowResources`.
- The first frame may produce empty cascades; downstream passes use the
  `HasDirectionalShadow()` early-out.

## 11. Threading and Synchronization Assumptions

- All public methods are called from the main thread.

## 12. Design Rationale

- The shadow subsystem is split into three small classes
  (Manager, Planner, Cache) so each can be reasoned about in
  isolation.
- `FreezeShadowMatrices` is a useful debug knob without touching the
  pipeline code.

## 13. Alternatives and Trade-offs

- Bundling planner + cache + manager into one file. Rejected for
  readability.
- Hardware PCF. Deferred to a future stage.

## 14. Extension Points

- Spot / point light shadows (placeholder; current renderer only
  honors `LIGHT_TYPE_DIRECTIONAL`).
- Async shadow map generation via compute.

## 15. Current Limitations

- One directional light is shadowed; only one is queried by the
  shader today.
- Per-cascade bias / texel size is computed but static.
- No `FreezeShadowMatrices` UI yet.

## 16. Source References

- `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.{h,cpp}`
- `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.{h,cpp}`
- `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.{h,cpp}`
- `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h`
- `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp:278`
