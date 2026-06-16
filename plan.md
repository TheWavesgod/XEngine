# XEngine Development Plan

## Project Positioning

**XEngine** is a renderer-first learning engine.

The goal is not to clone Unreal Engine or Unity.
The goal is to build a clean, modular, modern rendering engine architecture step by step.

Core direction:

```text
C++20
SDL3 platform layer
Vulkan-first RHI
Future D3D12 / Metal backend
Slang-first shader system
RenderGraph-managed frame
Bindless-ready material system
GPU-driven-ready renderer
Editor-ready runtime structure
```

---

# Current Status

```text
Stage 0  - Foundation + Engine Loop                     DONE
Stage 1  - SDL Platform Layer                            DONE
Stage 2A - Vulkan Dependencies + RHI Skeleton            DONE
Stage 2B-1 - Vulkan Instance / Surface / Device          DONE
Stage 2B-2 - Vulkan Swapchain / Clear / Present          DONE
Stage 3  - RenderSystem + Linear RenderGraph V0          DONE

Next:
Stage 4A - ShaderSystem + Slang Integration
Stage 4B - RHIShader / Pipeline / TrianglePass
```

---

# Major Architecture Principles

## Subsystem Ownership

```text
Engine owns SubsystemManager.
Subsystem creation order is registration order.
Subsystem destruction order is reverse registration order.
Logging is a static service, not a subsystem.
Time is an internal Engine service.
```

Current subsystem order:

```text
PlatformSystem
RHISystem
RenderSystem
```

Future subsystem order:

```text
FileSystem
PlatformSystem
InputSystem
JobSystem
AssetSystem
ShaderSystem
RHISystem
RenderSystem
SceneSystem
UISystem
EditorSystem
```

---

## Rendering Architecture Direction

Long-term renderer layering:

```text
RenderSystem
  -> RenderPipeline
      -> RenderFeature
          -> RenderPass
              -> RenderGraph
                  -> RHI
```

Meaning:

```text
RenderSystem:
  Owns high-level renderer lifecycle.

RenderPipeline:
  Assembles a full frame, such as ForwardPipeline, DeferredPipeline, EditorPipeline.

RenderFeature:
  Represents configurable renderer features, such as Bloom, FXAA, TAA, SSAO, Shadows.

RenderPass:
  Represents concrete GPU work, such as ClearPass, ForwardPass, TonemapPass.

RenderGraph:
  Records, compiles, and executes passes.

RHI:
  Executes backend-specific graphics commands.
```

Important distinction:

```text
RenderPass is the execution unit.
RenderFeature is the configurable feature module.
RenderPipeline is the frame assembly strategy.
RenderGraph is the execution and dependency system.
RenderSettings stores user/project/camera configuration.
```

---

# Stage 0 - Foundation + Engine Loop

## Status

```text
DONE
```

## Goal

Create a stable engine runtime skeleton before integrating rendering dependencies.

## Systems

```text
Core
Logging
Assert
Time
Subsystem
SubsystemManager
Engine
EngineConfig
Diagnostics placeholder
```

## Features

```text
Basic type aliases
Assertion macros
spdlog-based logging
Engine initialization
Engine shutdown
Main loop
Subsystem lifecycle management
Delta time calculation
```

## Completion Criteria

```text
SandboxApp starts.
Engine.Initialize() works.
Engine.Run() works.
Engine.Shutdown() works.
SubsystemManager creates subsystems in registration order.
SubsystemManager destroys subsystems in reverse registration order.
Logs show lifecycle events.
```

---

# Stage 1 - SDL Platform Layer

## Status

```text
DONE
```

## Goal

Add SDL3 as the platform backend while keeping SDL hidden inside private Platform implementation.

## Systems

```text
PlatformSystem
Window
SDLWindow
NativeWindowHandle
PlatformEvent queue
```

## Third-party Libraries

```text
SDL3
```

## Features

```text
Create native SDL window
Poll events
Handle close event
Track window size
Expose NativeWindowHandle without leaking SDL_Window
SDL3 built from ThirdParty/SDL
SDL3 dynamically linked by default
```

## Completion Criteria

```text
Sandbox opens a window.
Closing the window exits the engine loop.
SDL headers only appear in Platform/Private/SDL.
Public Platform headers do not expose SDL types.
```

---

# Stage 2A - Vulkan Dependencies + RHI Skeleton

## Status

```text
DONE
```

## Goal

Prepare Vulkan dependencies and create the first RHI skeleton.

## Dependency Policy

```text
Vulkan SDK:
  System SDK, detected by find_package(Vulkan REQUIRED) inside Engine/CMakeLists.txt.

volk:
  ThirdParty/volk, built with add_subdirectory.

VMA:
  ThirdParty/VulkanMemoryAllocator, privately included by XEngineRuntime.
```

## Systems

```text
RHI public API skeleton
RHISystem skeleton
Vulkan backend private skeleton
```

## Completion Criteria

```text
XENGINE_ENABLE_VULKAN=ON configures successfully.
Vulkan SDK is detected inside Engine/CMakeLists.txt.
volk is privately linked.
VMA is privately included.
No Vulkan / volk / VMA types appear in public headers.
```

---

# Stage 2B-1 - Vulkan Instance / Surface / Device / Allocator

