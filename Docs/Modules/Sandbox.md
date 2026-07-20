# Sandbox

## 1. Module Purpose

Sandbox is the headless demo app. It links against the public Engine
runtime (`XEngineRuntime`), constructs the engine, loads the default
`Default.xscene`, and runs the forward pipeline. It does not pull in the
Editor library.

## 2. Responsibilities

- Provide `main.cpp` (entry point).
- Construct `EngineConfig` with a 1280x720 window, SDL3 platform,
  Vulkan backend, no editor.
- Construct `Engine` and call `Initialize / Run / Shutdown`.
- Load `Default.xscene` via `SceneSerializer` before `Run`.

## 3. Non-Responsibilities

- Does not contain ImGui or any editor functionality.
- Does not contain UI panels; only the swapchain-rendered scene.
- Does not contain gameplay logic.

## 4. Public API Surface

Single entry point:

- `Apps/Sandbox/Source/main.cpp` - `int main()`.

CMake target `XEngineSandbox`.

## 5. Dependencies

### Depends on

- `XEngineRuntime` (the aggregate).

### Used by

- CI / smoke-test pipelines.
- Manual runs for verifying Forward rendering output.

## 6. Ownership and Lifetime

- Sandbox owns the `Engine` instance on the stack of `main`.
- The default scene lives in `Assets/Scenes/Default.xscene`.

## 7. Runtime Flow

- Construct config (`EngineConfig`).
- Construct engine and call `Initialize(config)`.
- Look up `SceneSystem` / `AssetSystem` from the engine.
- Build `SerializationContext { Assets = assetSystem }`.
- Call `SceneSerializer::LoadFromFile(*scene, "asset://Scenes/Default.xscene")`.
- Call `engine.Run()` which drives the per-frame loop (`BeginFrame` ->
  `OnUpdate` -> `EndFrame`).
- On `Run` returning, call `engine.Shutdown()`.

## 8. Important Invariants

- The Vulkan device initialization includes validation by default; CI
  may want to disable it via `EngineConfig::EnableValidation = false`.
- Sandbox assumes the asset assets and shaders are present at the
  expected paths.

## 9. Main Classes and Collaborators

- `Engine`, `EngineConfig`, `SceneSystem`, `AssetSystem`,
  `SceneSerializer`, `SerializationContext`.

## 10. Design Rationale

- A minimal app that exercises the engine end-to-end surfaces problems in
  startup, asset loading, scene loading, and the forward pipeline.
- Leaving the SceneSerializer / AssetSystem interaction alone lets a
  manual run verify round-tripping.

### Alternatives considered

- Embedding a scene directly into the binary. Rejected: the current
  loading path is the test that matters for correctness.

### Trade-offs

- Sandbox is the only realistic environment for catching startup bugs at
  the module boundary.

## 11. Failure Modes and Debugging

- Missing `Default.xscene`: `SceneSerializer::LoadFromFile` returns
  false; engine still runs (scene empty).
- Vulkan initialization failure: log printed, `main` returns non-zero.
- ImGui / Editor built but linked: not currently possible since
  Sandbox does not link the editor library.

## 12. Current Limitations

- No headless mode (no `--headless` flag); the SDK window is always
  created.
- No automated screenshot capture.

## 13. Source References

- `Apps/Sandbox/Source/main.cpp`
- `Apps/Sandbox/CMakeLists.txt`

## 14. Future Work

- Headless mode for CI screenshots / GPU-less sanity check.
- Boot scene passed on the CLI.
