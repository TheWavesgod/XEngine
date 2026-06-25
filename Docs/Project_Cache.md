# XEngine Project Cache for AI Coding Assistants

## Project Identity

XEngine is a learning-oriented but architecture-conscious C++20 game engine.

The current goal is to build a modular Vulkan-first renderer with a clean Runtime / Editor / Sandbox split. The engine is not trying to become a production engine immediately, but the code should be structured so that future features such as RenderGraph V1, RenderFeature, CSM, IBL, TAA, temporal upscaling, GPUScene, ray tracing, and editor tools can be added without major rewrites.

## Current Technology Direction

* Language: C++20
* Platform layer: SDL3
* Graphics backend: Vulkan first
* Future graphics backends: D3D12 and Metal
* Shader system: Slang
* Math backend: GLM, exposed through XEngine-facing aliases and helper functions
* Rendering style: Forward renderer first
* Editor UI: ImGui
* Asset pipeline: custom Asset module, with glTF importer support
* Runtime structure: separated Runtime, Editor, and Sandbox targets

## Coordinate System

XEngine world convention:

```text
+X = Forward
+Y = Right
+Z = Up
Left-handed world convention
```

Scene, Asset, and Renderer world-space code should use this convention.

Backend-specific clip-space, viewport, and projection differences must be handled at the RHI/projection boundary, not inside Scene or Asset logic.

## Module Boundaries

### Core / Math

Core and Math provide common types, utilities, logging, assertions, filesystem helpers, and math aliases.

Use XEngine-facing math aliases:

```cpp
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;
using Quat = glm::quat;
```

Project code should prefer `XEngine::Math` helpers over scattered direct `glm::` calls.

Important helpers include:

```text
ComposeTRS
TransformAABB
CombineAABB
PerspectiveLH_ZO
OrthographicLH_ZO
BuildViewMatrixLH_XForward
GetForwardVector
GetRightVector
GetUpVector
```

Do not introduce custom Vec3/Mat4 wrapper classes unless explicitly requested.

### Asset Module

Asset owns persistent imported CPU-side data.

Examples:

```text
TextureAsset
MeshAsset
MaterialAsset
future SceneAsset
future MaterialInstanceAsset
```

Asset public headers must not expose Renderer, RHI, Vulkan, stb, fastgltf, or Slang implementation details.

Asset may contain private importers.

### Scene Module

Scene owns entity/world/component state.

Examples:

```text
Scene
Entity
NameComponent
TransformComponent
MeshRendererComponent
CameraComponent
LightComponent
TransformSystem
DebugCameraController
```

Scene must not depend on Renderer or RHI.

Scene does not know RenderScene, RenderObject, RenderLight, CSM, shadow textures, GPU buffers, or pipelines.

Scene hierarchy is owned by Scene, not by a public HierarchyComponent.

Scene should support parent/child relationships internally using parent and children maps.

### Renderer Module

Renderer owns extracted frame data and renderer resources.

Examples:

```text
RenderSystem
RenderScene
RenderObject
RenderLight
RenderExtraction
RenderTextureManager
RenderMeshManager
RenderMaterialSystem
RenderShaderLibrary
RenderPipelineStateCache
RenderFrameResources
ForwardRenderPipeline
RenderShadowManager future
RenderDebugDraw future
```

Renderer can read Scene and Asset data. Scene and Asset must not depend on Renderer.

Renderer owns frame data, render resources, render pass scheduling, GPU-visible frame data, and render debug settings.

### RHI Module

RHI owns backend-agnostic GPU abstractions.

Examples:

```text
RHIDevice
RHITexture
RHITextureView
RHIBuffer
RHIShader
RHIGraphicsPipeline
RHICommandList
RHISampler
```

RHI must not know Scene, Renderer feature names, CSM, cascades, materials, or engine gameplay concepts.

It should expose generic capabilities such as texture arrays, depth attachments, sampled textures, samplers, and pipeline states.

### Editor Module

Editor depends on Runtime.

Editor owns:

```text
ImGui integration
EditorApplication
EditorContext
SceneHierarchyPanel
InspectorPanel
RendererDebugPanel
editor selection state
editor layout config
editor-only UI commands
```

Editor may call Runtime APIs such as SceneSerializer and RendererDebugSettings.

Runtime must not depend on Editor.

### Sandbox

Sandbox depends on Runtime.

Sandbox should load scenes and run them. It usually needs scene deserialization, but not scene saving or editor UI.

## Current Renderer Architecture

RenderSystem should be a high-level coordinator.

Long-lived renderer managers/resources should be owned by RenderSystem:

```text
RenderScene
RenderTextureManager
RenderMeshManager
RenderMaterialSystem
RenderShaderLibrary
RenderPipelineStateCache
RenderFrameResources
ForwardRenderPipeline
future RenderShadowManager
future RenderDebugDraw
```

ForwardRenderPipeline currently owns a simple linear RenderGraph and manually adds passes.

RenderGraph is still V0 / linear. Do not implement full RenderGraph V1 unless the current stage explicitly says so.

## Render Resource Context

Renderer passes should access shared renderer services through RenderResourceContext.

Expected shape:

```cpp
struct RenderResourceContext
{
    RenderTextureManager* Textures = nullptr;
    RenderMeshManager* Meshes = nullptr;
    RenderMaterialSystem* Materials = nullptr;
    RenderShaderLibrary* Shaders = nullptr;
    RenderPipelineStateCache* PipelineStates = nullptr;
    RenderFrameResources* FrameResources = nullptr;

    // Stage 9 and later:
    RenderShadowManager* Shadows = nullptr;
};
```