## Status

```text
DONE
```

## Goal

Create the Vulkan backend up to logical device and allocator creation.

## Features

```text
volkInitialize()
VkInstance
VkDebugUtilsMessengerEXT
VkSurfaceKHR from SDL window
VkPhysicalDevice selection
VkDevice
Graphics queue
Present queue
VmaAllocator
```

## Completion Criteria

```text
SDL window opens.
Vulkan instance is created.
SDL Vulkan surface is created.
Physical device is selected.
Logical device is created.
VMA allocator is created.
Shutdown destroys Vulkan resources in correct order.
```

---

# Stage 2B-2 - Vulkan Swapchain / Command / Clear / Present

## Status

```text
DONE
```

## Goal

Create a Vulkan swapchain and clear it every frame.

## Features

```text
VulkanSwapchain
Swapchain image views
Command pool
Primary command buffer
ImageAvailable semaphore
RenderFinished semaphore
InFlight fence
vkAcquireNextImageKHR
vkCmdClearColorImage
vkQueueSubmit
vkQueuePresentKHR
Basic resize / out-of-date handling
```

## Important Choice

Stage 2B-2 uses:

```text
vkCmdClearColorImage
```

It does not use:

```text
Render pass
Framebuffer
Graphics pipeline
Shader
Triangle
```

## Completion Criteria

```text
Window clears to fixed color.
Window close exits cleanly.
Resize does not crash.
RenderDoc can capture the clear frame.
```

---

# Stage 3 - RenderSystem + Linear RenderGraph V0

## Status

```text
DONE
```

## Goal

Move frame rendering ownership from RHISystem into RenderSystem + RenderGraph.

## Systems

```text
RenderSystem
RenderGraph V0
RenderGraphBuilder placeholder
RenderGraphContext
ClearPass
PresentPass placeholder
RenderSettings initial version
```

## Current RenderGraph Scope

```text
Linear pass list
Insertion-order execution
Single-threaded execution
No resource dependency analysis
No automatic barriers
No resource aliasing
No async compute
```

## RenderGraph Pass Types

```cpp
enum class RenderGraphPassType
{
    Graphics,
    Compute,
    Transfer,
    Present,
    External
};
```

## Important Design Decision

Do not create a `NeuralPass` class.

Future neural rendering features should be represented as:

```text
Compute pass
External pass
```

Examples:

```text
NeuralDenoisePass       -> Compute / External
NeuralTextureDecodePass -> Compute
DLSS / FSR / XeSS       -> External
```

## Completion Criteria

```text
Engine registers PlatformSystem -> RHISystem -> RenderSystem.
RHISystem no longer directly clears every frame.
RenderSystem builds RenderGraph every frame.
ClearPass calls RHIDevice::ClearSwapchain.
PresentPass exists as placeholder.
Window still clears to fixed color.
No shaders, triangle, pipeline, or RenderFeature system yet.
```

---

# Stage 4A - ShaderSystem + Slang Integration

## Status

```text
NEXT
```

## Goal

Introduce Slang as the primary shader language and compile `.slang` files to SPIR-V.

## Systems

```text
ShaderSystem
ShaderCompiler
ShaderTypes
ShaderReflection
ShaderModule
SlangCompiler
```

## Third-party Libraries

```text
Slang
```

## Features

```text
Read .slang files
Compile vertex / fragment / compute entry points
Output SPIR-V bytecode
Create CompiledShader objects
Keep Slang types private
```

## Public API Direction

```cpp
enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

enum class ShaderTarget
{
    VulkanSPIRV,
    D3D12DXIL,
    MetalMSL
};
```

## Completion Criteria

```text
ShaderSystem can compile Triangle.slang to SPIR-V.
ShaderSystem public headers do not expose Slang.
No Vulkan pipeline creation yet.
No triangle draw yet.
```

---

# Stage 4B - RHIShader / Pipeline / TrianglePass

## Status

```text
PLANNED
```

## Goal

Draw the first triangle using Slang-compiled shaders.

## Systems

```text
RHIShader
RHIPipeline
RHICommandList minimal draw API
VulkanShader
VulkanPipeline
VulkanCommandList
TrianglePass
```

## Features

```text
Create VkShaderModule
Create minimal graphics pipeline
Use dynamic rendering if available
Use vertex ID triangle
No vertex buffer
No descriptor set
No material
No texture
```

## RenderGraph Flow

```text
ClearPass
TrianglePass
PresentPass
```

## Completion Criteria

```text
Triangle.slang compiles.
Vulkan shader modules are created.
Graphics pipeline is created.
TrianglePass draws a triangle.
No mesh, material, texture, scene, or asset system yet.
```

---

# Stage 5 - Basic Mesh Forward Renderer

## Status

```text
PLANNED
```

## Goal

Move from hardcoded triangle rendering to basic mesh rendering.

## Systems

```text
RHI buffer abstraction
Basic mesh representation
Camera data
RenderView
RenderScene initial version
ForwardOpaquePass
```

## Features

```text
Vertex buffer
Index buffer
Depth buffer
Depth test
Camera uniform data
Draw cube or hardcoded mesh
```

## GPU-driven Preparation

Start designing render data as IDs and indices:

