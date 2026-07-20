# 04 Coordinate and Matrix Conventions

## 1. World Convention

```text
+X = Forward
+Y = Right
+Z = Up
Left-handed world
```

This is the same convention documented in `Docs/AI helper/Project_Cache.md` and
is enforced by the math helpers in `Engine/Source/Foundation/Math/Public/XEngine/Math/`.

## 2. Camera Local Axes

`Math::GetForwardVector / GetRightVector / GetUpVector(Quat)` return the world-space
camera basis. From the source, the constants are:

```text
Forward = +X
Right   = +Y
Up      = +Z
```

This is verified at `Engine/Shaders/Passes/ForwardPBR.slang:107` where the
shader comments "XEngine: camera forward is +X in view space" and uses
`viewPos4.x` as the view-space depth (the X component). The view matrix
transformation therefore places the camera-forward vector along +X after the
view is applied.

## 3. View Matrix Convention

`Math::BuildViewMatrixLH_XForward(WorldPosition, WorldRotation)` - the LH
suffix indicates a left-handed basis. `_XForward` indicates the camera looks
along the world +X axis.

The implementation lives at `Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h`.

The view matrix is built so that:
- View space `+X` = world forward in camera-local terms.
- View space `+Y` = world right.
- View space `+Z` = world up.

## 4. Projection Matrix Convention

