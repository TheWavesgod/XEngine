# Asset

## 1. Module Purpose

Asset owns the CPU-side records for textures, meshes, materials, scenes, and
shaders that the renderer ultimately consumes. The module is the source of
truth for asset metadata and is also the only place where source-of-truth
formats (glTF, png) are decoded.

The module exposes an `ISubsystem` (`AssetSystem`) that registers all assets
during engine startup. Render-side callers reach it through narrow read APIs
(`GetMeshAsset`, `GetMaterialAsset`, `GetTextureAsset`).

## 2. Responsibilities

- Register source-of-truth assets (glTF files, procedural meshes, test
  materials).
- Provide `AssetMetadata { Handle, Type, SourcePath, Name, LoadState,
  Dependencies }`.
- Hold CPU-side records (`TextureAsset`, `MeshAsset`, `MaterialAsset`,
  `SceneAsset`, `ShaderAsset` - last two are placeholders).
- Run importers: `GltfImporter`, `ImageImporter`, `MaterialImporter`,
  `TextureImporter`, `StbImageImporter`.
- Maintain a `AssetHandle` -> record lookup.

## 3. Non-Responsibilities

- Does not know about RHI, Renderer, GPU, Vulkan, Slang.
- Does not own GPU-side textures, meshes, or materials; the Renderer
  owns those via `RenderTextureManager`, `RenderMeshManager`,
  `RenderMaterialSystem`.
- Does not define component types (those live in `Runtime/Scene`).
- Does not serialize asset records (that is the Serialization module's
  responsibility; currently only Scene uses serialization).

## 4. Public API Surface

`Engine/Source/Runtime/Asset/Public/XEngine/Asset/` exposes:

- `AssetSystem.h` - `class AssetSystem : public ISubsystem` with
  `RegisterSourceAsset`, `ImportAsset`, `GetMeshAsset(handle)`,
  `GetMaterialAsset(handle)`, `GetTextureAsset(handle)`,
  `CreateProceduralCubeMeshAsset`, `CreateTestMaterialAsset`.
- `AssetMetadata.h` - metadata struct.
- `AssetTypes.h`, `AssetHandle.h`, `AssetId.h`, `AssetImportTypes.h`,
  `AssetManager.h` - shared types and helpers.

Asset records under `Assets/`:

- `TextureAsset.h` - `struct TextureAsset { u32 Width/Height; Channels;
  TextureAssetFormat Format; bool IsSRGB; std::vector<u8> Pixels; }`.
- `MeshAsset.h` - `MeshVertex { Position, Normal, Tangent, TexCoord0 }`,
  `MeshSubmesh { FirstIndex, IndexCount, VertexOffset, MaterialSlot, Bounds }`,
  `MeshAsset { Name, SourcePath, Vertices, Indices, Submeshes, Bounds }`.
- `MaterialAsset.h` - `ShadingModel Unlit/Lit`, `AlphaMode Opaque/Masked/Blend`,
  factors, texture handles, `DoubleSided`.
- `SceneAsset.h`, `ShaderAsset.h` - placeholders.

## 5. Dependencies

### Depends on

- `XEngineFoundation` (log, asserts, project paths, math).
- `XEngineCoreRuntime` (include path).
- `fastgltf` (PRIVATE).
- `stb` (include path PRIVATE in `Importers/`).

### Used by

- `Runtime/Renderer` (PRIVATE) - `RenderExtraction` reads asset records; the
  resource managers call `AssetSystem::GetTexture/Mesh/MaterialAsset`.
- `Runtime/Scene` (PUBLIC) - `SceneSerializer` may need `AssetSystem`
  references for saved scenes.
- `Apps/Sandbox` and `Apps/EditorApp` (PUBLIC) - configure the registry.

## 6. Ownership and Lifetime

- `AssetSystem` lives across engine lifetime.
- Asset records live inside the registry; `AssetHandle`s are stable for
  the engine lifetime (assumed) but the design does not yet guarantee it
  across reload.
- GPU-side resources created from asset records are owned by the Renderer
  module via its managers.

## 7. Runtime Flow

- `Engine::Initialize` calls `AssetSystem::OnCreate` after `ShaderSystem`.
- During startup (e.g. in `Apps/Sandbox/Source/main.cpp` indirectly), the
  asset system imports the default scene.
- During a frame, `RenderExtraction` consumes asset records to build
  `RenderObject` entries; no asset IO happens at runtime.
- Renderer managers lazily create GPU resources on first access through
  `GetOrCreateXxxFromAsset`.

## 8. Important Invariants

- `AssetHandle` is stable for the lifetime of the registry; reusing a
  handle after destruction returns a stale record (no current code does this).
- Asset records stay CPU-only; their data has no GPU representation until
  the renderer ingests them.
- Public Asset headers do not leak third-party types; all importer-side
  types (fastgltf, stb) are confined to `Private/Importers/`.

## 9. Main Classes and Collaborators

- `AssetSystem`.
- `TextureAsset`, `MeshAsset`, `MaterialAsset` (and the placeholder
  record types).
- Importers: `GltfImporter`, `ImageImporter`, `MaterialImporter`,
  `TextureImporter`, `StbImageImporter`.

## 10. Design Rationale

- Asset registry stays away from runtime hot path: importers do not run
  during rendering. Asset-loaded handlers are cheap lookup.
- Importer behind private API means future formats (e.g. KTX, FBX) slot in
  without changing the public surface.

### Alternatives considered

- Asset records owned by Renderer. Rejected: Renderer would then need to
  know about source formats, blurring the boundary.
- Lazy asset loading via `LoadAsync`. Deferred to a future stage.

### Trade-offs

- All assets are imported eagerly during engine init; this keeps the
  initial frame deterministic but lengthens startup. Profiling TBD.

## 11. Failure Modes and Debugging

- Missing source path: importer logs and skips; downstream callers see a
  null `AssetHandle`.
- Corrupted glTF: importer logs and returns an empty `MeshAsset`.
- Texture format not supported by importer: falls back to a default
  (currently no fallback; verify when adding formats).

## 12. Current Limitations

- `SceneAsset` and `ShaderAsset` are placeholders; no round-trip save.
- No async import.
- No dependency-based prefetch.

## 13. Source References

- `Engine/Source/Runtime/Asset/Public/XEngine/Asset/AssetSystem.h`
- `Engine/Source/Runtime/Asset/Public/XEngine/Asset/Assets/*.h`
- `Engine/Source/Runtime/Asset/Private/AssetSystem.cpp`
- `Engine/Source/Runtime/Asset/Private/AssetDatabase.cpp`
- `Engine/Source/Runtime/Asset/Private/AssetRegistry.cpp`
- `Engine/Source/Runtime/Asset/Private/Importers/*.cpp`
- `Engine/Source/Runtime/Asset/CMakeLists.txt`

## 14. Future Work

- Async import with `JobHandle` (the `JobSystem` skeleton in
  `Runtime/JobSystem` will be the host).
- Save/load round-trip using the Serialization module.
- Asset dependency graph and partial reload.