```cpp
struct RenderObject
{
    u32 ObjectId = 0;
    u32 MeshId = 0;
    u32 MaterialId = 0;

    Mat4 World;
    Mat4 PreviousWorld;

    AABB Bounds;
};
```

## Completion Criteria

```text
A basic 3D mesh renders.
Depth testing works.
Renderer uses RenderScene / RenderObject direction, not direct immediate hardcoding only.
```

---

# Stage 6 - Material + Texture + Basic PBR

## Status

```text
PLANNED
```

## Goal

Introduce material data, texture sampling, and basic physically based shading.

## Systems

```text
TextureManager
MaterialSystem
Sampler
MaterialAsset initial version
GPU material data placeholder
```

## Third-party Libraries

```text
stb_image
nlohmann/json optional
```

## Features

```text
Texture loading
Sampler creation
Unlit textured shader
Basic PBR shader
BaseColor texture
Normal texture
Metallic/Roughness texture
AO texture optional
Directional light
```

## Bindless Preparation

Material data should be index-based where possible:

```cpp
struct GPUMaterialData
{
    u32 BaseColorTextureIndex;
    u32 NormalTextureIndex;
    u32 MetallicRoughnessTextureIndex;
    u32 AOTextureIndex;

    Vec4 BaseColorFactor;
    f32 MetallicFactor;
    f32 RoughnessFactor;
};
```

## Completion Criteria

```text
A mesh can render with texture.
A mesh can render with basic PBR material.
MaterialAsset does not hold Vulkan handles.
```

---

# Stage 6 Split

Stage 6 is split into:

```text
Stage 6A - Math V0 + RHI Texture / Sampler / Image Upload Foundation
Stage 6B - TextureManager + stb_image File Loading
Stage 6C - MaterialSystem + Material Data
Stage 6D - BindGroup V0 + Unlit Textured Mesh
Stage 6E - Basic PBR Material
```

## Stage 6A Decisions

```text
Stage 6A introduces Math V0.
Math V0 uses glm as the backend.
XEngine code should use XEngine Math types such as Vec2 / Vec3 / Vec4 / Mat4 / Quat.
Direct glm usage should be avoided outside the Math module where practical.
Future stages may replace aliases with fully owned XEngine math structs if needed.
```

```text
stb_image is used as the Stage 6 simple development image loader.
It is lightweight and easy to integrate.
It is not the final production texture pipeline.
Future production texture support should include KTX2 / Basis Universal / DDS / GPU compressed formats.
```

```text
RHIDevice currently acts as a resource creation facade.
This is acceptable in early stages.
Renderer controls what resources are created.
RHI backend controls how native GPU resources are created.
Resource usage should happen through RHICommandList.
Future stages should split RHIDevice into ResourceFactory / UploadManager / Swapchain / Queue responsibilities.
```

Do not introduce RenderFeature system in Stage 6.
RenderFeature V0 is planned for Stage 9 when HDR / Tonemap / Bloom / FXAA become configurable.

---

## Stage 6B - TextureManager + stb_image File Loading

```text
TextureManager is a lightweight renderer-side manager, not the full AssetSystem.
stb_image is used for simple development-time loading of PNG/JPG/TGA/HDR-style files where supported.
TextureManager loads RGBA8 CPU pixels and creates RHITexture through RHIDevice.
TextureManager owns default fallback textures.
Missing files return a missing texture fallback.
No MaterialSystem yet.
No texture sampling shader yet.
No BindGroup / descriptor V0 yet.
Future production texture pipeline should support KTX2 / Basis Universal / DDS / GPU compressed formats.
```

## Stage 6C - MaterialSystem + Material Data

```text
MaterialSystem is a lightweight renderer-side manager, not AssetSystem.
MaterialHandle is introduced.
MaterialDesc stores base color, metallic, roughness, alpha mode, and texture handles.
GPUMaterialData is introduced as a GPU-friendly / bindless-ready data structure.
MaterialSystem owns default lit, default unlit, and missing materials.
MaterialSystem resolves invalid texture handles to TextureManager fallback textures.
No BindGroup / descriptor set yet.
No texture sampling shader yet.
No PBR shader yet.
RenderFeature system remains planned for Stage 9.
```

## Stage 6D - BindGroup V0 + Unlit Textured Mesh

```text
Introduces RHIBindGroupLayout and RHIBindGroup.
Introduces Vulkan descriptor set layout / descriptor pool / descriptor set update.
Stage 6D only supports combined image sampler for base color texture.
MaterialSystem creates per-material base color bind groups.
UnlitTextured.slang samples base color texture.
ForwardOpaquePass / ForwardMeshPass binds pipeline and material bind group.
No PBR yet.
No bindless descriptors yet.
No RenderFeature system yet.
```

## Stage 6E - Basic PBR Material

```text
Adds ForwardPBR.slang.
Extends MaterialSystem with PBR material bind groups.
Supports base color / normal / metallic-roughness / AO texture slots.
Supports base color factor, metallic factor, and roughness factor.
Implements a basic direct-lighting metallic-roughness BRDF.
Uses a simple hardcoded directional light.
Uses push constants for Stage 6E material scalar factors.
Uses safe fallback textures for missing material slots.
Does not implement IBL.
Does not implement shadows.
Does not implement glTF import.
Does not implement RenderFeature system.
Does not implement bindless descriptors.
```

