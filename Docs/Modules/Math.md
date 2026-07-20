# Math

## 1. Module Purpose

Math provides GLM-backed vector, matrix, and quaternion aliases plus the
camera/projection helpers and glTF conversion routines used across the engine.
The XEngine convention is enforced at this layer so higher modules never deal
with raw `glm::*` types.

## 2. Responsibilities

- Provide the `Vec2/Vec3/Vec4`, `Mat3/Mat4`, `Quat`, `IVec*`, `UVec*` aliases.
- Implement the named camera-matrix helpers:
  `ComposeTRS`, `TransformAABB`, `CombineAABB`, `PerspectiveLH_ZO`,
  `OrthographicLH_ZO`, `BuildViewMatrixLH_XForward`, `LookAtLH_XForward`,
  `GetForwardVector`, `GetRightVector`, `GetUpVector`.
- Implement glTF coordinate bridging (`GltfPositionToXEngine`,
  `GltfDirectionToXEngine`, `GltfTangentToXEngine`).
- Provide bounding volume primitives (`AABB`, `Frustum` placeholder,
  `Rotator`).

## 3. Non-Responsibilities

- Does not own any GPU resources.
- Does not implement the camera class itself; that lives in
  `Renderer/Private/Pipeline/` once the helper extraction lands.
- Does not implement world/level persistence or serialization.

## 4. Public API Surface

`Engine/Source/Foundation/Math/Public/XEngine/Math/` exposes:

- `MathTypes.h` - vector/matrix/quaternion aliases.
- `Math.h`, `MathFunctions.h` - generic helpers (Dot, Cross, Normalize,
  LookAt, Translate, Rotate, Scale, TransformPoint, TransformVector,
  AngleAxis, ExtractTranslation, ComposeTRS).
- `CameraMatrices.h` - `Math::PerspectiveLH_ZO`, `Math::OrthographicLH_ZO`,
  `Math::BuildViewMatrixLH_XForward`, `Math::LookAtLH_XForward`.
- `CoordinateSystem.h` - basis constants `Math::Forward/Right/Up` plus
  `GetForwardVector / GetRightVector / GetUpVector(Quat)` and unit helpers.
- `CoordinateConversion.h` - glTF -> XEngine bridge.
- `AABB.h` - `struct AABB { Vec3 Min, Max; }` plus
  `Math::TransformAABB`, `Math::CombineAABB`.
- `Frustum.h` - placeholder `struct Frustum`.
- `Rotator.h` - `struct Rotator` and Radian<->Degree conversion.
- `Transform.h` - convenience wrapper re-exporting the most-used helpers.

## 5. Dependencies

### Depends on

- `Core` (`XEngineFoundation`) for assert + log.
- `glm` (PUBLIC).

### Used by

- `Runtime/Scene` (`TransformComponent`, `TransformSystem`).
- `Runtime/Renderer` (`RenderExtraction`, `RenderFrameContext`,
  `ForwardRenderPipeline.cpp`, `RenderProjection.h`,
  `DirectionalShadowPlanner`).
- `Runtime/Asset` (`GltfImporter`).

## 6. Ownership and Lifetime

- All math types are value types or stack-only POD structs; no ownership.
- The named helpers are stateless and inline.
- `AABB` and `Frustum` are POD structs.

## 7. Runtime Flow

- Every subsystem that computes or transforms geometry calls into Math.
- There is no initialization or shutdown phase for the module.

## 8. Important Invariants

- **Convention**: +X Forward, +Y Right, +Z Up, left-handed. Any new helper
  must respect this; verification is by review since no static analysis
  enforces it.
- **Column-major matrices** match GLM and the GPU layout; `static_assert`s
  in `Renderer/Private/Resources/RenderShaderTypes.h:10` lock the size.
- **LH + ZO projections**: zero-to-one NDC depth range; Y flip is applied
  by `Math/Pipeline/RenderProjection.h::ApplyRHIClipSpaceConvention` only
  once per frame from `RenderSystem`.
- **glTF conversion is mandatory at import time**; downstream code never
  sees glTF coordinates.

## 9. Main Classes and Collaborators

- `Math` namespace (free function helpers).
- `Vec*`, `Mat*`, `Quat` (aliases).
- `AABB`, `Frustum`, `Rotator`.

## 10. Design Rationale

- A named, helper-rich Math layer documents the conventions, which matters
  most at the time a new coordinate-sensitive renderer feature is added.
- The `XForward` suffix on view helpers and the `LH_ZO` suffix on projection
  helpers embed the convention in the function name rather than in a comment.

### Alternatives considered

- Custom structs per data type. Rejected: no current need for additional
  invariants beyond GLM's column-major.
- Column-major matrix storage. Rejected: GLM matches Vulkan / shader
  layout natively.

### Trade-offs

- Math functions are not SIMD-accelerated; performance matters in
  `DirectionalShadowPlanner.BuildPlan` but is acceptable for V0.

## 11. Failure Modes and Debugging

- A mis-applied coordinate conversion shows up as visible scene objects in
  the wrong orientation. Symptoms: upside-down models, side-mirrored
  objects. Trace through `GltfImporter` and confirm all `Position`,
  `Direction`, and `Tangent` channels are mapped.

## 12. Current Limitations

- `Frustum::Cull` is not implemented.
- No SIMD math.

## 13. Source References

- `Engine/Source/Foundation/Math/Public/XEngine/Math/*.h`
- `Engine/Source/Foundation/Math/Private/Frustum.cpp`
- `Engine/Source/Foundation/CMakeLists.txt`

## 14. Future Work

- Implement `Frustum::Cull` for the future RenderFeature work.
- Add SIMD-friendly math variants for inner loops in shadow math.
