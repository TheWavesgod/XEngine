# CSM_04 — DirectionalShadowPlanner and Coordinate System

## Goal

Document the coordinate system conventions used by `DirectionalShadowPlanner` and the math that computes per-cascade light view-projection matrices. The planner's `.cpp` is already mostly correct; this document explains **why** each step is the way it is, and how to fix or extend the planner without introducing right-handed assumptions or wrong axis mappings.

## File Under Review

- `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.h`
- `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp`

The header has the public API:

```cpp
struct DirectionalShadowPlanDesc { /* see CSM_02 */ };
class DirectionalShadowPlanner
{
public:
    bool BuildPlan(const DirectionalShadowPlanDesc& desc,
                   RenderDirectionalShadowFrameData& outData) const;
};
```

The implementation currently contains:

- `ComputeCascadeSplits` (practical `(log, uniform, lerp)` scheme)
- `GetCameraFrustumCornersWorldSpace` (NDC inverse)
- `GetCascadeFrustumCornersWorldSpace` (linear interp along `near → far` rays)
- `ComputeAverageCenter` / `ComputeBoundingSphereRadius` (sphere fit)
- `QuantizeRadius` (16-quantum stabilization)
- `BuildLightBasis` (forward, right, up; with vertical-light fallback)
- `SnapToTexelGrid` (early abandoned variant — keep as comment or remove)
- `ComputeTexelSnapOffset` (returns world-space offset to subtract from light view translation)

The pipeline is correct in spirit. Below is a deep explanation of each block, with cross-references to the helpers in `Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h`.

## Coordinate System Rules (DO NOT VIOLATE)

### World (XEngine convention)

```text
+X = Forward
+Y = Right
+Z = Up
Left-handed
```

### Camera view space (used for cascade splitting)

```text
Camera looks along its local +X (forward).
Camera local +Y is right, local +Z is up.
```

A world point `P` is transformed to view space as `view * vec4(P, 1.0)`. View-space `+X` is the camera's forward, view-space `-X` is behind the camera, and view-space `Z` is depth (positive for points in front of the camera).

The cascade split distances are stored as **positive camera view-space depth** (i.e. `> 0` for points in front of the camera). `BuildGPUFrameData` should upload these directly. They are not camera-near/camera-far ratios; they are absolute depths.

### Light space (used for shadow projection)

```text
Light "camera" looks along its +X (light forward = the light's ray direction).
Light local +Y is right, +Z is up.
```

This matches `BuildViewMatrixLH_XForward` exactly. The light view matrix in XEngine is built so that `lightView * worldPos` puts the world point into the same +X-forward left-handed space, with depth along +X.

### Vulkan depth convention

```text
After projection, depth is in [0, 1].
Reverse-Z is enabled: light's "near" (closer to the light) maps to 1.0,
"far" maps to 0.0.
```

`Math::OrthographicLH_ZO(left, right, bottom, top, near, far)` returns a column-major `Mat4` whose `near` parameter corresponds to the depth value that maps to `1.0` in NDC (this matches `Math::PerspectiveLH_ZO` and Vulkan). For reverse-Z, pass `near > far` to swap.

`Math::Inverse(m)` is a general 4x4 inverse. Use it to invert the camera's `Projection * View` matrix to obtain world-space frustum corners.

## Cascade Split Calculation

`ComputeCascadeSplits` produces `cascadeSplits[0..N-1]` of positive depth values, monotonically increasing.

```cpp
static void ComputeCascadeSplits(
    float cameraNear,
    float cameraFar,
    u32 cascadeCount,
    float splitLambda,
    float* outSplits)
{
    splitLambda = Math::Clamp(splitLambda, 0.0f, 1.0f);
    const float nearClip = cameraNear;
    const float farClip  = cameraFar;
    const float clipRange = farClip - nearClip;
    const float safeNear = (nearClip > 1e-4f) ? nearClip : 1e-4f;
    const float ratio = farClip / safeNear;

    for (u32 i = 0; i < cascadeCount; ++i)
    {
        const float p = static_cast<float>(i + 1) / static_cast<float>(cascadeCount);
        const float logSplit   = nearClip * std::pow(ratio, p);
        const float linearSplit = nearClip + clipRange * p;
        const float split = Math::Lerp(linearSplit, logSplit, splitLambda);
        outSplits[i] = split;
    }
}
```