---

# Stage 7 - Asset System Foundation

## Status

```text
PLANNED
```

## Goal

Build the asset metadata/import pipeline in small steps, then connect real assets to renderer and scene systems.

## Stage 7 Split

```text
Stage 7A - Asset Core
Stage 7B - TextureAsset + Private ImageImporter
Stage 7C - MeshAsset + RenderMeshManager Bridge
Stage 7D - MaterialAsset + RenderMaterial Bridge
Stage 7E - glTF Importer V0
Stage 7F - Scene System V0 + RenderExtraction
Stage 7G - InputSystem V0 + Debug Camera
```

## Stage 7A - Asset Core

```text
AssetHandle
AssetType
AssetMetadata
AssetRegistry
AssetSystem subsystem
Public AssetImportTypes
Path-based metadata registration
Basic type guessing from source extensions
No real importers yet
No GPU resources
```

## Stage 7B - TextureAsset + Private ImageImporter

```text
TextureAsset CPU-side RGBA8 data
IAssetImporter private to Runtime/Asset
Public AssetSystem exposes ImportAsset(), not importer registration
Private ImporterRegistry
Extension-based importer dispatch
Private ImageImporter backed by stb_image
AssetSystem image import into TextureAsset
AssetSystem TextureAsset lookup by AssetHandle
Renderer TextureManager bridge from TextureAsset to RHITexture
Renderer image decoding deprecated
No glTF parsing yet
No MeshAsset import yet
No MaterialAsset import yet
No GPU resources inside AssetSystem
```

## Stage 7C - MeshAsset + RenderMeshManager Bridge

```text
MeshAsset CPU-side mesh data
MeshVertex
MeshSubmesh
Vertices, indices, submeshes, and bounds
AssetSystem CPU-side MeshAsset storage
Procedural cube MeshAsset validation
Renderer MeshHandle
Renderer-private RenderMeshManager
MeshAsset -> vertex/index RHIBuffer bridge
ForwardOpaquePass draws RenderMesh through RenderMeshManager
No glTF parsing yet
No MaterialAsset yet
No SceneAsset yet
No GPU resources inside AssetSystem
```

## Stage 7D - MaterialAsset + RenderMaterial Bridge

```text
MaterialAsset CPU-side material data
MaterialAsset stores base color, metallic, roughness, alpha mode, and TextureAsset references
MaterialAsset contains no RHI or Vulkan resources
AssetSystem CPU-side MaterialAsset storage
AssetSystem test MaterialAsset validation helper
Renderer MaterialSystem creates MaterialHandle from MaterialAsset
Renderer MaterialSystem resolves TextureAsset handles through AssetSystem / TextureManager
TextureManager caches textures created from AssetHandle references
ForwardOpaquePass draws RenderMesh with MaterialHandle
No glTF parsing yet
No SceneAsset yet
No GPU resources inside AssetSystem
```

## Stage 7E - glTF Importer V0

```text
Uses fastgltf 0.9
GltfImporter is private to Asset module
Public headers do not expose fastgltf
Imports .gltf and .glb files through AssetSystem::ImportAsset
Converts glTF meshes into MeshAsset
Converts glTF materials into MaterialAsset
Converts glTF images into TextureAsset where supported
Supports static meshes and basic metallic-roughness material data
Supports external images and GLB bufferView image data where decodable by stb_image
Does not implement SceneAsset yet
Does not implement animation, skinning, morph targets, or glTF extensions
Does not create GPU resources inside AssetSystem
Stage 7E is the final pure Asset import stage
Stage 7F will connect imported assets to Scene / RenderObject
```

## Stage 7F - Scene System V0 + RenderExtraction

```text
Introduces Scene module
Adds Entity handle
Adds TransformComponent
Adds MeshRendererComponent
Adds CameraComponent data only
Adds SceneSystem subsystem
Scene stores AssetHandle references, not renderer handles
Adds RenderScene and RenderObject in Renderer
Adds RenderExtraction boundary
RenderExtraction resolves MeshAsset into MeshHandle through RenderMeshManager
RenderExtraction resolves MaterialAsset into MaterialHandle through MaterialSystem
RenderMeshManager caches AssetHandle to MeshHandle mappings
MaterialSystem caches AssetHandle to MaterialHandle mappings
TextureManager keeps AssetHandle to TextureHandle caching
ForwardOpaquePass draws RenderScene.OpaqueObjects
RenderSystem creates a validation Scene entity from Cube or DamagedHelmet glTF assets
RenderSystem falls back to a procedural cube entity if glTF validation import is unavailable
Does not implement InputSystem
Does not implement DebugCamera control yet
Does not implement full ECS, scene serialization, animation, or skinning
Stage 7G will implement InputSystem V0 + DebugCamera in Scene
```

## Stage 7G - InputSystem V0 + Debug Camera

