# Core (Foundation)

## 1. Module Purpose

Core is the engine-agnostic foundation: types, asserts, logging, light
diagnostics, and the project-path helper. It is the leaf of every module
graph and contains nothing gameplay- or rendering-specific.

The Foundation module bundles four sub-modules into one library
`XEngineFoundation`:

- `Core` - types, asserts, handle, project paths.
- `Diagnostics` - profiler stubs and scoped timer.
- `Logging` - log facade backed by spdlog.
- `Math` - GLM-backed math aliases, camera-matrix helpers, glTF conversion.

## 2. Responsibilities

- Provide engine-wide types (`u8/u16/u32/u64`, `f32`, `Vec*`, `Mat*`,
  `Handle<T>`, etc.).
- Provide logging and asserts that the rest of the engine calls.
- Provide the math helpers used everywhere (`ComposeTRS`, `TransformAABB`,
  `CombineAABB`, `PerspectiveLH_ZO`, `OrthographicLH_ZO`,
  `BuildViewMatrixLH_XForward`, `LookAtLH_XForward`, `GetForwardVector`,
  `GetRightVector`, `GetUpVector`).
- Provide coordinate conversion (`GltfPositionToXEngine`,
  `GltfDirectionToXEngine`, `GltfTangentToXEngine`).

## 3. Non-Responsibilities

- Does not know about scenes, assets, RHI, renderer, GPU, or platform windowing.
- Does not allocate GPU memory.
- Does not implement the Engine class. The Engine class lives in
  `Runtime/Engine` (target `XEngineCoreRuntime`); Core is its dependency.

## 4. Public API Surface

| Header | Type | Purpose |
|---|---|---|
| `XEngine/Core/Types.h` | `u8/u16/u32/u64`, `f32`, `bool`, sized vector | Sized primitives. |
| `XEngine/Core/Defines.h` | `XENGINE_*` build flags | Compile-time switches. |
| `XEngine/Core/Base.h` | `NonCopyable`, base macros | Inheritance/utility macros. |
| `XEngine/Core/Handle.h` | `template<class T> Handle<T>` | Strongly typed handle. |
| `XEngine/Core/Assert.h` | `XENGINE_ASSERT(cond, msg)` | Debug-only assertion. |
| `XEngine/Core/Result.h` | `Result<T>` | Lightweight outcome type. |
| `XEngine/Core/ProjectPaths.h` | `class ProjectPaths` | Project-aware path helper. |
| `XEngine/Core/UUID.h` | `class UUID` | Stable identifier. |
| `XEngine/Core/Colors.h` | Color constants | Color helpers. |
| `XEngine/Logging/Log.h` | `class Log` static | Trace/Debug/Info/Warn/Error/Critical. |
| `XEngine/Diagnostics/Profiler.h` | `class Profiler` | Profiler stubs. |
| `XEngine/Diagnostics/ScopedTimer.h` | `ScopedTimer` | RAII scope timer. |
| `XEngine/Math/MathTypes.h` | `Vec2/3/4`, `Mat3/4`, `Quat` | GLM aliases. |
| `XEngine/Math/MathFunctions.h` | `ComposeTRS`, transforms, etc. | Math ops. |
| `XEngine/Math/CameraMatrices.h` | `PerspectiveLH_ZO`, etc. | Projection/view helpers. |
| `XEngine/Math/CoordinateSystem.h` | `Math::GetForwardVector` etc. | Basis conventions. |
| `XEngine/Math/CoordinateConversion.h` | `Gltf*ToXEngine` | glTF -> XEngine bridge. |
| `XEngine/Math/AABB.h` | `struct AABB`, helpers | Bounding box. |
| `XEngine/Math/Frustum.h` | `struct Frustum` | Frustum helper (placeholder). |
| `XEngine/Math/Rotator.h` | `struct Rotator` | Euler rotator. |
| `XEngine/Math/Math.h`, `Transform.h` | convenience includes | Re-exports. |

