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