```text
Introduces Input module
Adds InputSystem subsystem
Adds engine-level KeyCode / MouseButton types
Platform events are translated into engine input events
InputSystem tracks current/previous key and mouse state
InputSystem tracks mouse position, mouse delta, and mouse wheel delta
Adds UE-style Scene DebugCameraController
RMB + mouse controls camera yaw/pitch
RMB + WASD/QE controls camera movement
Shift accelerates movement
Mouse wheel adjusts movement speed
SceneSystem owns a primary debug camera entity
RenderSystem uses the primary Scene camera
Debug camera can frame imported model bounds
Validation prefers DamagedHelmet, then Cube with texture, then procedural cube
DamagedHelmet should be viewable through auto-framing and interactive navigation
Does not implement full editor viewport focus, input rebinding, gamepad input, picking, or gizmos
```

## Asset / Renderer / RHI Boundary

```text
AssetSystem:
  Owns source paths, asset handles, metadata, private importer registry, and CPU-side asset data.

Renderer:
  Owns render resource managers, material systems, and GPU-facing render representations.
  Converts TextureAsset data into RHITexture objects.
  Converts MeshAsset data into vertex/index RHIBuffer objects.
  Converts MaterialAsset data into MaterialHandle / bind group backed renderer materials.

RHI:
  Owns backend GPU objects such as buffers, images, samplers, descriptor sets, and pipelines.
```

Correct future data flow:

```text
.gltf / .glb / .png / .jpg
  -> AssetSystem
  -> private importer
  -> TextureAsset / MeshAsset / MaterialAsset
  -> Renderer managers
  -> RHI resources
```

## fastgltf Decision

```text
fastgltf 0.9 is the selected glTF importer library.
It is placed under ThirdParty/fastgltf.
It should be used only inside Asset/Private/Importers/GltfImporter in Stage 7E.
Public Asset headers must not expose fastgltf types.
Renderer and RHI must not include fastgltf.
```

## Validation Assets

```text
Validation assets are placed under Assets/models/gltf:
- Cube with texture
- DamagedHelmet
```

## Completion Criteria

```text
Stage 7A creates AssetSystem and metadata registration without parsing glTF or creating GPU resources.
Later Stage 7 sub-stages add texture, mesh, material, glTF, and scene integration.
```

---

# Stage 8 - Lighting + Shadow

## Stage 8A - Renderer Architecture Stabilization

```text
COMPLETE
```

- Clarifies Renderer naming and distinguishes RenderPipeline from the RHI graphics pipeline.
- Adds RenderFrameContext and RenderResourceContext.
- Adds the RenderPipeline base class and ForwardRenderPipeline.
- Moves per-frame pass composition out of RenderSystem.
- Keeps one unified linear RenderGraph per frame.
- Adds RenderShaderLibrary for persistent RHIShader reuse.
- Adds RenderPipelineStateCache for persistent graphics pipeline reuse.
- Keeps RenderTextureManager, RenderMeshManager, and RenderMaterialSystem as Asset-to-GPU bridges.
- Keeps the existing RHIPipeline name to avoid a broad Vulkan/backend rename.
- Does not implement lights, shadows, RenderFeature, HDR, post-processing, or RHIPipelineCache.

Current frame flow:

```text
SceneSystem
  -> RenderExtraction
  -> RenderScene
  -> ForwardRenderPipeline
  -> RenderGraph
  -> ClearPass / ForwardOpaquePass / PresentPass
  -> RHI
```

Future stages:

```text
Stage 8B - LightComponent + RenderLight Extraction
Stage 8C - GPU Light Data + PBR Shader Integration
Stage 8D - Runtime Serialization + SceneSerializer V0 + Validation Scene Migration
```

## Stage 8C - Per-frame GPU Data + Shader Lighting Integration

- Adds `GPUFrameData`.
- Adds `GPULightingData`.
- Adds `RenderFrameResources`.
- Uses one per-frame buffer per renderer frame slot.
- Binds Set 0 per-frame data in `ForwardOpaquePass`.
- Keeps material textures in Set 1.
- Splits shader common code into Common / Lighting / BRDF / Material files.
- Makes Directional Light affect the PBR shader through extracted scene lighting.
- Does not implement shadows or IBL.

## Stage 8B-pre - Coordinate Convention Cleanup

Mid-term cleanup:
- Keeps GLM as the Math backend through XEngine aliases.
- Consolidates common operations behind XEngine Math helpers.
- Removes redundant GPU matrix/vector wrapper types and pure-copy packing.
- Allows shader-visible structs to use Mat4 and Vec4 with layout checks.
- Removes legacy image loading from Renderer; decoding stays in Asset private importers.
- Keeps CMake source discovery aligned with the current Renderer and Math files.

```text
COMPLETE
```

- Defines XEngine world coordinates as +X forward, +Y right, +Z up, left-handed.
- Centralizes coordinate axes and transform direction helpers in Math.
- Converts glTF position, normal, tangent, and triangle winding during import.
- Centralizes left-handed camera/view/projection matrix construction.
- Adds an RHI clip-space convention and a single Renderer projection adaptation point.
- Moves AABB transformation and combination into Math.
- Moves GPU matrix packing out of renderer passes.
- Updates DebugCamera to the XEngine world convention.
- Does not implement lights, shadows, GPU light buffers, or RenderFeature.

## Pre-Stage 8C - Transform Hierarchy + Rotator Cleanup

