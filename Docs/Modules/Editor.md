# Editor

## 1. Module Purpose

The Editor module hosts the ImGui-driven editor shipped by the
`XEngineEditorApp` executable. It is built only when
`XENGINE_ENABLE_EDITOR` is ON and links only against the public Engine
runtime plus ImGui.

The Editor does not interfere with Sandbox; both can coexist in the build
output but cannot co-execute.

## 2. Responsibilities

- Provide `EditorApplication` (drives `Engine::Run` with editor-only
  config).
- Provide `EditorSystem` (subsystem registered by the Editor app, owns
  editor-only state such as the active scene, layout, and panel state).
- Render ImGui panels (SceneHierarchy, Inspector, Viewport, etc.) on top
  of the Vulkan swapchain image through
  `RHIDevice::RenderVulkanOverlay`.
- Provide docking layout persistence (default in
  `Config/Editor/DefaultDocking.ini`, user-saved in
  `Saved/Config/Editor/Docking.ini`).

## 3. Non-Responsibilities

- Does not modify the Engine class.
- Does not replace the Renderer or Scene module; only reads from them
  through their public APIs.
- Does not run as part of the Sandbox target; `XENGINE_ENABLE_EDITOR`
  controls the boundary.
- Does not contain gameplay logic.

## 4. Public API Surface

`Engine/Source/Editor/Public/XEngine/Editor/`:

- `EditorApplication.h` - the entry-point class for `Apps/EditorApp`.
- `EditorSystem.h` - the editor subsystem (registered by the Editor
  app via `EngineConfig::ConfigureSubsystems`).
- `EditorContext.h`, `EditorCamera.h`.

Private:

- Many panel implementations (SceneHierarchyPanel, InspectorPanel,
  ViewportPanel, etc.) and ImGui under `Engine/Source/Editor/Private/`.

## 5. Dependencies

### Depends on

- `XEngineRuntime` (PUBLIC).
- `ThirdParty_imgui` (PRIVATE).

### Used by

- `Apps/EditorApp` (PUBLIC).
- Not used by Sandbox.

## 6. Ownership and Lifetime

- `EditorApplication` owns an `Engine` instance and drives its
  `Initialize / Run / Shutdown`.
- `EditorSystem` owns editor-only state and is registered at runtime
  through the `EngineConfig::ConfigureSubsystems` lambda.
- The Editor is optional; omitting `XENGINE_ENABLE_EDITOR` removes the
  library and the executable target.

## 7. Runtime Flow

- `Apps/EditorApp/Source/main.cpp` constructs `EditorApplication` and
  invokes `Run()`.
- `EditorApplication::Run` calls `Engine::Initialize` with
  `EnableEditor=true` and registers `EditorSystem`.
- Each frame:
  1. Engine subsystems tick (Render etc.).
  2. `EditorSystem::OnUpdate` updates panel state and calls the
     `OverlayCallback` set on the renderer.
  3. The renderer's callback is invoked from
     `RHIDevice::RenderVulkanOverlay`, which runs ImGui into the swapchain
     image just before present.
- On shutdown, ImGui state is saved to `Saved/Config/Editor/Docking.ini`.

## 8. Important Invariants

- ImGui panels must consume `RHINativeCommandBuffer` (uintptr_t) when
  drawing through `RHIDevice::RenderVulkanOverlay`; they must never
  include `vulkan.h` directly.
- Editor code must not allocate RHI resources outside the public
  resource factories.
- Docking layout is loaded from user config first, with fallback to
  default config (`Config/Editor/DefaultDocking.ini`).

## 9. Main Classes and Collaborators

- `EditorApplication`, `EditorSystem`.
- Each panel owns its own ImGui state and is updated by `EditorSystem`.

## 10. Design Rationale

- The Editor lives in its own library so Sandbox can ship without
  ImGui dependencies.
- `EditorSystem` is a regular subsystem so it can subscribe to the
  engine lifecycle and avoid coupling to internals.

### Alternatives considered

- A separate process driving the Editor while the sandbox is another
  process. Rejected for V0: single-process tool flow simplifies scene
  state and asset references.
- Editor as part of Runtime. Rejected: violates Runtime / Editor
  boundary.

### Trade-offs

- ImGui has its own context, separate from any future in-engine HUD.
- Vulkan-only path for overlays; D3D12 / Metal backends would each need
  their own equivalent of `RHIDevice::RenderVulkanOverlay`.

## 11. Failure Modes and Debugging

- Layout corruption: user-saved `Docking.ini` is the first attempt;
  fallback is default config. If even default fails, ImGui draws a blank
  viewport at next launch.
- Overlay callback failing: the editor's overlay renders nothing and
  the swapchain presents the same scene as without the overlay.

## 12. Current Limitations

- Editor and Sandbox share the same scene loading pipeline; there is no
  dedicated editor-only scene save/load flow yet.
- Asset registry does not currently expose hot-reload from the editor.
- Vulkan overlay path: D3D12 / Metal equivalents do not exist yet.

## 13. Source References

- `Engine/Source/Editor/Public/XEngine/Editor/*.h`
- `Engine/Source/Editor/Private/` (panel implementations).
- `Engine/Source/Editor/CMakeLists.txt`
- `Apps/EditorApp/Source/main.cpp`
- `Apps/EditorApp/CMakeLists.txt`

## 14. Future Work

- Multi-window editor with separate viewports.
- Editor-only scene save round-trip through `Serialization`.
- Asset hot-reload driven by the editor UI.