Do not pass many unrelated manager pointers separately if RenderResourceContext is available.

## Pipeline Naming Rules

Use these names consistently:

```text
RenderPipeline / ForwardRenderPipeline:
  frame composition strategy

RenderPipelineStateCache:
  caches RHIGraphicsPipeline objects

GraphicsPipelineStateKey:
  key for graphics pipeline state

RenderShaderLibrary:
  caches RHIShader objects
```

Do not introduce a confusing `RenderPipelineCache` name.

Native backend pipeline cache can be added later as an RHI/backend feature, not now.

## Shader Organization

Preferred shader layout:

```text
Assets/Shaders/Common/
  Types.slang
  Math.slang
  Constants.slang

Assets/Shaders/Lighting/
  LightingTypes.slang
  BRDF.slang
  Lighting.slang
  ShadowTypes.slang future
  ShadowSampling.slang future

Assets/Shaders/Materials/
  MaterialTypes.slang
  PBRMaterial.slang

Assets/Shaders/Passes/
  ForwardPBR.slang
  ShadowDepth.slang future
  DebugLine.slang future
  UnlitTextured.slang
```

BRDF.slang should remain pure lighting math. It should not contain bindings, shadow texture sampling, pass logic, or material loading.

Pass shaders should be thin entry points.

## Binding Convention

Use this global convention unless explicitly changed:

```text
Set 0 = per-frame global data
  camera
  lighting
  shadow data future
  global textures such as shadow map future

Set 1 = material data
  material constants
  material textures
  material samplers

Set 2 = object data
  object constants, object buffer, or push constants
```

Lighting and shadows are frame/global data, not material data.

## Transform Rules

TransformComponent should store local TRS and cached world TRS/matrices.

Recommended fields:

```text
LocalPosition
LocalRotation
LocalScale
WorldPosition
WorldRotation
WorldScale
LocalMatrix
WorldMatrix
Dirty flag
```

Local setters belong on TransformComponent.

World setters should live in Scene/TransformSystem or a scene-level transform API because parent context is required.

World getters can be on TransformComponent.

Rotation should support a user-friendly degree-based Rotator:

```text
Roll  = rotation around +X / Forward
Pitch = rotation around +Y / Right
Yaw   = rotation around +Z / Up
```

Composition order:

```text
q = qYawZ * qPitchY * qRollX
```

## Render Extraction Rules

RenderExtraction belongs in Renderer, not Scene.

Correct flow:

```text
Scene -> RenderExtraction -> RenderScene
```

RenderExtraction should create:

```text
RenderObject
RenderLight
camera/frame data inputs
```

RenderExtraction should use TransformComponent world matrices, not recompute ad-hoc transform logic.

## GPU Data Rules

GPU-visible structs should live near renderer resource/frame data, for example:

```text
Renderer/Private/Resources/RenderGPUData.h
```

Use `alignas(16)` and packed Vec4 fields where practical.

Do not invent separate GPU matrix/vector wrappers unless layout genuinely differs.

Keep CPU-side renderer frame data separate from GPU-packed data.

Examples:

```text
RenderShadowFrameData:
  CPU-side renderer data

GPUShadowData:
  shader-visible packed data
```

## Scene Serialization Direction

Serialization infrastructure should be a Runtime module.

Recommended split:

```text
Runtime/Serialization:
  generic JSON/archive/context/helpers

Runtime/Scene:
  SceneSerializer

Editor:
  calls SceneSerializer for load/save

Sandbox:
  calls SceneSerializer for load only
```

Do not put SceneSerializer inside Editor only, because Sandbox also needs scene loading.

Do not make Runtime depend on Editor.

## Editor Config Direction

Editor default config should be separated from user/saved config.

Recommended paths:

```text
Config/Editor/DefaultDocking.ini
Config/Editor/DefaultEditorSettings.json

Saved/Config/Editor/Docking.ini
Saved/Config/Editor/EditorSettings.json
```

Default config is committed to git.

Saved config is user-local and should be gitignored.

ImGui docking layout should load user layout first, then fallback to default layout.

On shutdown, save current layout to Saved config, not Default config.

## General Coding Rules for AI Assistants

When modifying XEngine:

```text
1. Do not invent unrelated architecture.
2. Do not move files across modules unless requested.
3. Do not introduce Scene -> Renderer or Asset -> Renderer dependencies.
4. Do not expose Vulkan types in public Runtime interfaces.
5. Do not put backend-specific code in public headers.
6. Do not add large systems outside the current stage.
7. Prefer small, reviewable changes.
8. Keep compile boundaries clean.
9. Update CMake/Xcode project files when adding/removing source files.
10. Add comments only where they clarify ownership, data layout, or non-obvious rendering logic.
11. Do not add noisy comments that restate obvious code.
12. If API is missing, first propose the minimal API change before using it.
13. If unsure about existing code, inspect the repo before editing.
14. Preserve current working functionality unless the task explicitly asks to replace it.
15. Keep stage scope strict.
```

## Preferred AI Workflow

For each task:

```text
1. Read project cache.
2. Read current stage brief.
3. Inspect relevant existing files.
4. Summarize current state briefly.
5. Propose implementation plan.
6. Wait for confirmation if the change is large.
7. Implement in small batches.
8. Compile or provide likely compile issues.
9. Summarize changed files and architectural impact.
```