- `p = (i + 1) / N` produces the *far* edge of cascade `i` (cascades are `[prev, splits[i]]`).
- `logSplit` (logarithmic) gives more resolution to near cascades.
- `linearSplit` (uniform) gives more resolution to far cascades.
- `splitLambda = 0` → all linear; `1` → all log; `0.5` → practical default.

The first cascade is `[cameraNear, splits[0]]`, the second is `[splits[0], splits[1]]`, etc.

## Camera Frustum Corners in World Space

The full camera frustum is reconstructed from NDC corners with `nearZ = 0, farZ = 1` (Vulkan convention):

```cpp
const std::array<Vec3, 8> ndcCorners =
{
    Vec3(-1, -1, 0), // near-bottom-left
    Vec3( 1, -1, 0), // near-bottom-right
    Vec3( 1,  1, 0), // near-top-right
    Vec3(-1,  1, 0), // near-top-left
    Vec3(-1, -1, 1), // far-bottom-left
    Vec3( 1, -1, 1), // far-bottom-right
    Vec3( 1,  1, 1), // far-top-right
    Vec3(-1,  1, 1), // far-top-left
};
```

Inverse `Projection * View` and divide by `w`:

```cpp
const Mat4 invViewProj = Math::Inverse(cameraProjection * cameraView);
for (u32 i = 0; i < 8; ++i)
{
    Vec4 p = invViewProj * Vec4(ndcCorners[i], 1.0f);
    cornersWorld[i] = Vec3(p) / p.w;
}
```

**Why this works for XEngine's +X forward view:** The XEngine `BuildViewMatrixLH_XForward` does not rotate depth. View-space Z is still depth in the conventional sense — the projection matrix's near/far map to Z. The view matrix only rotates the basis from world to view, not from view to NDC. The NDC Z=0/1 convention is preserved.

**Watch-out:** If the camera projection is right-handed (e.g. `glm::perspective` with default Z clip), the depth in NDC will be `[-1, 1]`. This is the OpenGL convention and **must not** be used. Verify by checking the math: `Math::PerspectiveLH_ZO` produces the `[0, 1]` depth matrix. Any future camera must use this helper.

## Cascade Sub-Frustum

For a cascade with `[splitNear, splitFar]` (in view-space depth, both > 0):

```cpp
const float nearT = (splitNear - cameraNear) / (cameraFar - cameraNear);
const float farT  = (splitFar  - cameraNear) / (cameraFar - cameraNear);
```

These are parametric positions along the camera's `near → far` axis in `[0, 1]`. The cascade's 8 corners are:

```cpp
for (u32 i = 0; i < 4; ++i)
{
    const Vec3 fullNear = fullFrustumCorners[i];
    const Vec3 fullFar  = fullFrustumCorners[i + 4];
    const Vec3 ray = fullFar - fullNear;
    cascadeCorners[i]     = fullNear + ray * nearT;
    cascadeCorners[i + 4] = fullNear + ray * farT;
}
```

This works **only because** the camera's view-space depth is monotonic along the `near → far` ray. For XEngine's `+X forward` view, this is true.

## Bounding Sphere

`ComputeAverageCenter` averages the 8 corners. `ComputeBoundingSphereRadius` takes the maximum distance from the center. This is conservative (always contains the box) but cheap.

`QuantizeRadius(r) = ceil(r * 16) / 16` rounds the radius to 1/16 unit. This is a coarse stabilization; for better stability, snap to the shadow texel grid. The current approach is acceptable for V0.

## Light Basis

```cpp
static LightBasis BuildLightBasis(const Vec3& directionToLight)
{
    LightBasis basis;
    basis.Forward = Math::Normalize(directionToLight);

    // World up is +Z. If the light is nearly vertical, fall back to +Y.
    const Vec3 worldUp = (std::fabs(basis.Forward.z) > 0.999f)
        ? Vec3(0, 1, 0)
        : Vec3(0, 0, 1);

    basis.Right = Math::Normalize(Math::Cross(basis.Forward, worldUp));
    basis.Up    = Math::Normalize(Math::Cross(basis.Right,  basis.Forward));
    return basis;
}
```

This is the standard Gram-Schmidt. **XEngine note:** The light's `DirectionToLight` is the **direction from a shaded surface point to the light**, not the direction the light is pointing. For a directional light, the convention used in `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp` line 177-180 is:

