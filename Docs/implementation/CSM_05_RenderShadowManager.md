# CSM_05 — RenderShadowManager

## Goal

`RenderShadowManager` is the **CPU-side coordinator** for shadow rendering in Stage 9 V0. It connects the scene extraction (which already produces a `RenderScene.Lights` array with `CastShadow` flags) to the `ShadowResourceCache` (which owns the GPU shadow resources) and to the `DirectionalShadowPlanner` (which computes matrices).

`RenderShadowManager` does not create or own `RHIDevice` resources directly. It delegates to `ShadowResourceCache`. It does not compute matrices itself. It delegates to `DirectionalShadowPlanner`. It does not draw. The `ShadowDepthPass` consumes the output.

## File to Modify

- `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.h` (header is correct; `.cpp` is empty)
- `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp` (needs full implementation)

## Public API (header, already correct)

```cpp
class RenderShadowManager
{
public:
    void Initialize(RHIDevice& device);
    void Shutdown(RHIDevice& device);

    void PrepareFrame(RHIDevice& device,
                      const RenderScene& scene,
                      const RenderFrameContext& frame,
                      const ShadowSettings& settings,
                      const ShadowDebugSettings& debugSettings);

    void FillGPUShadowData(GPUShadowData& outData) const;
    const RenderShadowFrameData& GetFrameData() const;
    bool HasDirectionalShadow() const;

private:
    void PrepareDirectionalShadow(RHIDevice& device,
                                  const RenderScene& scene,
                                  const RenderFrameContext& frame,
                                  const DirectionalShadowSettings& settings,
                                  const ShadowDebugSettings& debugSettings);

    RenderShadowFrameData m_FrameData;
    DirectionalShadowPlanner m_DirectionalPlanner;
    ShadowResourceCache m_ResourceCache;

    bool m_HasFrozenData = false;
    RenderShadowFrameData m_FrozenFrameData;
};
```

## Member Variables

| Member | Purpose |
|--------|---------|
| `m_FrameData` | Per-frame shadow state. Recomputed every frame unless `FreezeShadowMatrices` is on. |
| `m_DirectionalPlanner` | Pure math; stateless. Owned by value. |
| `m_ResourceCache` | Owns shadow GPU resources (texture array, sampled view, per-layer depth views, sampler). |
| `m_HasFrozenData` | True after the user enables `FreezeShadowMatrices` and we have captured one frame of data. |
| `m_FrozenFrameData` | Snapshot of `m_FrameData` taken when `FreezeShadowMatrices` was enabled. Used to keep matrices stable across frames while the toggle is on. |

## Detailed Implementation

### `Initialize`

```cpp
void RenderShadowManager::Initialize(RHIDevice& device)
{
    m_ResourceCache.Initialize(device);
    m_FrameData = {};
    m_FrozenFrameData = {};
    m_HasFrozenData = false;
    XENGINE_LOG_INFO("RenderShadowManager initialized");
}
```

`DirectionalShadowPlanner` requires no initialization — it has no members.

### `Shutdown`

```cpp
void RenderShadowManager::Shutdown(RHIDevice& device)
{
    m_ResourceCache.Shutdown(device);
    m_FrameData = {};
    m_FrozenFrameData = {};
    m_HasFrozenData = false;
}
```

### `HasDirectionalShadow`

```cpp
bool RenderShadowManager::HasDirectionalShadow() const
{
    return m_HasFrozenData
        ? m_FrozenFrameData.Directional.Enabled
        : m_FrameData.Directional.Enabled;
}
```

### `GetFrameData`

```cpp
const RenderShadowFrameData& RenderShadowManager::GetFrameData() const
{
    return m_HasFrozenData ? m_FrozenFrameData : m_FrameData;
}
```

The `ShadowDepthPass` and `ForwardOpaquePass` (via `FillGPUShadowData`) use this.

### `PrepareFrame`

```cpp
void RenderShadowManager::PrepareFrame(RHIDevice& device,
                                       const RenderScene& scene,
                                       const RenderFrameContext& frame,
                                       const ShadowSettings& settings,
                                       const ShadowDebugSettings& debugSettings)
{
    if (debugSettings.FreezeShadowMatrices)
    {
        if (!m_HasFrozenData)
        {
            PrepareDirectionalShadow(device, scene, frame, settings.Directional, debugSettings);
            m_FrozenFrameData = m_FrameData;
            m_HasFrozenData = true;
        }
        // else: keep using m_FrozenFrameData
        return;
    }
    else
    {
        m_HasFrozenData = false;
        m_FrozenFrameData = {};
    }

    PrepareDirectionalShadow(device, scene, frame, settings.Directional, debugSettings);
}
```

The first frame after `FreezeShadowMatrices` is enabled captures the data; subsequent frames return immediately. When the toggle is disabled, the snapshot is cleared.

### `PrepareDirectionalShadow`