- Adds local/world transform separation.
- Adds degree-based Rotator.
- Defines Roll/Pitch/Yaw around +X/+Y/+Z.
- Adds Scene-managed parent-child hierarchy.
- Adds the Scene-private TransformSystem.
- Updates camera, light, and mesh extraction to read world transforms.
- Adds engine color presets.
- Moves common helper calls under XEngine::Math.

## Stage 8D - Runtime Serialization + SceneSerializer V0 + Validation Scene Migration

```text
COMPLETE
```

- Cleans `ThirdParty/json` down to the header-only include tree plus license files.
- Adds `ThirdParty_json` and the `XEngineSerialization` runtime module.
- Adds JSON load/save helpers, serialization context, and `.xscene` version metadata.
- Adds `SceneSerializer` in the Scene module for entities, transforms, cameras, lights,
  mesh renderers, and hierarchy.
- Moves validation scene content into `Assets/Scenes/*.xscene`.
- Updates Sandbox to load `Assets/Scenes/Default.xscene` at startup.
- Keeps RenderSystem focused on renderer resources, scene extraction, and frame rendering only.
- Does not implement ImGui, editor panels, shadows, CSM, or scene save UI.

## Stage 8E-1 - ThirdParty ImGui Cleanup + Editor ImGui Foundation

```text
COMPLETE
```

- Cleans `ThirdParty/imgui` to core files, SDL3 backend, Vulkan backend, stdlib helper, and license.
- Adds `ThirdParty_imgui` as an editor-only static library target.
- Adds `EditorApplication`, `EditorContext`, and a real `EditorSystem` skeleton.
- Adds editor-private `ImGuiLayer` and `ImGuiVulkanBackend`.
- Enables docking without multi-viewport.
- Adds a temporary `XEngine Editor Debug` validation window.
- Adds a generic renderer overlay callback and a Vulkan-native RHI bridge for editor overlay rendering.
- Keeps ImGui out of Runtime public headers, Sandbox, Scene, Asset, Serialization, Renderer implementation, RHI implementation, and Shader modules.
- Does not implement SceneHierarchyPanel, InspectorPanel, RendererDebugPanel, ViewportPanel, editor camera movement, gizmos, DebugDraw, shadows, CSM, asset browser, undo/redo, prefab, project system, or native file dialogs.

## Stage 8E-2 - EditorCamera, ViewportPanel, Mouse Capture, and Axis Gizmo

```text
COMPLETE
```

- Adds editor-only `EditorCamera`.
- Adds renderer-neutral `RenderView` and `RenderSystem::SetViewProvider`.
- Editor chooses between EditorCamera and active Scene primary CameraComponent through `UseEditorCamera`.
- Adds viewport input state to `EditorContext`.
- Adds reusable editor free camera control for mouse look, WASD, Q/E, and Shift speed.
- Adds Platform window APIs for cursor visibility, relative mouse mode, and focus state.
- Adds `ViewportPanel` with hover/focus tracking, capture hints, and camera capture entry.
- Releases camera capture on Esc, focus loss, or disabling Editor Camera.
- Adds a screen-space viewport axis gizmo for +X Forward, +Y Right, +Z Up.
- Keeps Sandbox on Scene primary CameraComponent and out of Editor/ImGui.
- Does not implement SceneHierarchyPanel, InspectorPanel, RendererDebugPanel, Viewport render target, DebugDraw, shadows, CSM, picking, manipulation gizmos, asset browser, undo/redo, prefab, project system, or native file dialogs.

## Stage 8E-3 - Editor Panels, Scene Load/Save UI, and Renderer Debug Settings

```text
COMPLETE
```

- Adds editor-private `MainMenuBar`, `SceneHierarchyPanel`, `InspectorPanel`, and `RendererDebugPanel`.
- Adds fixed-path New/Open/Save/Save As scene workflows through Runtime `SceneSerializer`.
- Tracks selection, panel visibility, and scene dirty state in `EditorContext`.
- Adds `Scene::Clear()` and keeps hierarchy traversal behind Scene query APIs.
- Inspector edits local transform, light, camera, and mesh renderer state without touching editor camera data.
- Adds runtime `RendererDebugSettings` and exposes it through `RenderSystem`.
- Adds a main editor dockspace without layout persistence or multi-viewport platform windows.
- Keeps Sandbox runtime-only: load `.xscene`, use Scene primary camera, no Editor/ImGui link, no save path.
- Does not implement CSM, shadow maps, DebugDraw, picking, manipulation gizmos, asset browser, material editor, undo/redo, prefab, project system, or native file dialogs.

## Status

```text
PLANNED
```

## Goal

Add basic real-time lighting and shadows.

## Systems

```text
Light system
Shadow pass
Shadow map resources
RenderGraph texture read/write
```

## Features

```text
Directional light
Point light optional
ShadowMapPass
Directional shadow map
PCF filtering
Cascaded shadow maps later
```

## RenderPipeline Preparation

At this stage, start preparing the idea of:

```text
ForwardPipeline
```

But do not introduce a full RenderFeature system yet unless necessary.

## Completion Criteria

```text
Directional light affects PBR shading.
A shadow map is rendered.
ForwardPBRPass reads shadow map.
RenderGraph tracks shadow pass dependency.
```

---

# Stage 9 - HDR + Post-processing + RenderFeature V0

## Status

```text
PLANNED
```

## Goal

