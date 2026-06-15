# XEngine

XEngine is a renderer-first learning engine.

V0.1 focuses on:
- Clean engine architecture
- C++20
- Public/Private module layout
- SDL3 platform backend
- RHI abstraction
- Vulkan-first backend placeholder
- Slang-first shader system placeholder
- RenderGraph-ready Renderer structure
- Scene / Asset / Editor placeholders

Stage 1 adds:
- SDL3 source vendored under ThirdParty/SDL
- SDL3 built together with XEngine
- SDL3 dynamically linked by default
- SDL3 runtime copied beside app executables on Windows
- SDL3 platform backend
- Main window creation
- Event polling
- Window close handling
- SubsystemContext

SDL is hidden inside Platform/Private/SDL.
Public headers do not expose SDL types.

Stage 2A adds:
- Vulkan SDK detection through find_package(Vulkan)
- volk source integration from ThirdParty/volk
- VMA header integration from ThirdParty/VulkanMemoryAllocator
- RHI public API skeleton
- RHISystem skeleton
- Vulkan backend skeleton

Stage 2A does not create a Vulkan instance yet.
Stage 2A does not create a swapchain yet.
Stage 2A does not render or clear the screen yet.
Actual Vulkan initialization starts in Stage 2B.

Stage 2B-1 adds:
- Minimal PlatformEvent queue
- Window resize events
- Vulkan loader initialization through volk
- Vulkan instance creation
- SDL Vulkan surface creation
- Physical device selection
- Logical device creation
- Graphics / present queue discovery
- VMA allocator creation

Stage 2B-1 does not create a swapchain.
Stage 2B-1 does not render.
Stage 2B-1 does not clear the screen.
Swapchain and clear screen are planned for Stage 2B-2.

Stage 4 - ShaderSystem + Slang + Triangle adds:
- ShaderSystem subsystem
- Online shader compilation
- XENGINE_ENABLE_SHADER_COMPILER option
- Slang integration
- ShaderStage / ShaderTarget / ShaderCodeFormat public types
- CompiledShader public structure
- ShaderReflection placeholder
- Triangle.slang sample shader
- SPIR-V compilation validation
- RHIShader
- RHIPipeline
- VulkanShader
- VulkanPipeline
- RHICommandList minimal draw API
- TrianglePass
- First triangle rendering

Stage 4 uses a private slangc fallback because the local Slang source snapshot does not provide a complete buildable CMake dependency set.
Future release/runtime builds may disable online shader compilation and load precompiled shader outputs instead.

Stage 5 - Engine CMake Modularization + Basic Mesh Forward Renderer adds:
- Modular Engine CMake targets
- Foundation / Engine / Platform / Shader / RHI / Renderer module CMake files
- RHI buffer abstraction for vertex and index data
- Vulkan CPU-visible buffer allocation through VMA
- Depth texture creation and dynamic rendering depth attachment
- Vertex input layout support
- Index buffer binding and indexed draw commands
- Push constants for per-object MVP data
- Hardcoded indexed cube mesh
- MeshForward.slang
- ForwardMeshPass
- Default RenderGraph order: ClearPass -> ForwardMeshPass -> PresentPass

Stage 5 does not implement glTF loading, material systems, texture sampling, PBR, ECS, ImGui, RenderFeature, GPU-driven rendering, or descriptor-based per-material resources.

Stage 6A adds:
- Math V0 using glm backend
- RHITexture abstraction
- RHISampler abstraction
- VulkanTexture
- VulkanSampler
- Initial CPU pixel data upload to GPU texture
- Default white texture validation
- Default normal texture validation
- stb_image dependency prepared for Stage 6B

Stage 6A does not implement full TextureManager.
Stage 6A does not implement MaterialSystem.
Stage 6A does not sample textures in shaders yet.
Stage 6A does not implement PBR.
Stage 6A does not implement bindless descriptors.

Stage 6B adds:
- TextureManager
- TextureHandle
- stb_image-based private image loading
- RGBA8 image loading
- RHITexture creation from loaded image data
- Default fallback textures
- Optional checker texture load from Assets/Textures/checker.png

Stage 6B does not implement MaterialSystem.
Stage 6B does not bind textures to shaders.
Stage 6B does not implement PBR.
Stage 6B does not implement AssetSystem.
Stage 6B does not implement texture streaming.

Stage 6C adds:
- MaterialHandle
- MaterialDesc
- GPUMaterialData placeholder
- MaterialSystem
- Default lit material
- Default unlit material
- Missing material
- Texture fallback resolution through TextureManager

Stage 6C does not implement descriptor sets.
Stage 6C does not bind material textures to shaders.
Stage 6C does not implement PBR.
Stage 6C does not implement AssetSystem.
Stage 6C does not implement RenderFeature system.

Stage 6D adds:
- RHIBindGroupLayout
- RHIBindGroup
- Vulkan descriptor set layout
- Vulkan descriptor pool
- Vulkan descriptor set update
- Combined image sampler binding
- UnlitTextured shader
- Material base color bind group
- Textured mesh rendering

