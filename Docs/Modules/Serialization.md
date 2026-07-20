# Serialization

## 1. Module Purpose

Serialization provides a thin JSON read/write wrapper around `nlohmann::json` and
the version + context helpers used by the Scene module. It exists so that
Asset and Scene files use the same file IO + schema-versioning conventions.

## 2. Responsibilities

- JSON file read/write (`JsonSerialization.h`).
- `SerializationContext` (per save/load session) carrying the current
  `AssetSystem*` so nested asset references can be resolved.
- Schema version constant `XSceneSerializationVersion = 1` for scene save
  compatibility checks.

## 3. Non-Responsibilities

- Does not implement binary serialization.
- Does not implement asset registration or asset IO. Asset is its own
  module; Serialization only knows about JSON files and contexts.
- Does not know about RHI or renderer.

## 4. Public API Surface

`Engine/Source/Runtime/Serialization/Public/XEngine/Serialization/`:

- `JsonSerialization.h` - `using Json = nlohmann::json;`,
  `LoadJsonFile(path, outJson)`, `SaveJsonFile(path, json)`.
- `SerializationContext.h` - `struct SerializationContext { AssetSystem* Assets; ... }`.
- `SerializationVersion.h` - `constexpr u32 XSceneSerializationVersion = 1`.

Private:

- `JsonSerialization.cpp` - implementation backed by `nlohmann::json`
  (CMake dependency `ThirdParty_json`).

## 5. Dependencies

### Depends on

- `XEngineFoundation` (log, asserts, project paths).
- `XEngineCoreRuntime` (include path).
- `ThirdParty_json` (PUBLIC).

### Used by

- `Runtime/Scene` (`SceneSerializer`) reads and writes `*.xscene` JSON.
- `Apps/Sandbox` constructs the `SerializationContext` to call
  `SceneSerializer::LoadFromFile`.
- `Runtime/Asset` may use JSON for in-memory metadata (it has its own
  database/registry classes too).

## 6. Ownership and Lifetime

- `SerializationContext` is a short-lived value object passed by reference
  during save/load.
- The `Json` type from nlohmann holds memory internally; ownership is
  transferred when the type is copied or moved.
- No module-owned long-lived state.

## 7. Runtime Flow

- `Apps/Sandbox/Source/main.cpp` constructs `SerializationContext { Assets }`
  and calls `SceneSerializer::LoadFromFile(*scene, "asset://Scenes/Default.xscene")`
  during startup.
- On save (not yet exercised in the live code), the context is again
  constructed and passed to `SceneSerializer::SaveToFile`.

## 8. Important Invariants

- The on-disk schema must bump `XSceneSerializationVersion` whenever a
  non-backward-compatible field is added.
- All asset references in serialized scenes are stored as `AssetHandle`s,
  never as raw paths.

## 9. Main Classes and Collaborators

- `LoadJsonFile`, `SaveJsonFile`.
- `SerializationContext`.
- `XSceneSerializationVersion`.

## 10. Design Rationale

- A flat JSON API gives simple round-tripping and human-readable saves
  for the V0 stage.
- `nlohmann::json` is the dependency; using the same library everywhere
  avoids format conflicts.

### Alternatives considered

- A binary format for save files. Rejected for V0: JSON is debug-friendly
  and the schema complexity is low.
- A custom JSON subset. Rejected: nlohmann already covers the needs.

### Trade-offs

- JSON is slower than binary; for a learning engine that is acceptable.
- The `SerializationContext` pattern is heavier than passing `AssetSystem*`
  directly but future-proofs when more services may need to participate.

## 11. Failure Modes and Debugging

- Missing/corrupted JSON file: `LoadJsonFile` returns `false`; callers
  log a warning and fall back to a default scene.
- Version mismatch: `SceneSerializer` is the place that compares
  versions (current behavior is "ignore", which should become "warn or
  refuse to load").

## 12. Current Limitations

- Only Scene uses serialization; Asset records currently live entirely
  in the runtime registry.
- No streaming or partial save/load.

## 13. Source References

- `Engine/Source/Runtime/Serialization/Public/XEngine/Serialization/*.h`
- `Engine/Source/Runtime/Serialization/Private/JsonSerialization.cpp`
- `Engine/Source/Runtime/Serialization/CMakeLists.txt`

## 14. Future Work

- Add backward-compatible version migration helpers.
- Use Serialization as the Asset module's metadata backend once the
  asset registry grows.