Introduce HDR rendering, post-processing, and the first real RenderFeature system.

This is the recommended stage to formally introduce:

```text
RenderPipeline
RenderFeature
RenderSettings expansion
```

because this is the first stage where multiple configurable rendering features become meaningful.

## Systems

```text
ForwardPipeline
RenderFeature base concept
RenderSettings expanded
PostProcess system
TonemapPass
Bloom passes
FXAAPass optional
```

## Why RenderFeature Starts Here

Stage 9 introduces several configurable features:

```text
Bloom on/off
Tone mapping mode
FXAA on/off
Exposure settings
Color grading optional
```

This is the right time to move away from hardcoding pass order directly in RenderSystem.

## Recommended Layering

```text
RenderSystem
  -> ForwardPipeline
      -> RenderFeature
          -> RenderPass
              -> RenderGraph
```

## Initial RenderFeature Candidates

```text
BloomFeature
TonemapFeature
AntiAliasingFeature V0
```

## AntiAliasing V0

Start small:

```cpp
enum class AntiAliasingMode
{
    None,
    FXAA
};
```

Do not implement TAA yet.

## Features

```text
HDR color target
Tonemapping
Gamma correction
Bloom downsample
Bloom upsample
FXAA optional
Color grading optional
```

## Completion Criteria

```text
Scene renders into HDRColor.
TonemapPass writes LDR output.
Bloom can be toggled.
FXAA can be toggled if implemented.
RenderFeature V0 can add one or more passes to RenderGraph.
RenderSystem delegates frame assembly to ForwardPipeline.
```

---

# Stage 10 - GPUScene + RenderQueue

## Status

```text
PLANNED
```

## Goal

Refactor renderer data into GPU-friendly buffers and prepare for GPU-driven rendering.

## Systems

```text
GPUScene
RenderQueue
DrawList
ObjectBuffer
MaterialBuffer
MeshBuffer
LightBuffer
```

## Features

```text
GPUObjectData
GPUMaterialData
GPUMeshData
Opaque render queue
Transparent render queue placeholder
CPU frustum culling
Draw sorting
```

## GPU-driven Preparation

This is the real foundation for GPU-driven rendering.

The renderer should move toward:

```text
ObjectBuffer
MaterialBuffer
MeshBuffer
DrawList
```

instead of immediate per-object CPU binding.

## Completion Criteria

```text
Objects are uploaded into structured GPU buffers.
Render queue controls draw order.
Foundation for indirect drawing exists.
```

---

# Stage 11 - Bindless Resource Model

## Status

```text
PLANNED
```

## Goal

Introduce a modern resource binding model.

## Systems

```text
BindlessResourceManager
Texture heap
Sampler heap
Material texture indices
```

## Features

```text
Global texture array
Global sampler table
Material stores texture indices
Descriptor indexing
Non-uniform indexing
```

## Why This Matters

Bindless is a major prerequisite for scalable GPU-driven rendering.

GPU-driven draw should be able to resolve:

```text
object id -> material id -> texture indices
```

without CPU-side per-material descriptor rebinding.

## Completion Criteria

```text
Material no longer requires per-material descriptor set binding.
Textures are accessed by index.
Bindless manager owns descriptor allocation/update.
```

---

# Stage 12 - Forward+ / Clustered Lighting

## Status

```text
PLANNED
```

## Goal

Support many dynamic lights efficiently and mature compute-pass infrastructure.

## Systems

```text
LightCullingPass
Cluster data
Tile light list
Compute pipeline
```

## Features

```text
Depth prepass
Light culling compute shader
Forward+ tile lighting
Clustered lighting optional
Many point lights
```

## GPU-driven Preparation

This stage trains the renderer for:

```text
Compute pass
Storage buffers
Compute-to-graphics dependency
GPU-generated lists
```

These are also required by later GPU-driven culling.

## Completion Criteria

```text
Many lights render with reasonable performance.
Light culling runs as a compute pass.
ForwardPBRPass reads light lists.
RenderGraph handles compute-to-graphics dependency.
```

---

# Stage 13 - Editor Base

## Status

```text
PLANNED
```

## Goal

Build the first editor interface.

## Systems

```text
UISystem
ImGuiRenderer
EditorSystem
ViewportPanel
SceneHierarchyPanel
InspectorPanel
AssetBrowserPanel
ProfilerPanel
RenderGraphPanel
```

## Third-party Libraries

```text
Dear ImGui docking branch
```

## Features

```text
DockSpace
Viewport panel
Scene hierarchy placeholder
Inspector placeholder
Asset browser placeholder
Profiler panel placeholder
RenderGraph panel placeholder
```

## Completion Criteria

```text
EditorApp launches with dockable UI.
SandboxApp does not depend on Editor.
ImGui does not appear in public runtime headers.
Viewport can display renderer output.
```

---

# Stage 14 - Temporal Renderer

## Status

```text
PLANNED
```

## Goal

Introduce temporal rendering infrastructure.

## Systems

```text
Frame history
Motion vector pass
History resource manager
Temporal resolve pass
TAAFeature
```

## Features

```text
Previous view-projection matrix
Previous object transform
Jittered projection
Motion vector buffer
TAA
History color
History depth
Temporal accumulation
```

## AntiAliasing Expansion