## 5. Dependencies

### Depends on

- `glm` (PUBLIC).
- `spdlog` (PRIVATE for `Log.cpp`).

### Used by

- Every other Runtime module (via `XEngineRuntime` aggregate).
- Apps (via `XEngineRuntime`).

## 6. Ownership and Lifetime

- Log state is a static singleton; `Log::Initialize` / `Log::Shutdown` are
  called from `Engine::Initialize` / `Engine::Shutdown`
  (`Engine/Private/Engine.cpp:48-?`).
- `ProjectPaths::Initialize` registers the canonical paths; called from
  `Engine::Initialize`.
- Handle<T> is a value type; no ownership lives here.
- AABB / Frustum are POD; no ownership.

## 7. Runtime Flow

- `Engine::Initialize` calls `Log::Initialize()` and `ProjectPaths::Initialize()`
  before any subsystem starts.
- During normal tick, `Log::Info/Warn/Error` macro calls originate from
  every module and pass through Core.
- `Math::*` is called anywhere a matrix, vector, basis, or frame conversion
  is needed; the helpers are inline and have no allocation.

## 8. Important Invariants

- All handles returned by Core are value types and must be carried by the
  caller; `IsValid()` is the only canonical "non-null" check.
- Logging macros are thread-safe via spdlog's sink but the codebase
  currently assumes a single producer thread for diagnostics.
- Coordinate conversion is the **only** place where glTF or other source
  conventions meet XEngine. Any new importer must normalize at this
  boundary; downstream code assumes XEngine convention.

## 9. Main Classes and Collaborators

- `Log` (static facade).
- `Handle<T>` (templated).
- `AABB`, `Frustum`.
- `Math::*` helpers.
- `CoordinateConversion` (glTF bridge).

## 10. Design Rationale

- The leaf module keeps Vulkan / Slang / SDL3 / ImGui out of the picture so
  every higher module has a stable base.
- Math aliases prevent scattering `glm::vec3 / glm::mat4` through code; the
  named functions also document the conventions.
- Coordinate conversion is a function-only boundary; no shared mutable state.

### Alternatives considered

- Vendor a fully custom math library. Rejected: GLM is well-tested and
  column-major, matching the convention the rest of the engine assumes.
- Split Foundation into four libraries. Rejected: they have no observable
  boundary between them; combining them keeps link time short.

### Trade-offs

- No custom math primitive types means we have less freedom to enforce
  invariants at the type level.
- spdlog is a heavy dependency for a thin facade, but threading and
  formatting make it worthwhile.

## 11. Failure Modes and Debugging

- If `Log::Initialize` is not called, the static facades no-op. This is
  intentional in tests; watch the build flags.
- A `XENGINE_ASSERT` failure calls `ReportAssertionFailure` and aborts.
  Bug reports should capture the assertion message and the call site.

## 12. Current Limitations

- `Frustum::Cull` is a placeholder; culling math is not implemented.
- `Rotator` is not yet used widely; degree/radian conversions live in
  `MathFunctions.h`.
- No SIMD-aware math.

## 13. Source References

- `Engine/Source/Foundation/Core/Public/XEngine/Core/*.h`
- `Engine/Source/Foundation/Core/Private/{Assert.cpp,ProjectPaths.cpp,UUID.cpp}`
- `Engine/Source/Foundation/Logging/Public/XEngine/Logging/Log.h`
- `Engine/Source/Foundation/Logging/Private/Log.cpp`
- `Engine/Source/Foundation/Diagnostics/Public/XEngine/Diagnostics/*.h`
- `Engine/Source/Foundation/Diagnostics/Private/Profiler.cpp`
- `Engine/Source/Foundation/Math/Public/XEngine/Math/*.h`
- `Engine/Source/Foundation/CMakeLists.txt`

## 14. Future Work

- Static analysers could enforce that calling code never mixes `glm::*` and
  `XEngine::Math::*`.
- `Frustum` culling math should land with the future RenderFeature work.