`Math::PerspectiveLH_ZO` and `Math::OrthographicLH_ZO` produce left-handed
projections with the ZO suffix implying **zero-to-one depth range** (matching
Vulkan's NDC range `[0, 1]`).

The Y-flip for the active backend is **applied at the RHI boundary** in
`Math/Pipeline/RenderProjection.h`:

```cpp
inline Mat4 ApplyRHIClipSpaceConvention(const Mat4& projection,
                                         const RHIClipSpaceConvention& convention);
```

`VulkanDevice::GetClipSpaceConvention` reports
`{DepthZeroToOne = true, FlipProjectionY = true, UseInvertedViewportY = false,
DefaultFrontFace = RHIFrontFace::CounterClockwise}`.

`RenderSystem::Render` applies `ApplyRHIClipSpaceConvention` to both the
ViewProvider and the SceneSystem-derived projection (lines 210-212 and
245-247). This is the only place where the Y-flip is applied.

## 5. Directional Light Convention

`DirectionalLightComponent` defines its forward rotation; the convention in
`RenderExtraction` is:

- Light rays travel along the world forward axis rotated by the entity's
  world rotation.
- `DirectionToLight` is the direction from the surface point back to the light
  source, i.e. `-forward`.
- The shader uses `light.DirectionType.xyz` as the `L` vector in
  `BRDF.slang`, consistent with `NdotL = saturate(dot(N, L))`.

Shadow sampling does **not** pre-rotate this convention; the data path is
"entity world rotation -> -forward -> CPU shadow planner uses this vector ->
LVP -> ShadowDepth pass uses `LightViewProjection` directly -> ForwardPBR
shader samples the cascade texture using `worldPos -> LVP`."

## 6. Shadow Light View Convention

`RenderShadowType.h` defines a `RenderShadowCascade` with:

- `LightView` and `LightProjection` matrices.
- `LightViewProjection = LightProjection * LightView`.

The shadow planner is in `Renderer/Private/Shadows/DirectionalShadowPlanner.cpp`.
There is currently no shader-visible `LightView` matrix - only
`LightViewProjection` ends up in `GPUShadowData.Cascades[i].LightViewProjection`
(`Engine/Shaders/Lighting/ShadowTypes.slang`). `LightView` is kept CPU-side for
potential future use (e.g. stabilization math).

## 7. Cascade Frustum Convention

Cascade splits are computed CPU-side in `DirectionalShadowPlanner`. Each
cascade's `LightViewProjection` is built such that the cascade's near/far in
view space is `cascade.SplitNear/cascade.SplitFar`. From
`Engine/Shaders/Lighting/ShadowTypes.slang`:

```cpp
struct GPUCascadeShadowData {
    float4x4 LightViewProjection;

    // x = split far in view-space depth
    // y = depth bias
    // z = normal bias
    // w = texel size
    float4 Params;
};
```

The shadow shader therefore uses `worldPos -> LightViewProjection`, with the
view-space depth read from the X coordinate of `LightViewProjection * worldPos`
(this matches the camera convention).

## 8. Vulkan Clip-Space Adaptation

Vulkan + zero-to-one depth conventions:

```text
OpenGL         : [-1, 1] depth, [-1, 1] clip volume, Y up.
Vulkan         : [ 0, 1] depth, [-1, 1] clip volume on X/Y, framebuffer
                                  origin top-left.
XEngine target : Vulkan NDC, so Y is flipped on the CPU (projection matrix)
                 and depth is [0, 1].
```

Y-flip implementation: `Math/Pipeline/RenderProjection.h::ApplyRHIClipSpaceConvention`
multiplies `projection[1][1] *= -1` when `convention.FlipProjectionY == true`.

Inverted viewport Y is reported `false` by the Vulkan backend
(`VulkanDevice::GetClipSpaceConvention`), so no extra flip happens at the
command-list layer. The Vulkan command list sets viewport Y=0 with positive
height in `VulkanCommandList::SetViewport`.

## 9. Matrix Memory Layout

- All matrices are GLM column-major (`Mat4 = glm::mat4` defined in
  `Foundation/Math/Public/XEngine/Math/MathTypes.h`).
- CPU-side `static_assert(sizeof(Mat4) == sizeof(float) * 16)` is enforced at
  `Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderTypes.h:10`.
- GPU-side structs (`GPUCameraData`, `GPULightingData`, `GPUShadowData`,
  `GPUFrameData`) use `alignas(16)` and explicit `float4x4`/`float4` fields so
  they match Slang's std140-friendly layout.

The shader-side comment at `Engine/Shaders/Common/Types.slang` says:

```text
// Must match the C++ GPUFrameData layout.
// Keep fields 16-byte aligned and update both sides together.
```

This is the contract enforced by Visual Studio debug layout checks
(`Renderer/Private/ShaderInterop/GPUFrameTypes.h:33-38`) and Slang layout.

## 10. CPU / Shader Multiplication Order

- Vertex transform (object -> world): `worldPos = Model * float4(position, 1.0)`
- Vertex transform (world -> clip): `clipPos = ViewProjection *
  float4(worldPos, 1.0)` in the live shaders (`ForwardPBR.slang`,
  `DepthOnly.slang`).
- Vertex transform (object -> clip, no world step): the depth-only shader
  goes `worldPos = Model * float4(position, 1.0); clipPos = LightViewProjection *
  float4(worldPos, 1.0)`.

There is no row-major translation step; both sides assume GLSL/Slang's
column-major multiplication convention (mat4 * vec4).

## 11. Asset Import Conversion Boundary

glTF assets come in with a right-handed Z-up convention. The conversion
helpers are declared in `Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateConversion.h`:

- `Math::GltfPositionToXEngine(Vec3) -> Vec3`
- `Math::GltfDirectionToXEngine(Vec3) -> Vec3`
- `Math::GltfTangentToXEngine(Vec3) -> Vec3`

These must be called at the asset-import boundary in
`Engine/Source/Runtime/Asset/Private/Importers/GltfImporter.cpp` and the
converted coordinates propagate through Scene + Renderer without further
transformation. Currently:

- `GltfImporter.cpp` is implemented; the conversion boundary is at the GPU
  upload step.
- Subsequent code (Scene, Renderer, shaders) operates exclusively in XEngine
  coordinates.

## 12. Inconsistencies or Open Items

- `RenderSystem::Render` builds the ViewProjection locally inside the
  per-frame function; the Scene's primary camera and the application-level
  ViewProvider path duplicate this math. See class docs for RenderSystem and
  CameraProjection (currently absent).
- `LightView` is computed CPU-side and exposed in
  `RenderDirectionalShadowFrameData` but never reaches the shader; this is
  kept for future use rather than removed.

## 13. Source References

- `Engine/Source/Foundation/Math/Public/XEngine/Math/MathTypes.h`
- `Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateSystem.h`
- `Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h`
- `Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateConversion.h`
- `Engine/Source/Runtime/Renderer/Private/Scene/RenderExtraction.cpp:30-116`
- `Engine/Source/Runtime/Renderer/Private/Pipeline/RenderProjection.h`
- `Engine/Source/Renderer/Private/RenderSystem.cpp:197-254`
- `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp:271-279`
- `Engine/Shaders/Common/Types.slang`
- `Engine/Shaders/Lighting/ShadowTypes.slang`
- `Engine/Shaders/Passes/ForwardPBR.slang`
- `Engine/Shaders/Passes/DepthOnly.slang`
- `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp`