Expand:

```cpp
enum class AntiAliasingMode
{
    None,
    FXAA,
    TAA
};
```

## Completion Criteria

```text
Renderer stores current and previous frame data.
Motion vectors render correctly.
TAA improves image stability.
History resources are managed safely.
TAA is implemented as a RenderFeature.
```

---

# Stage 15 - Advanced Renderer Experiments

## Status

```text
PLANNED
```

## Goal

Explore modern and experimental rendering techniques after the renderer architecture is stable.

## Candidate Features

```text
SSAO / GTAO
SSR
Deferred rendering
Visibility buffer
GPU-driven rendering
GPU frustum culling
GPU occlusion culling
Indirect draw
Meshlet rendering
Mesh shader path
Ray traced shadows
Ray traced reflections
Simple path tracing mode
Virtual shadow maps
Virtual texturing
External upscalers
Neural rendering experiments
```

---

## Stage 15A - GPU-driven V0: CPU-generated Indirect Draw

```text
CPU builds VkDrawIndexedIndirectCommand buffer.
GPU executes indirect draw.
No GPU culling yet.
```

## Stage 15B - GPU-driven V1: GPU Frustum Culling

```text
Compute shader tests object bounds.
Visible objects write indirect draw commands.
Graphics pass executes indirect draw.
```

## Stage 15C - GPU-driven V2: GPU Draw Count

```text
Atomic counter / count buffer.
vkCmdDrawIndexedIndirectCount.
```

## Stage 15D - GPU-driven V3: Hi-Z Occlusion Culling

```text
Build depth pyramid.
Compute shader tests object bounds against Hi-Z.
Cull hidden objects.
```

## Stage 15E - Meshlet / Cluster Culling

```text
meshoptimizer meshlets.
Cluster bounds.
Cluster cone culling.
Compute fallback first.
Mesh shader path later.
```

## Stage 15F - External / Neural Features

Use `RenderGraphPassType::External` or `RenderGraphPassType::Compute`.

Candidate features:

```text
DLSS
FSR
XeSS
Neural denoising
Neural texture decoding
Neural material approximation
Neural radiance cache experiments
```

Do not introduce these before the renderer has:

```text
Motion vectors
Depth
History resources
RenderFeature system
External pass support
Stable RenderGraph resources
```

---

# Multithreading Plan

## Stage 0-3

```text
No real multithreading.
RenderGraph execution is single-threaded.
```

## Stage 4.5 - JobSystem V0

Introduce after ShaderSystem is working.

```text
Worker thread pool
Submit()
Wait()
ParallelFor()
Used by shader compilation later
```

Do not implement work stealing or complex task graphs yet.

## Stage 7.5 - Async Asset Loading V0

```text
Texture decoding
glTF parsing
Mesh optimization
CPU-side asset import on workers
GPU resource creation stays on render/RHI thread
```

## Stage 10.5 - Parallel Render Preparation

```text
Parallel scene extraction
Parallel CPU frustum culling
Parallel draw list building
```

## Stage 12.5+ - RenderGraph Task Scheduling

```text
Pass preparation tasks
Parallel command recording
Async compute groundwork
```

---

# RenderFeature System Plan

The RenderFeature system should **not** be implemented too early.

## Not in Stage 3

Stage 3 only contains:

```text
RenderSystem
RenderGraph V0
ClearPass
PresentPass
RenderSettings initial clear color
```

Do not implement:

```text
BloomFeature
AntiAliasingFeature
ShadowFeature
SSAOFeature
```

## Formal Introduction

RenderFeature should be formally introduced in:

```text
Stage 9 - HDR + Post-processing + RenderFeature V0
```

Reason:

```text
Stage 9 is the first time XEngine has multiple configurable rendering features:
- Bloom
- Tonemap
- FXAA
- Exposure
- Color grading
```

## Future Expansion

```text
Stage 14:
  TAAFeature

Stage 15:
  External upscalers
  Neural rendering experiments
  GPU-driven features
```

---

# Recommended Development Sequence From Now

```text
1. Stage 4A - ShaderSystem + Slang Integration
2. Stage 4B - RHIShader / Pipeline / TrianglePass
3. Stage 4.5 - JobSystem V0
4. Stage 5 - Basic Mesh Forward Renderer
5. Stage 6 - Material + Texture + Basic PBR
6. Stage 7 - Asset System Foundation
7. Stage 7.5 - Async Asset Loading V0
8. Stage 8 - Lighting + Shadow
9. Stage 9 - HDR + Post-processing + RenderFeature V0
10. Stage 10 - GPUScene + RenderQueue
11. Stage 11 - Bindless Resource Model
12. Stage 12 - Forward+ / Clustered Lighting
13. Stage 13 - Editor Base
14. Stage 14 - Temporal Renderer
15. Stage 15 - Advanced Renderer Experiments
```

---

# Immediate Next Step

The next practical stage is:

```text
Stage 4A - ShaderSystem + Slang Integration
```

Primary goal:

```text
Compile Triangle.slang to SPIR-V through XEngine ShaderSystem.
```

Do not create Vulkan pipeline yet in Stage 4A.

Stage 4B will handle:

```text
VkShaderModule
Graphics pipeline
TrianglePass
Draw triangle
```