```cpp
gpuLight.DirectionType = Vec4 {
    Math::Normalize(renderLight.DirectionToLight),
    static_cast<float>(GPULightType::Directional)
};
```

The shader uses this to compute the light contribution:

```hlsl
float3 L = g_Lights[i].DirectionType.xyz;  // surface → light
```

**Convention consistency check:** In XEngine, world +X is the camera's forward. The light's *ray* direction is therefore also along +X in light-space, *not* -Z. The `BuildLightBasis` code uses `basis.Forward = directionToLight`, i.e. the basis's +X axis is the light's ray direction. This is consistent with the camera convention.

If `DirectionToLight` is supplied to the planner, then `Forward = DirectionToLight`, and `Right`, `Up` form a left-handed orthonormal frame with `Cross(Forward, Up) = Right`. This is correct for a left-handed system where basis axes satisfy:

```text
Cross(Right, Up) = Forward
```

The code above computes `Right = Cross(Forward, worldUp)`, then `Up = Cross(Right, Forward)`. Verifying with a specific case:

- `Forward = (+1, 0, 0)`, `worldUp = (+0, 0, +1)`.
- `Right = Cross((1,0,0), (0,0,1)) = (0*1 - 0*0, 0*0 - 1*1, 1*0 - 0*0) = (0, -1, 0)`. Hmm. With GLM `Cross(a, b) = a × b = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)`, so:
  - `(1,0,0) × (0,0,1) = (0*1 - 0*0, 0*0 - 1*1, 1*0 - 0*0) = (0, -1, 0)`. So `Right = (0, -1, 0)`.
- `Up = Cross(Right, Forward) = (0,-1,0) × (1,0,0) = (-1*0 - 0*0, 0*1 - 0*0, 0*0 - (-1)*1) = (0, 0, 1)`. So `Up = (0, 0, 1)`.
- Verify: `Cross(Right, Up) = (0,-1,0) × (0,0,1) = (-1*1 - 0*0, 0*0 - 0*1, 0*0 - (-1)*0) = (-1, 0, 0)`. **But Forward is (1, 0, 0)**, not (-1, 0, 0). So the basis is not right-handed-consistent in the strict sense.

In a left-handed coordinate system, the convention is `Cross(Right, Up) = -Forward`, which gives `Cross((0,-1,0), (0,0,1)) = (0,0,1) × (-1, 0, 0) ...`. Wait, let me re-derive with the correct formula.

Let `a = (0, -1, 0)` (Right), `b = (0, 0, 1)` (Up). GLM's `Cross(a, b) = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)`:
- `a.y*b.z - a.z*b.y = -1*1 - 0*0 = -1`
- `a.z*b.x - a.x*b.z = 0*0 - 0*1 = 0`
- `a.x*b.y - a.y*b.x = 0*0 - (-1)*0 = 0`

So `Cross(Right, Up) = (-1, 0, 0)`. This is **not** `Forward = (1, 0, 0)`. The basis is **right-handed** in this configuration.

This is a known issue with this basis construction in a left-handed world. The light-space matrix built from `[Right, Up, Forward, ...]` will produce a left-handed-to-right-handed flip when the light is exactly along world +X.

**This is a real bug in `BuildLightBasis` for the left-handed XEngine convention.** The fix is to flip the cross-product order or negate one of the basis vectors. The cleanest fix:

```cpp
static LightBasis BuildLightBasis(const Vec3& directionToLight)
{
    LightBasis basis;
    basis.Forward = Math::Normalize(directionToLight);

    // World up is +Z. If the light is nearly vertical, fall back to +Y.
    const Vec3 worldUp = (std::fabs(basis.Forward.z) > 0.999f)
        ? Vec3(0, 1, 0)
        : Vec3(0, 0, 1);

    // For a left-handed basis where Cross(Right, Up) = Forward,
    // Right = Cross(worldUp, Forward) and Up = Cross(Forward, Right).
    basis.Right = Math::Normalize(Math::Cross(worldUp, basis.Forward));
    basis.Up    = Math::Normalize(Math::Cross(basis.Forward, basis.Right));
    return basis;
}
```