This is the main per-frame routine. Pseudocode:

```cpp
void RenderShadowManager::PrepareDirectionalShadow(
    RHIDevice& device,
    const RenderScene& scene,
    const RenderFrameContext& frame,
    const DirectionalShadowSettings& settings,
    const ShadowDebugSettings& debugSettings)
{
    // 1. Reset the per-frame data.
    m_FrameData = {};
    m_FrameData.Directional.Enabled = false;
    m_FrameData.Directional.CascadeCount = 0;

    // 2. Master switch.
    if (!settings.Enabled || settings.Technique != DirectionalShadowTechnique::CascadedShadowMaps)
    {
        return;
    }

    // 3. Find the first enabled directional light with CastShadow.
    const RenderLight* shadowLight = nullptr;
    for (const RenderLight& light : scene.Lights)
    {
        if (!light.Enabled) continue;
        if (light.Type != RenderLightType::Directional) continue;
        if (!light.CastShadow) continue;
        shadowLight = &light;
        break;
    }
    if (shadowLight == nullptr)
    {
        return;
    }

    // 4. Acquire or recreate shadow resources.
    DirectionalShadowResourceDesc resDesc;
    resDesc.Resolution   = settings.Resolution;
    resDesc.CascadeCount = settings.CascadeCount;
    resDesc.DepthFormat  = RHIFormat::D32Float;
    resDesc.StorageMode  = settings.StorageMode;

    DirectionalShadowResources& res =
        m_ResourceCache.GetOrCreateDirectionalShadowResources(device, resDesc);

    if (!res.Texture || !res.SampledView || !res.Sampler)
    {
        XENGINE_LOG_WARN("Shadow resources unavailable; disabling shadows this frame");
        return;
    }
    for (u32 i = 0; i < res.CascadeCount; ++i)
    {
        if (!res.LayerDepthViews[i])
        {
            XENGINE_LOG_WARN("Shadow per-layer view missing; disabling shadows this frame");
            return;
        }
    }

    // 5. Build the per-cascade matrices via the planner.
    DirectionalShadowPlanDesc planDesc;
    planDesc.Light             = shadowLight;
    planDesc.CameraView        = frame.ViewMatrix;
    planDesc.CameraProjection  = frame.ProjectionMatrix;
    planDesc.CameraNear        = /* per-camera near plane; */ 0.1f;
    planDesc.CameraFar         = /* per-camera far plane; */ 1000.0f;
    planDesc.CameraPosition    = frame.CameraWorldPosition;
    planDesc.SceneBounds       = ComputeShadowCasterBounds(scene);
    planDesc.CascadeCount      = settings.CascadeCount;
    planDesc.Resolution        = settings.Resolution;
    planDesc.SplitLambda       = settings.SplitLambda;
    planDesc.DepthBias         = settings.DepthBias;
    planDesc.NormalBias        = settings.NormalBias;
    planDesc.StabilizeCascades = settings.StabilizeCascades;
    planDesc.ReverseZ          = true; // Vulkan default

    if (!m_DirectionalPlanner.BuildPlan(planDesc, m_FrameData.Directional))
    {
        XENGINE_LOG_WARN("DirectionalShadowPlanner failed; disabling shadows this frame");
        m_FrameData = {};
        return;
    }

    // 6. Wire GPU resources into the frame data.
    m_FrameData.Directional.Enabled      = true;
    m_FrameData.Directional.CascadeCount = settings.CascadeCount;
    m_FrameData.Directional.ShadowTexture  = res.Texture.get();
    m_FrameData.Directional.SampledView    = res.SampledView.get();
    m_FrameData.Directional.Sampler        = res.Sampler.get();
    for (u32 i = 0; i < settings.CascadeCount; ++i)
    {
        m_FrameData.Directional.CascadeDepthViews[i] = res.LayerDepthViews[i].get();
    }
}
```

### `ComputeShadowCasterBounds` helper

This is a static method or a free function. Pseudocode:

```cpp
static AABB ComputeShadowCasterBounds(const RenderScene& scene)
{
    AABB bounds;
    bounds.Min = Vec3( std::numeric_limits<float>::infinity());
    bounds.Max = Vec3(-std::numeric_limits<float>::infinity());
    bool any = false;
    for (const RenderObject& obj : scene.OpaqueObjects)
    {
        if (!obj.CastShadow) continue;
        if (obj.Mesh.IsValid() == false) continue;
        // RenderObject.WorldBounds is in world space; expand the AABB by
        // the object's local bounds transformed to world.
        bounds.Min = Math::Min(bounds.Min, obj.WorldBounds.Min);
        bounds.Max = Math::Max(bounds.Max, obj.WorldBounds.Max);
        any = true;
    }
    if (!any)
    {
        // Fall back to a 100m cube around the origin.
        bounds.Min = Vec3(-50, -50, -50);
        bounds.Max = Vec3( 50,  50,  50);
    }
    return bounds;
}
```

