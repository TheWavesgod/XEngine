# Scene

## 1. Module Purpose

Scene is the gameplay-domain entity/component hierarchy. It owns `Entity`,
`Scene`, all the standard component records (NameComponent (placeholder),
TransformComponent, MeshRendererComponent, CameraComponent,
LightComponent), the per-frame TransformSystem, the DebugCameraController,
and the scene serializer.

The runtime representation is CPU-only; the module has zero RHI or Renderer
dependencies.

## 2. Responsibilities

- Manage entities, their components, and parent/child hierarchy.
- Drive `TransformSystem::Update(Scene&)` each frame to refresh cached
  world transforms.
- Run `SceneSystem::FrameDebugCamera` to update the editor/debug free camera.
- Serialize and deserialize the entity graph to/from `.xscene` files.

## 3. Non-Responsibilities

- No RHI, Vulkan, GPU, Shader, Renderer references in `Public/`.
- Scene component data does not store GPU-side state.
- Scene does not own render resources; it is consumed read-only by
  `RenderExtraction`.

## 4. Public API Surface

`Engine/Source/Runtime/Scene/Public/XEngine/Scene/`:

- `Scene.h` - `class Scene` with `CreateEntity / DestroyEntity`,
  `AddTransform / MeshRenderer / Camera / Light`,
  `Get*Component*`, `SetParent / ClearParent / HasParent / GetParent /
  GetChildren / GetRootEntities / IsDescendantOf`, world/local setters,
  `UpdateTransforms`, plus internal `m_Parents`, `m_Children`, and
  `m_RootEntities` maps.
- `Entity.h` - `struct Entity { u32 Index, Generation; }` plus
  `InvalidEntityIndex`.
- `SceneSystem.h` - `class SceneSystem : public ISubsystem` with
  `m_ActiveScene`, `DebugCameraController`, `GetPrimaryCameraEntity`,
  `GetPrimaryCameraTransform`, `FrameDebugCamera`.
- `SceneSerializer.h` - `class SceneSerializer { SerializationContext m_Context; }`.
- `DebugCameraController.h` - UE-style free-camera controller.
- `Components.h` - umbrella include.
- `Components/*.h` - one header per component type.

Private:

- `Private/Scene.cpp`, `Private/Entity.cpp`, `Private/SceneSystem.cpp`,
  `Private/DebugCameraController.cpp`.
- `Private/Serialization/SceneSerializer.cpp` - JSON round-trip.
- `Private/Systems/TransformSystem.cpp` - per-frame hierarchy update.

## 5. Dependencies

### Depends on

- `XEngineFoundation` (log, asserts, math).
- `XEngineCoreRuntime` (include path).
- `XEngineSerialization` (PUBLIC).
- `XEngineAsset` (PUBLIC) - components reference `AssetHandle`s.
- `XEngineInput` (PUBLIC) - used by `DebugCameraController` and
  `SceneSystem::FrameDebugCamera`.

### Used by

- `Runtime/Renderer` (PRIVATE) - `RenderExtraction` reads `Scene` entity
  data.
- `Apps/Sandbox` (PUBLIC) - loads `Default.xscene`, drives the active
  scene.
- `Apps/EditorApp` (PUBLIC) - drives the same via `EditorApplication`.

## 6. Ownership and Lifetime

- `SceneSystem` owns exactly one `Scene` instance (`m_ActiveScene`).
- `Scene` owns entities and their components. Components are value types;
  the storage is `std::unordered_map<Entity, std::unique_ptr<TComponent>>`
  in `Scene.cpp` (described by source).
- Hierarchy is owned by `Scene` via `m_Parents` / `m_Children`.
- `SceneSerializer` borrows the `AssetSystem*` from `SerializationContext`
  to resolve `AssetHandle`s.

## 7. Runtime Flow

- `Engine::Initialize` constructs `SceneSystem` and calls `OnCreate`.
- During `Engine::Run`, the implicit-per-frame order inside
  `SceneSystem::OnUpdate` (or called by parent) does:
  1. `TransformSystem::Update(scene)` - recomputes world matrices for
     dirty entities recursively.
  2. `SceneSystem::FrameDebugCamera(scene, dt)` - advances the free-camera
     controller if any.
- The Renderer reads the scene inside `RenderExtraction::Extract(scene, ...)`.

## 8. Important Invariants

- Every entity has a `TransformComponent` (`Scene.cpp` lookup defaults to
  identity if missing).
- World matrices are cached on `TransformComponent` and marked dirty when
  local setters are called or when the hierarchy changes.
- `Entity::Generation` increments on destroy so stale handles can be
  detected.
- `SceneSystem::GetPrimaryCamera()` walks `CameraComponent::Primary` to
  find exactly one active camera.

## 9. Main Classes and Collaborators

- `Scene`, `Entity`, `TransformComponent`, `MeshRendererComponent`,
  `CameraComponent`, `LightComponent`.
- `TransformSystem`.
- `SceneSystem`, `DebugCameraController`.
- `SceneSerializer`.

## 10. Design Rationale

- Hierarchical transforms are owned inside `Scene` rather than in a
  `HierarchyComponent`; the latter would re-create parent/child coupling
  inside a per-entity component, which is harder to keep in sync with
  world matrix propagation.
- Free-camera (debug) lives in this module because it depends on
  InputSystem and Scene, not on RHI.

### Alternatives considered

- Per-component System classes in addition to `TransformSystem`. Deferred
  to a future RenderFeature / RenderPass overhaul.
- Storing entity data in flat packed arrays (ECS style). Rejected for V0
  because the entity count is small.

### Trade-offs

- Hierarchy traversal is recursive; fine for small graphs, would need
  flattening for very large scenes.

## 11. Failure Modes and Debugging

- Entity destruction mid-frame: `Entity::Generation` keeps the old handle
  valid-but-dead until reused; readers should check.
- Missing camera: `RenderSystem` falls back to `FallbackViewProjection`.

## 12. Current Limitations

- `NameComponent` is a placeholder.
- No frustum culling helper (`Frustum` placeholder in Foundation).
- No entity save/load diffing.

## 13. Source References

- `Engine/Source/Runtime/Scene/Public/XEngine/Scene/*.h`
- `Engine/Source/Runtime/Scene/Private/*.cpp`
- `Engine/Source/Runtime/Scene/CMakeLists.txt`

## 14. Future Work

- Component-level System classes (Physics, Animation).
- Save/load diffing through `Serialization` module.
- Frustum culling integrated with the future RenderFeature.