Stage 6D does not implement PBR.
Stage 6D does not implement bindless descriptors.
Stage 6D does not implement AssetSystem.
Stage 6D does not implement RenderFeature system.

Stage 6E adds:
- ForwardPBR.slang
- Basic metallic-roughness PBR shader
- PBR material bind group
- Base color texture support
- Normal texture slot with safe fallback
- Metallic-roughness texture slot with safe fallback
- AO texture slot with safe fallback
- Base color factor, metallic factor, and roughness factor
- Simple directional light
- Lit material rendering through ForwardOpaquePass

Stage 6E does not implement IBL.
Stage 6E does not implement shadows.
Stage 6E does not implement glTF import.
Stage 6E does not implement bindless descriptors.
Stage 6E does not implement RenderFeature system.

Stage 7A adds:
- Runtime Asset module
- AssetSystem subsystem
- AssetHandle
- AssetType / AssetLoadState
- AssetMetadata
- AssetRegistry
- AssetImportContext / AssetImportResult
- Source asset metadata registration
- Path lookup and basic extension-based type guessing
- fastgltf 0.9 source validation for future glTF importer work
- glTF validation asset documentation under Assets/Models/gltf

Stage 7A does not parse glTF yet.
Stage 7A does not create GPU resources.
Stage 7A does not implement AssetDatabase persistence.
Stage 7A does not implement async loading.

Stage 7B adds:
- TextureAsset CPU-side RGBA8 texture data
- Public AssetImportTypes without exposing importer implementations
- Private IAssetImporter under Runtime/Asset
- Private ImporterRegistry with extension-based importer dispatch
- Private ImageImporter using stb_image inside Asset implementation
- AssetSystem image import into TextureAsset
- AssetSystem TextureAsset lookup by AssetHandle
- Renderer TextureManager bridge from TextureAsset to RHITexture
- Optional checker texture import through AssetSystem

IAssetImporter is private to the Asset module.
stb_image is private to Asset importer implementation.
Renderer does not own image decoding; the old renderer ImageLoader compatibility files have been removed.

Stage 7B does not parse glTF yet.
Stage 7B does not implement MeshAsset import.
Stage 7B does not implement MaterialAsset import.
Stage 7B does not create GPU resources inside AssetSystem.

Stage 7C adds:
- MeshAsset CPU-side mesh data
- MeshVertex
- MeshSubmesh
- Mesh bounds
- CPU-side MeshAsset storage in AssetSystem
- Procedural cube MeshAsset validation
- MeshHandle renderer-side runtime handle
- RenderMeshManager
- MeshAsset to RHIBuffer bridge
- ForwardOpaquePass drawing through RenderMeshManager

Stage 7C does not parse glTF yet.
Stage 7C does not implement MaterialAsset.
Stage 7C does not implement SceneAsset.
Stage 7C does not create GPU resources inside AssetSystem.

Stage 7D adds:
- MaterialAsset CPU-side material data
- CPU-side MaterialAsset storage in AssetSystem
- Test MaterialAsset validation helper
- MaterialAsset to renderer MaterialHandle bridge
- TextureAsset handle resolution through AssetSystem and TextureManager
- TextureManager AssetHandle texture cache
- ForwardOpaquePass using MeshHandle plus MaterialHandle

Stage 7D does not parse glTF yet.
Stage 7D does not implement SceneAsset.
Stage 7D does not create GPU resources inside AssetSystem.
Stage 7D does not implement bindless descriptors.

Stage 7E adds:
- Private GltfImporter
- fastgltf 0.9 integration
- .gltf / .glb import through AssetSystem
- glTF mesh to MeshAsset conversion
- glTF material to MaterialAsset conversion
- glTF image to TextureAsset conversion where supported
- Validation using Assets/models/gltf
- Optional renderer smoke path using the Cube glTF asset with procedural fallback

Stage 7E does not implement SceneAsset.
Stage 7E does not implement animation.
Stage 7E does not implement skinning.
Stage 7E does not create GPU resources inside AssetSystem.

Stage 7F adds:
- Scene module
- SceneSystem subsystem
- Entity handles
- TransformComponent
- MeshRendererComponent with MeshAsset / MaterialAsset handles
- CameraComponent data
- RenderScene
- RenderObject
- RenderExtraction boundary
- AssetHandle to MeshHandle / MaterialHandle caches in renderer managers
- ForwardOpaquePass drawing RenderScene opaque objects
- Validation Scene entity created from glTF assets with procedural cube fallback

Stage 7F does not implement InputSystem.
Stage 7F does not implement interactive DebugCamera.
Stage 7F does not implement full ECS.
Stage 7F does not implement scene serialization.
Stage 7F does not implement animation or skinning.

Stage 7G adds:
- InputSystem V0
- Engine-level KeyCode / MouseButton input types
- Platform event to input event translation
- Current and previous keyboard state tracking
- Current and previous mouse button state tracking
- Mouse position, mouse delta, and mouse wheel tracking
- Scene DebugCameraController
- UE-style RMB + mouse yaw/pitch navigation
- RMB + WASD/QE camera movement
- Shift accelerated movement
- Mouse wheel movement speed adjustment
- Active Scene camera used by renderer
- Automatic camera framing for imported model bounds
- DamagedHelmet-first visual validation with Cube and procedural fallbacks