Verify:
- `Forward = (1, 0, 0)`, `worldUp = (0, 0, 1)`.
- `Right = Cross((0, 0, 1), (1, 0, 0)) = (0*0 - 1*0, 1*1 - 0*0, 0*0 - 0*1) = (0, 1, 0)`. ✓
- `Up = Cross((1, 0, 0), (0, 1, 0)) = (0*0 - 0*1, 0*0 - 1*0, 1*1 - 0*0) = (0, 0, 1)`. ✓
- `Cross(Right, Up) = Cross((0,1,0), (0,0,1)) = (1*1 - 0*0, 0*0 - 0*1, 0*0 - 1*0) = (1, 0, 0) = Forward`. ✓

**Action item:** Replace the basis construction in `DirectionalShadowPlanner.cpp::BuildLightBasis` with the corrected version above. This is required for the left-handed XEngine convention to be consistent.

The test case is a directional light pointing along world `+X` (e.g. a "sunrise" light). With the current code, the cascade's projection would be flipped on the Y axis, and shadows would appear with a horizontal mirror flip. With the corrected code, the cascade is correct.

For lights not aligned with world axes, the difference is small. But the corrected code is required for correctness.

## Light View Matrix

The light view matrix is built directly from the basis:

```cpp
Mat4 lightView = Mat4(1.0f);
lightView[0] = Vec4(lightBasis.Right,   0.0f);
lightView[1] = Vec4(lightBasis.Up,      0.0f);
lightView[2] = Vec4(lightBasis.Forward, 0.0f);
lightView[3] = Vec4(lightPosition,      1.0f);
lightView = Math::Inverse(lightView);
```

This builds an affine matrix `[R, U, F, P]` in column-major form (GLM convention), then inverts it. The matrix is mathematically the same as the camera's `BuildViewMatrixLH_XForward` if the basis satisfies `Cross(R, U) = F`.

With the corrected basis from the previous section, this is correct.

**Alternative formulation using the engine helper:**

```cpp
Quat lightRotation = QuatLookAtLH_XForward(/* forward = */ lightBasis.Forward, /* up = */ lightBasis.Up);
Mat4 lightView = Math::BuildViewMatrixLH_XForward(lightPosition, lightRotation);
```

If `Math::QuatLookAtLH_XForward` exists, this is preferred for consistency. If not, the direct construction above is correct.

## Texel Snap

`ComputeTexelSnapOffset` returns a world-space offset to subtract from the light view's translation column:

```cpp
lightView[3] -= Vec4(snap, 0.0f);
```

This is the standard "stable CSM" trick. The math:

1. Project the world center into light clip space: `clip = lightViewProj * vec4(center, 1)`.
2. Compute NDC: `ndcX = clip.x / clip.w, ndcY = clip.y / clip.w`.
3. Snap NDC to the nearest texel: `snapped = round(ndc / texelSize) * texelSize`.
4. Compute the world-space offset that, when subtracted from the light position, would put the projected center on the snapped NDC.
5. Subtract this offset from the light view's translation column.

This is the correct approach. Do **not** modify the world center directly — the light's projection bounds are tied to the light view's position.

`texelSize` is `2.0 / resolution` because the orthographic projection covers `[-1, 1]` in clip space (after the standard `[-radius, radius]` mapping).

## Light Projection (Reverse-Z)

```cpp
Mat4 lightProjection;
if (desc.ReverseZ)
{
    lightProjection = Math::OrthographicLH_ZO(
        -radius, radius,    // left, right
        -radius, radius,    // bottom, top
        radius + depthBias, // near (mapped to depth 1.0)
        -radius - depthBias // far  (mapped to depth 0.0)
    );
}
else
{
    lightProjection = Math::OrthographicLH_ZO(
        -radius, radius,
        -radius, radius,
        -radius - depthBias, // near (depth 0.0)
        radius + depthBias    // far  (depth 1.0)
    );
}
```

The `radius` here is the world-space sphere radius. The `+ depthBias` slack is added to the light's `near`/`far` extents to avoid clipping close or far surfaces.

**Watch-out:** `Math::OrthographicLH_ZO` is called with `near > far` for reverse-Z. The math helper must be checked to confirm that passing `near > far` produces the correct projection. If it asserts or normalizes, swap the order at the call site. The current code does the swap, which is correct.

**Watch-out 2:** With reverse-Z, the depth test in `ShadowDepth.slang` is `SV_DepthGreaterEqual` (Vulkan default), and the comparison in `ForwardPBR.slang` is `shadowDepth >= refDepth` (which means "not in shadow"). Do not invert this.