The fallback guarantees that the planner has a non-empty AABB even when no shadow-casting objects exist (e.g. an empty editor scene). The planner currently uses the per-cascade frustum corners to compute the sphere, so the `SceneBounds` parameter is informational in V0. The fallback still avoids a degenerate AABB that would be passed to the planner.

### `FillGPUShadowData`

```cpp
void RenderShadowManager::FillGPUShadowData(GPUShadowData& outData) const
{
    const RenderDirectionalShadowFrameData& dir = GetFrameData().Directional;

    outData.ShadowParams = Vec4(
        dir.Enabled ? 1.0f : 0.0f,
        static_cast<float>(dir.CascadeCount),
        static_cast<float>(dir.CascadeCount > 0 ? dir.Cascades[0].Resolution : 0),
        0.0f); // visualize cascades is set by the caller (RenderFrameResources) from debug settings.

    for (u32 i = 0; i < MaxShadowCascades; ++i)
    {
        GPUCascadeShadowData& gpu = outData.Cascades[i];
        if (i < dir.CascadeCount)
        {
            const RenderShadowCascade& c = dir.Cascades[i];
            gpu.LightViewProjection = c.LightViewProjection;
            const float texelSize = (c.Resolution > 0)
                ? 1.0f / static_cast<float>(c.Resolution)
                : 0.0f;
            gpu.Params = Vec4(
                c.SplitFar,
                c.BiasParams.x, // depth bias
                c.BiasParams.y, // normal bias
                texelSize);
        }
        else
        {
            gpu.LightViewProjection = Mat4(1.0f);
            gpu.Params = Vec4(0.0f);
        }
    }
}
```

`ShadowParams.w` (visualize cascades) is left at zero here and is set by the caller (see `CSM_07`). This keeps the manager focused on per-light data.

## Camera Near/Far

The current code has a `TODO` comment: `RenderFrameContext` does not yet expose the active camera's near/far. For V0, use the camera's near/far from the active camera, or accept reasonable defaults.

Two options:

1. **Recommended — pass via `RenderFrameContext`.** Extend `RenderFrameContext` with `float CameraNear` and `float CameraFar`. `RenderSystem::Render` sets them from the active camera's `CameraComponent`. The manager reads them via `frame.CameraNear` / `frame.CameraFar`.

2. **Fallback — derive from projection matrix.** The Z-clip range can be read off the projection matrix's `(2, 2)` and `(2, 3)` entries for a perspective matrix. This is fragile and not recommended.

Use option 1. The change to `RenderFrameContext` is a single line addition per field.

## Behavior with `CastShadow = false`

When the first directional light has `CastShadow = false` (or no directional light exists), `m_FrameData.Directional.Enabled = false` and the GPU side has `ShadowParams.x = 0`. The shader must early-out: `if (g_FrameData.Shadow.ShadowParams.x < 0.5) { skip shadow lookup; treat surfaces as fully lit; }`. See [CSM_08](CSM_08_Shaders.md) for the shader integration.

## Behavior with `DirectionalShadowTechnique::None`

`settings.Technique == DirectionalShadowTechnique::None` means "no shadow work this frame". The manager should treat this identically to "no shadow-casting light" — return with `m_FrameData.Directional.Enabled = false`. The `PrepareFrame` code above already handles this.

## What This Document Does Not Do

- It does not describe the resource cache implementation — see [CSM_03](CSM_03_ShadowResources.md).
- It does not describe the planner's math — see [CSM_04](CSM_04_DirectionalShadowPlanner_Coordinates.md).
- It does not describe the depth pass — see [CSM_06](CSM_06_ShadowDepthPass_And_Pipeline.md).
- It does not describe the GPU data upload — see [CSM_07](CSM_07_FrameResources_And_ForwardSampling.md).

## Common Mistakes

1. **Forgetting to clear `m_FrameData` at the start of `PrepareDirectionalShadow`.** Otherwise stale data from a previous frame (e.g. a different cascade count) can leak through.

2. **Not validating the per-layer depth view pointers before dereferencing.** `GetOrCreateDirectionalShadowResources` may return a half-built cache if creation failed mid-way. Always check every pointer.

3. **Recomputing matrices when `FreezeShadowMatrices` is on.** The whole point of the freeze is to keep matrices stable. Capture once and reuse.

4. **Setting `ShadowParams.w` from the manager.** The manager does not own the debug flag. The caller (e.g. `RenderFrameResources` or a debug overlay) sets it.

5. **Forgetting to set `m_FrameData.Directional.CascadeCount`.** The shader uses `g_FrameData.Shadow.ShadowParams.y` to clamp cascade lookups. If the count is zero, the shader should treat shadows as disabled.

6. **Setting `outData.ShadowParams.z` to the wrong value.** The third component is shadow resolution, not cascade count. Both are useful; the planner-side code uses resolution for texel-size math.