Stage 7G does not implement full editor viewport focus.
Stage 7G does not implement input rebinding.
Stage 7G does not implement gamepad input.
Stage 7G does not implement scene picking or gizmos.

Stage 8A stabilizes renderer architecture.

Current renderer frame flow:

```text
SceneSystem
  -> RenderExtraction
  -> RenderScene
  -> ForwardRenderPipeline
  -> RenderGraph
  -> Passes
  -> RHI
```

- RenderSystem is a high-level coordinator.
- ForwardRenderPipeline owns frame composition and one linear per-frame RenderGraph.
- RenderShaderLibrary owns and reuses RHIShader objects.
- RenderPipelineStateCache owns and reuses graphics RHIPipeline objects.
- RenderTextureManager, RenderMeshManager, and RenderMaterialSystem bridge Asset data to GPU resources.
- ForwardOpaquePass requests graphics pipelines through RenderPipelineStateCache.

The current RHI type is still named `RHIPipeline`; Stage 8A treats it as the graphics-pipeline
type and avoids a broad backend rename.

Stage 8A does not add lighting, shadows, HDR, post-processing, RenderFeature, or RHIPipelineCache.

Stage 8B-pre establishes the XEngine world coordinate convention:

```text
+X = Forward
+Y = Right
+Z = Up
Left-handed world space
```

External glTF positions, directions, tangents, and triangle winding are converted into XEngine
coordinates once during import. Scene and Renderer use XEngine coordinates internally. Graphics
API clip-space differences are adapted through the RHI projection convention boundary.

Common AABB, camera matrix, coordinate conversion, and transform-axis helpers are centralized.
Renderer passes and scene extraction no longer define general-purpose matrix or bounds utilities.

Stage 8B-pre does not implement lights, shadows, GPU light buffers, or RenderFeature.

Mid-term cleanup keeps GLM as the backend implementation of XEngine Math. Engine-facing code uses
the `XEngine::Vec2`, `Vec3`, `Vec4`, `Mat4`, and `Quat` aliases from `MathTypes.h`, while common
operations are exposed through XEngine Math helpers instead of scattered direct GLM calls.

Shader-visible renderer structs may use `Mat4` and `Vec4` directly when their layout is validated
with compile-time checks. Redundant `Matrix4`, `Vector4`, and pure-copy GPU matrix packing helpers
have been removed. Image decoding remains private to Asset importers; RenderSystem only coordinates
asset-backed render resource managers.

Pre-Stage 8C establishes the transform hierarchy foundation:

- TransformComponent stores local TRS and cached world TRS/matrices.
- User-facing Euler rotations use degree-based `Math::Rotator`.
- Roll, Pitch, and Yaw rotate around XEngine +X, +Y, and +Z respectively.
- Scene owns parent and direct-child relationships and rejects hierarchy cycles.
- The Scene-private TransformSystem recursively updates roots and descendants every frame.
- Camera, light, and mesh extraction consume cached world transforms only.
- Common runtime math operations live under `XEngine::Math`.
- `XEngine::Colors` provides reusable engine color presets.

Stage 8C adds per-frame GPU data and shader lighting integration.

Renderer now uploads `GPUFrameData` once per frame:

- Camera matrices and camera world position.
- Ambient lighting and extracted scene lights.
- One uniform buffer and bind group per renderer frame slot.

`ForwardPBR.slang` reads camera and lighting data from set 0 instead of hardcoded shader values.
Material textures remain in set 1, while object data stays in push constants. Shader code is split
into reusable Common, Lighting, BRDF, Material, and Pass files. Stage 8C evaluates directional
lights first and deliberately leaves shadows, IBL, HDR, bloom, and clustered lighting for later.

Stage 2B-2 adds:
- Vulkan swapchain creation
- Swapchain image views
- One-frame command resources
- `vkCmdClearColorImage`-based clear
- Queue submit
- Present
- Basic resize / out-of-date handling

Stage 2B-2 still does not include Renderer or RenderGraph.
Clear screen is temporarily handled by RHISystem / RHIDevice for backend validation.
Stage 3 will move frame execution into RenderGraph.

Stage 3 adds:
- RenderSystem subsystem
- Linear RenderGraph V0
- ClearPass
- PresentPass placeholder
- Pass type categories: Graphics / Compute / Transfer / Present / External
- RHISystem no longer directly clears every frame

Stage 3 does not implement shaders, triangle rendering, RenderGraph resource dependencies, async compute, neural rendering, or GPU-driven rendering.
Future neural rendering features should be represented as Compute or External passes.

V0.1 does not implement:
- Full Vulkan renderer
- D3D12
- Metal
- Physics
- Audio
- Scripting
- Networking

## Build

```bash
cmake --preset default
cmake --build --preset default
```

The default scaffold is designed to compile without vendored third-party dependencies.