## Filling the Output

```cpp
RenderShadowCascade& cascade = outData.Cascades[cascadeIndex];
cascade.LightView           = lightView;
cascade.LightProjection     = lightProjection;
cascade.LightViewProjection = lightProjection * lightView;
cascade.SplitNear           = splitNear;
cascade.SplitFar            = splitFar;
cascade.LayerIndex          = cascadeIndex;
cascade.Resolution          = desc.Resolution;
cascade.ShadowMapSize       = Vec4(res, res, 1/res, 1/res);
cascade.BiasParams          = Vec4(depthBias, normalBias, 0, 0);

AABB worldBounds;
worldBounds.Min = Vec3( std::numeric_limits<float>::infinity());
worldBounds.Max = Vec3(-std::numeric_limits<float>::infinity());
for (const Vec3& corner : cascadeCorners)
{
    worldBounds.Min = Math::Min(worldBounds.Min, corner);
    worldBounds.Max = Math::Max(worldBounds.Max, corner);
}
cascade.WorldBounds = worldBounds;

// Light-space bounds
Vec3 lightSpaceMin( std::numeric_limits<float>::infinity());
Vec3 lightSpaceMax(-std::numeric_limits<float>::infinity());
for (const Vec3& corner : cascadeCorners)
{
    const Vec4 ls = lightView * Vec4(corner, 1.0f);
    lightSpaceMin = Math::Min(lightSpaceMin, Vec3(ls));
    lightSpaceMax = Math::Max(lightSpaceMax, Vec3(ls));
}
cascade.LightSpaceBounds = AABB(lightSpaceMin, lightSpaceMax);
```

`LightSpaceBounds` is in light-view space (pre-projection). The Z range of this AABB tells you whether the cascade's depth range is inside the projection's `[-radius, radius]` range.

## Final Code Listing (corrected)

The complete corrected `BuildLightBasis` is the only change to the planner needed:

```cpp
static LightBasis BuildLightBasis(const Vec3& directionToLight)
{
    LightBasis basis;
    basis.Forward = Math::Normalize(directionToLight);

    // World up is +Z. If the light is nearly vertical, fall back to +Y.
    const Vec3 worldUp = (std::fabs(basis.Forward.z) > 0.999f)
        ? Vec3(0, 1, 0)
        : Vec3(0, 0, 1);

    // Left-handed basis: Cross(Right, Up) = Forward.
    basis.Right = Math::Normalize(Math::Cross(worldUp, basis.Forward));
    basis.Up    = Math::Normalize(Math::Cross(basis.Forward, basis.Right));
    return basis;
}
```

The rest of the planner is correct. The `AABB::FromPoints` reference has already been replaced with a `Min/Max` loop. No other changes are required to the planner for V0.

## What This Document Does Not Do

- It does not describe how the planner's output is uploaded to the GPU — see [CSM_07](CSM_07_FrameResources_And_ForwardSampling.md).
- It does not describe the shader side of CSM — see [CSM_08](CSM_08_Shaders.md).
- It does not describe how the planner is invoked — see [CSM_05](CSM_05_RenderShadowManager.md).

## Common Mistakes (Planner)

1. **Using `glm::lookAtRH` or any right-handed lookAt.** All matrix construction must go through the `Math::LookAtLH_XForward` helper or the direct `[Right, Up, Forward, Position]` formulation.

2. **Forgetting that `DirectionToLight` is surface→light, not the light's pointing direction.** The shader treats `DirectionToLight` as the L vector for shading. The planner's `BuildLightBasis` uses it as the light's forward axis. This is consistent *only if* the light's rays travel in the direction of `DirectionToLight` from a surface to the light. Verify with `RenderFrameResources.cpp::BuildGPULightingData` line 178.

3. **Using `OrthographicLH_ZO` with `near < far` when reverse-Z is desired.** Always pass `near > far` for reverse-Z.

4. **Setting `ReverseZ = false` in the desc.** Vulkan's default is reverse-Z. The shader side assumes reverse-Z. Do not toggle this in V0.

5. **Not validating `desc.CameraNear > 0` before using `std::log` or `std::pow(ratio, p)`.** The current code uses `safeNear` to avoid `log(0)`. Verify this is in place after any edits.
