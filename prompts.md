# XEngine Stage 5 Prompt - Engine CMake Modularization + Basic Mesh Forward Renderer

## Role

You are a senior C++ game engine architecture assistant.

You are working inside an existing C++20 project named **XEngine**.

Previous stages are complete:

```text
Stage 0 - Foundation + Engine Loop
- Core types
- Logging
- Assert
- Time
- SubsystemManager
- Engine lifecycle

Stage 1 - SDL Platform Layer
- SDL3 platform backend
- Window abstraction
- SDLWindow
- PlatformSystem
- Window close / resize events
- Minimal PlatformEvent queue

Stage 2 - Vulkan RHI Clear Screen
- Vulkan SDK detection
- volk
- VMA
- Vulkan instance
- Debug messenger
- SDL Vulkan surface
- Physical device selection
- Logical device
- Graphics / present queues
- VMA allocator
- Swapchain
- Command buffer
- Sync objects
- vkCmdClearColorImage
- Present
- Basic resize handling

Stage 3 - RenderSystem + Linear RenderGraph V0
- RenderSystem
- Linear RenderGraph V0
- RenderGraphContext
- RenderGraphBuilder placeholder
- ClearPass
- PresentPass placeholder
- RenderGraphPassType:
  - Graphics
  - Compute
  - Transfer
  - Present
  - External

Stage 4 - ShaderSystem + Slang + Triangle
- ShaderSystem
- Slang online compilation
- Triangle.slang
- RHIShader
- RHIPipeline
- VulkanShader
- VulkanPipeline
- RHICommandList minimal draw API
- TrianglePass
- First triangle rendering
```

Your task is to implement **Stage 5: Engine CMake Modularization + Basic Mesh Forward Renderer**.

This stage has two major parts:

```text
Part A:
  Refactor Engine CMake into clean module-level CMakeLists.txt files.

Part B:
  Move from hardcoded TrianglePass rendering to a basic indexed mesh forward renderer.
```

Do not implement glTF loading, AssetSystem, MaterialSystem, texture sampling, PBR, ECS Scene, ImGui, RenderFeature system, GPU-driven rendering, bindless resources, post-processing, async loading, or JobSystem yet.

---

# Stage 5 Goal

After this stage:

```text
Engine CMake is modular and maintainable.
Apps still link only XEngineRuntime.
XEngine can create GPU buffers.
XEngine can create a hardcoded indexed mesh.
XEngine can render a basic 3D mesh.
XEngine has a minimal RenderScene / RenderObject direction.
XEngine has a fixed camera / MVP path.
XEngine has a depth buffer.
XEngine uses ForwardMeshPass as the default render path.
README is cleaned up and completed sub-stages are merged into parent stages.
```

The final rendering path should be:

```text
RenderSystem::Render()
  -> RHIDevice::BeginFrame()
  -> RenderGraph
      -> ClearPass
      -> ForwardMeshPass
      -> PresentPass
  -> RHIDevice::EndFrame()
```

`TrianglePass` can remain in the codebase as a debug/example pass, but the default Sandbox rendering path should use `ForwardMeshPass`.

---

# Important Architecture Decisions

Follow these strictly:

```text
1. Stage 5 introduces basic mesh rendering, not full asset rendering.
2. The mesh can be hardcoded in code.
3. No glTF loading yet.
4. No AssetSystem yet.
5. No MaterialSystem yet.
6. No texture loading or texture sampling yet.
7. No PBR yet.
8. No ECS Scene yet.
9. No RenderFeature system yet.
10. No GPU-driven rendering yet.
11. No bindless resource model yet.
12. Use CPU-created vertex/index buffers.
13. Use indexed drawing.
14. Use a depth buffer.
15. Use push constants for MVP.
16. Keep RenderGraph V0 linear and single-threaded.
17. Do not implement resource dependency analysis in RenderGraph yet.
18. Public RHI / Renderer headers must not expose Vulkan, volk, VMA, SDL, or Slang.
19. Vulkan remains private to Runtime/RHI/Private/Vulkan.
20. Slang remains private to Runtime/Shader/Private/Slang.
```

---

# Part A - Engine CMake Modularization

## Problem

`Engine/CMakeLists.txt` has become too large.

It likely contains many source files and dependencies from:

```text
Foundation
Runtime/Engine
Runtime/Platform
Runtime/Shader
Runtime/RHI
Runtime/Renderer
Vulkan backend
SDL backend
Slang integration
```

This is not maintainable.

Stage 5 should split Engine CMake into module-level CMake files.

---

## Required CMake Layout

Refactor toward this layout:

```text
Engine/
  CMakeLists.txt

  Source/
    Foundation/
      CMakeLists.txt

    Runtime/
      Engine/
        CMakeLists.txt

      Platform/
        CMakeLists.txt

      Shader/
        CMakeLists.txt

      RHI/
        CMakeLists.txt

      Renderer/
        CMakeLists.txt
```

If the existing layout differs slightly, adapt cleanly while preserving the same module boundaries.

---

## Engine Top-level CMakeLists.txt

After refactor, `Engine/CMakeLists.txt` should mostly orchestrate modules.

Example:

```cmake
add_subdirectory(Source/Foundation)
add_subdirectory(Source/Runtime/Engine)
add_subdirectory(Source/Runtime/Platform)
add_subdirectory(Source/Runtime/Shader)
add_subdirectory(Source/Runtime/RHI)
add_subdirectory(Source/Runtime/Renderer)
```

It should not contain a huge flat source list anymore.

---

## Required CMake Targets

Prefer target-per-module.

Create or maintain these targets:

```text
XEngineFoundation
XEngineCoreRuntime
XEnginePlatform
XEngineShader
XEngineRHI
XEngineRenderer
XEngineRuntime
```

`XEngineRuntime` should be an aggregate target used by applications.

Apps should continue to link only:

```cmake
target_link_libraries(XEngineSandbox
    PRIVATE
        XEngineRuntime
)
```

and:

```cmake
target_link_libraries(XEngineEditorApp
    PRIVATE
        XEngineRuntime
)
```

Apps should not need to know about internal module targets.

---

## Target Responsibilities

### XEngineFoundation

Contains:

```text
Core
Logging
Assert
Math
Diagnostics
```

Depends on:

```text
spdlog private
glm private if GLM is introduced in Stage 5
```

---

### XEngineCoreRuntime

Contains:

```text
Engine
EngineConfig
Subsystem
SubsystemContext
SubsystemManager
Time
```

Depends on:

```text
XEngineFoundation
```

---

### XEnginePlatform

Contains:

```text
PlatformSystem
Window
WindowDesc
NativeWindowHandle
PlatformEvents
SDLWindow
SDLPlatformUtils
```

Depends on:

```text
XEngineFoundation
XEngineCoreRuntime
SDL private
```

SDL headers must remain private to Platform private implementation.

---

### XEngineShader

Contains:

```text
ShaderSystem
ShaderCompiler
ShaderTypes
ShaderReflection
ShaderModule
ShaderCache
SlangCompiler
SlangReflection
```

Depends on:

```text
XEngineFoundation
XEngineCoreRuntime
Slang private when XENGINE_ENABLE_SHADER_COMPILER=ON
```

Slang headers must remain private to Shader private implementation.

---

### XEngineRHI

Contains:

```text
RHI public API
RHISystem
Vulkan backend
VulkanDevice
VulkanSwapchain
VulkanShader
VulkanPipeline
VulkanBuffer
VulkanTexture
VulkanCommandList
VulkanAllocator
VulkanUtils
```

Depends on:

```text
XEngineFoundation
XEngineCoreRuntime
XEnginePlatform
XEngineShader public types
volk private
VMA private
Vulkan SDK headers private
SDL private only if needed for Vulkan surface handling
```

Vulkan SDK lookup must remain inside the RHI-related CMake, not root CMake.

Do not put:

```cmake
find_package(Vulkan REQUIRED)
```

in the root `CMakeLists.txt`.

---

### XEngineRenderer

Contains:

```text
RenderSystem
RenderSettings
RenderTypes
RenderGraph
RenderGraphContext
RenderGraphBuilder
RenderGraphExecutor
ClearPass
PresentPass
TrianglePass
ForwardMeshPass
StaticMesh
PrimitiveMeshes
```

Depends on:

```text
XEngineFoundation
XEngineCoreRuntime
XEngineShader
XEngineRHI
```

Renderer public headers must not expose Vulkan or Slang.

---

### XEngineRuntime

Aggregate target.

It should link:

```text
XEngineFoundation
XEngineCoreRuntime
XEnginePlatform
XEngineShader
XEngineRHI
XEngineRenderer
```

Apps should link to `XEngineRuntime`.

If `XEngineRuntime` already exists as a real library target, keep it working.
If it is easier to make it an interface aggregate target, that is acceptable as long as Apps build and link successfully.

---

## CMake Dependency Rules

Follow these rules:

```text
1. Root CMakeLists.txt only defines project/options and adds subdirectories.
2. ThirdParty/CMakeLists.txt handles vendored third-party libraries.
3. Engine module CMake files own engine module source lists.
4. Vulkan SDK lookup belongs in RHI CMake, not root.
5. SDL linking belongs in Platform CMake.
6. Slang linking belongs in Shader CMake.
7. spdlog linking belongs in Foundation CMake.
8. VMA include path belongs in RHI CMake.
9. Apps should link only XEngineRuntime.
10. Public include directories should only expose each module's Public folders.
11. Private implementation folders should remain private.
```

---

## ThirdParty CMake

Keep existing third-party policy:

```text
ThirdParty/spdlog
ThirdParty/SDL
ThirdParty/volk
ThirdParty/VulkanMemoryAllocator
ThirdParty/slang
```

If adding GLM in this stage:

```text
ThirdParty/glm
```

Do not add:

```text
fastgltf
stb_image
EnTT
ImGui
meshoptimizer
nlohmann/json
```

Those are later stages.

---

## CMake Acceptance Criteria

CMake modularization is complete when:

```text
1. Engine/CMakeLists.txt is no longer a large flat source list.
2. Each major engine module has its own CMakeLists.txt.
3. XEngineFoundation builds.
4. XEngineCoreRuntime builds.
5. XEnginePlatform builds.
6. XEngineShader builds.
7. XEngineRHI builds.
8. XEngineRenderer builds.
9. XEngineRuntime aggregates the needed modules.
10. XEngineSandbox links only XEngineRuntime.
11. XEngineEditorApp links only XEngineRuntime.
12. Vulkan SDK lookup remains in Engine/RHI-related CMake.
13. SDL is linked privately by Platform.
14. Slang is linked privately by Shader when shader compiler is enabled.
15. VMA is included privately by RHI.
16. Public include boundaries remain clean.
17. Project still configures and builds successfully.
```

---

# Part B - Basic Mesh Forward Renderer

## Goal

Move from:

```text
TrianglePass
  -> Draw(3)
```

to:

```text
ForwardMeshPass
  -> bind vertex buffer
  -> bind index buffer
  -> push MVP matrix
  -> DrawIndexed()
```

The mesh can be a hardcoded cube or similar indexed mesh.

Preferred result:

```text
A colored indexed cube rendered with depth testing.
```

---

# Math Requirements

If the project already has a math layer, use it.

If no math layer exists yet, add a minimal math solution.

Preferred simple option:

```text
Use GLM privately or through a thin XEngine math wrapper.
```

If adding GLM:

```text
ThirdParty/glm
```

Rules:

```text
- GLM may be used internally by Renderer private code.
- Avoid exposing GLM directly in public XEngine headers if practical.
- If public math types are needed, prefer XEngine-owned simple wrappers.
- Do not overbuild a full custom math library in Stage 5.
```

Minimal XEngine math public types can be:

```cpp
struct Vec2
{
    f32 X = 0.0f;
    f32 Y = 0.0f;
};

struct Vec3
{
    f32 X = 0.0f;
    f32 Y = 0.0f;
    f32 Z = 0.0f;
};

struct Vec4
{
    f32 X = 0.0f;
    f32 Y = 0.0f;
    f32 Z = 0.0f;
    f32 W = 0.0f;
};

struct Mat4
{
    f32 M[16] {};
};
```

If using GLM internally, conversion into `Mat4` is acceptable.

---

# Files to Implement or Update

Implement or update these files as needed:

```text
Engine/CMakeLists.txt

Engine/Source/Foundation/CMakeLists.txt
Engine/Source/Runtime/Engine/CMakeLists.txt
Engine/Source/Runtime/Platform/CMakeLists.txt
Engine/Source/Runtime/Shader/CMakeLists.txt
Engine/Source/Runtime/RHI/CMakeLists.txt
Engine/Source/Runtime/Renderer/CMakeLists.txt

Engine/Source/Foundation/Math/Public/XEngine/Math/MathTypes.h
Engine/Source/Foundation/Math/Public/XEngine/Math/Math.h
Engine/Source/Foundation/Math/Private/Math.cpp

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICommandList.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBuffer.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSwapchain.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSwapchain.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUtils.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUtils.cpp

Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderTypes.h
Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderSettings.h
Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderSystem.h

Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphContext.h
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphContext.cpp

Engine/Source/Runtime/Renderer/Private/Mesh/StaticMesh.h
Engine/Source/Runtime/Renderer/Private/Mesh/StaticMesh.cpp
Engine/Source/Runtime/Renderer/Private/Mesh/PrimitiveMeshes.h
Engine/Source/Runtime/Renderer/Private/Mesh/PrimitiveMeshes.cpp

Engine/Source/Runtime/Renderer/Private/Passes/ForwardMeshPass.h
Engine/Source/Runtime/Renderer/Private/Passes/ForwardMeshPass.cpp

Engine/Shaders/Passes/MeshForward.slang

ThirdParty/CMakeLists.txt
README.md
```

If files already exist, update them cleanly.

Do not delete `TrianglePass` unless it conflicts. It can remain as a debug/example pass.

---

# RHI Buffer Requirements

Stage 5 must introduce a minimal buffer abstraction.

## RHIBuffer.h

Create or update:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

#include <cstddef>

namespace XEngine
{
    enum class RHIBufferUsage : u32
    {
        None        = 0,
        Vertex      = 1 << 0,
        Index       = 1 << 1,
        Uniform     = 1 << 2,
        Storage     = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5
    };

    enum class RHIMemoryUsage
    {
        GPUOnly,
        CPUToGPU,
        GPUToCPU
    };

    struct RHIBufferDesc
    {
        std::size_t Size = 0;
        RHIBufferUsage Usage = RHIBufferUsage::None;
        RHIMemoryUsage MemoryUsage = RHIMemoryUsage::GPUOnly;
        const char* DebugName = nullptr;
    };

    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;

        virtual std::size_t GetSize() const = 0;
    };
}
```

Add bitwise helpers for `RHIBufferUsage` if needed.

---

## RHIDevice Buffer API

Add to `RHIDevice`:

```cpp
virtual std::shared_ptr<RHIBuffer> CreateBuffer(
    const RHIBufferDesc& desc,
    const void* initialData,
    std::size_t initialDataSize) = 0;
```

Stage 5 may use CPU-visible memory for simplicity.

Acceptable Stage 5 path:

```text
Vertex/index buffers use CPUToGPU memory.
Map memory.
Copy data.
Unmap memory.
Use buffer directly for rendering.
```

Future improvement:

```text
Stage 7+:
  staging buffers
  upload queue
  async asset upload
```

Do not implement a full staging upload system yet.

---

# VulkanBuffer Requirements

Implement `VulkanBuffer` using VMA.

It should own:

```cpp
VkBuffer m_Buffer = VK_NULL_HANDLE;
VmaAllocation m_Allocation = VK_NULL_HANDLE;
VmaAllocationInfo m_AllocationInfo {};
std::size_t m_Size = 0;
```

Creation should map XEngine usage flags to Vulkan usage flags:

```text
Vertex      -> VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
Index       -> VK_BUFFER_USAGE_INDEX_BUFFER_BIT
Uniform     -> VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
Storage     -> VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
TransferSrc -> VK_BUFFER_USAGE_TRANSFER_SRC_BIT
TransferDst -> VK_BUFFER_USAGE_TRANSFER_DST_BIT
```

Memory usage:

```text
CPUToGPU:
  host-visible allocation
  suitable for initial vertex/index data upload in Stage 5

GPUOnly:
  GPU-local allocation
```

For Stage 5, CPUToGPU is enough.

Keep the implementation simple and robust.

Do not implement staging upload yet.

---

# RHI Texture / Depth Buffer Requirements

Stage 5 needs a depth buffer.

Preferred approach:

```text
Expose minimal RHITexture / RHITextureDesc now,
but only implement the depth texture path needed by VulkanDevice.
```

## RHITexture.h

Create or update:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    enum class RHITextureUsage : u32
    {
        None            = 0,
        ColorAttachment = 1 << 0,
        DepthStencil    = 1 << 1,
        Sampled         = 1 << 2,
        TransferSrc     = 1 << 3,
        TransferDst     = 1 << 4
    };

    struct RHITextureDesc
    {
        u32 Width = 0;
        u32 Height = 0;
        RHIFormat Format = RHIFormat::Undefined;
        RHITextureUsage Usage = RHITextureUsage::None;
        const char* DebugName = nullptr;
    };

    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;
        virtual RHIFormat GetFormat() const = 0;
    };
}
```

Add bitwise helpers for `RHITextureUsage` if needed.

---

# Vulkan Depth Buffer Requirements

`VulkanDevice` should create a depth texture matching swapchain extent.

It should be recreated when swapchain is recreated.

Depth resource:

```text
VkImage
VkImageView
VmaAllocation
Format: VK_FORMAT_D32_SFLOAT
Usage: VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
Layout during rendering: VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
```

Depth clear:

```text
Clear depth to 1.0 at the beginning of ForwardMeshPass.
```

Acceptable approach:

```text
Use vkCmdBeginRendering with depth attachment loadOp = CLEAR.
```

Destroy depth image before destroying VMA allocator/device.

On resize:

```text
Recreate swapchain.
Recreate depth image.
```

Do not implement sampled depth yet.

Do not implement depth pyramid yet.

---

# RHICommandList Requirements

Extend `RHICommandList`.

Required methods:

```cpp
virtual void SetGraphicsPipeline(RHIPipeline* pipeline) = 0;

virtual void SetVertexBuffer(RHIBuffer* buffer, u64 offset = 0) = 0;
virtual void SetIndexBuffer(RHIBuffer* buffer, RHIIndexFormat format, u64 offset = 0) = 0;

virtual void PushConstants(
    ShaderStage stages,
    const void* data,
    std::size_t size,
    std::size_t offset = 0) = 0;

virtual void Draw(
    u32 vertexCount,
    u32 instanceCount,
    u32 firstVertex,
    u32 firstInstance) = 0;

virtual void DrawIndexed(
    u32 indexCount,
    u32 instanceCount,
    u32 firstIndex,
    i32 vertexOffset,
    u32 firstInstance) = 0;
```

Add to `RHITypes.h`:

```cpp
enum class RHIIndexFormat
{
    UInt16,
    UInt32
};
```

Do not expose Vulkan command buffers publicly.

Do not add descriptor APIs yet.

---

# Push Constants

Stage 5 should use push constants for the MVP matrix.

Add minimal pipeline support:

```text
RHIGraphicsPipelineDesc.PushConstantSize
RHIGraphicsPipelineDesc.PushConstantStages
```

Do not implement descriptor sets yet.

Push constant data:

```cpp
struct MeshPushConstants
{
    Mat4 ModelViewProjection;
};
```

Vulkan pipeline layout should include one push constant range:

```text
stageFlags = vertex stage
offset = 0
size = sizeof(MeshPushConstants)
```

---

# RHIPipeline Requirements

Extend `RHIGraphicsPipelineDesc`.

It should support:

```cpp
struct RHIVertexAttributeDesc
{
    u32 Location = 0;
    RHIFormat Format = RHIFormat::Undefined;
    u32 Offset = 0;
};

struct RHIVertexBufferLayoutDesc
{
    u32 Stride = 0;
    std::vector<RHIVertexAttributeDesc> Attributes;
};

struct RHIGraphicsPipelineDesc
{
    RHIShader* VertexShader = nullptr;
    RHIShader* FragmentShader = nullptr;

    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;

    bool EnableDepthTest = true;
    bool EnableDepthWrite = true;

    RHIVertexBufferLayoutDesc VertexLayout;

    u32 PushConstantSize = 0;
    ShaderStage PushConstantStages = ShaderStage::Vertex;

    const char* DebugName = nullptr;
};
```

Do not implement descriptor set layouts yet.

Do not implement blending beyond simple opaque overwrite.

Do not implement multiple color attachments yet.

---

# RHIFormat Requirements

Ensure `RHITypes.h` supports formats needed by Stage 5:

```cpp
enum class RHIFormat
{
    Undefined,
    BGRA8Unorm,
    RGBA8Unorm,
    RGBA16Float,
    D32Float,

    R32G32Float,
    R32G32B32Float,
    R32G32B32A32Float
};
```

Map these to Vulkan formats privately.

---

# Vulkan Pipeline Requirements

Update `VulkanPipeline` to support:

```text
vertex input layout
input assembly triangle list
viewport/scissor dynamic state
rasterization fill
back-face cull optional
multisampling 1 sample
depth test enabled
depth write enabled
color blend disabled
dynamic rendering with color + depth formats
push constant range
empty descriptor set layout
```

Use dynamic rendering.

Do not use traditional render pass / framebuffer.

Do not implement descriptor sets.

---

# VulkanCommandList Requirements

Implement:

```text
SetGraphicsPipeline:
  vkCmdBindPipeline

SetVertexBuffer:
  vkCmdBindVertexBuffers

SetIndexBuffer:
  vkCmdBindIndexBuffer

PushConstants:
  vkCmdPushConstants

Draw:
  vkCmdDraw

DrawIndexed:
  vkCmdDrawIndexed
```

The command list should use the current frame command buffer internally.

Do not expose `VkCommandBuffer` publicly.

---

# Dynamic Rendering Changes

Stage 5 must ensure dynamic rendering includes both:

```text
color attachment = swapchain image view
depth attachment = depth image view
```

Rendering flow:

```text
ClearPass:
  may still use vkCmdClearColorImage for swapchain color.

ForwardMeshPass:
  transition swapchain image to COLOR_ATTACHMENT_OPTIMAL.
  transition depth image to DEPTH_ATTACHMENT_OPTIMAL.
  begin dynamic rendering with color + depth attachments.
  bind pipeline.
  bind vertex/index buffers.
  push MVP constants.
  draw indexed mesh.
  end rendering.

EndFrame:
  transition swapchain image to PRESENT_SRC_KHR.
```

Depth attachment should clear to 1.0 at the beginning of the mesh pass.

Color attachment loadOp can be LOAD because ClearPass already cleared the swapchain image.

---

# Mesh Data Requirements

Add a simple private mesh representation.

## Vertex

```cpp
struct MeshVertex
{
    Vec3 Position;
    Vec3 Color;
};
```

## StaticMesh

```cpp
class StaticMesh
{
public:
    std::shared_ptr<RHIBuffer> VertexBuffer;
    std::shared_ptr<RHIBuffer> IndexBuffer;

    u32 VertexCount = 0;
    u32 IndexCount = 0;
    RHIIndexFormat IndexFormat = RHIIndexFormat::UInt32;
};
```

## PrimitiveMeshes

Create a hardcoded cube.

Preferred:

```text
Colored cube with 8 vertices and 36 indices.
```

A simpler indexed pyramid is acceptable if it validates depth.

Do not load mesh files.

Do not add glTF.

---

# RenderScene Initial Direction

Add minimal renderer-side structures.

In `RenderTypes.h` or private renderer files:

```cpp
struct RenderObject
{
    StaticMesh* Mesh = nullptr;

    Mat4 Model;
    Mat4 ModelViewProjection;

    u32 ObjectId = 0;
    u32 MeshId = 0;
    u32 MaterialId = 0;
};
```

Important:

```text
Start reserving ObjectId / MeshId / MaterialId for future GPUScene / GPU-driven rendering.
```

Do not implement GPUScene yet.

Do not implement SceneSystem yet.

Do not implement ECS yet.

For Stage 5, RenderSystem may internally create one RenderObject for the hardcoded cube.

---

# Camera Requirements

Implement a minimal fixed camera path.

Stage 5 may use a fixed camera.

Example:

```text
Position: (0, 0, 3)
LookAt:   (0, 0, 0)
Up:       (0, 1, 0)
FOV:      60 degrees
Near:     0.1
Far:      100.0
```

Compute:

```text
View matrix
Projection matrix
Model matrix
MVP = Projection * View * Model
```

If using Vulkan clip space, ensure projection is correct for Vulkan conventions.

It is acceptable to use GLM privately:

```text
glm::lookAt
glm::perspective
```

Remember Vulkan projection usually needs Y correction if using GLM default OpenGL conventions.

---

# MeshForward.slang

Create:

```text
Engine/Shaders/Passes/MeshForward.slang
```

The shader should use:

```text
vertex input:
  position
  color

push constant:
  MVP matrix

fragment output:
  vertex color
```

Example conceptual shader:

```hlsl
struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

struct PushConstants
{
    float4x4 modelViewProjection;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> g_PushConstants;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.position = mul(g_PushConstants.modelViewProjection, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target0
{
    return float4(input.color, 1.0);
}
```

If local Slang syntax for push constants differs, adapt correctly.

Do not use descriptor sets.

Do not use textures.

---

# ShaderSystem Integration

RenderSystem should compile `MeshForward.slang` through ShaderSystem.

Compile:

```text
vertexMain -> VulkanSPIRV
fragmentMain -> VulkanSPIRV
```

Then create:

```text
RHIShader vertex shader
RHIShader fragment shader
RHIPipeline mesh forward pipeline
```

It is acceptable to keep Triangle shader support from Stage 4.

Default rendering should use MeshForward pipeline.

---

# ForwardMeshPass

Create:

```text
Renderer/Private/Passes/ForwardMeshPass.h
Renderer/Private/Passes/ForwardMeshPass.cpp
```

Suggested API:

```cpp
void AddForwardMeshPass(
    RenderGraph& graph,
    RHIPipeline* pipeline,
    const std::vector<RenderObject>& objects);
```

If `RenderObject` uses private types, keep the pass private.

Execution:

```text
For each object:
  bind pipeline
  bind vertex buffer
  bind index buffer
  push MVP constants
  draw indexed
```

Stage 5 only needs one object, but structure should support multiple objects later.

---

# RenderSystem Behavior

`RenderSystem::OnCreate` should:

```text
1. Get ShaderSystem.
2. Get RHISystem / RHIDevice.
3. Create hardcoded cube mesh.
4. Compile MeshForward.slang vertex/fragment shaders.
5. Create RHIShader objects.
6. Create RHIGraphicsPipeline with:
   - vertex layout position/color
   - color format = swapchain color format
   - depth format = D32Float
   - depth test/write enabled
   - push constant range = MVP matrix
7. Create one RenderObject.
```

`RenderSystem::Render` should:

```text
1. Get RHIDevice.
2. BeginFrame.
3. Build RenderGraph:
   - ClearPass
   - ForwardMeshPass
   - PresentPass
4. Execute graph.
5. EndFrame.
```

On destroy:

```text
Release pipeline.
Release shaders.
Release mesh buffers.
```

Because `RenderSystem` is destroyed before `RHISystem`, GPU resources should be destroyed before device shutdown.

---

# Resize Behavior

When swapchain is resized:

```text
VulkanDevice recreates swapchain.
VulkanDevice recreates depth image.
Viewport/scissor should use current swapchain extent.
```

If pipeline depends only on format and format does not change, it can remain alive.

For Stage 5:

```text
Assume swapchain format stays stable.
Depth image must be recreated on resize.
Viewport/scissor should update per frame.
```

---

# RenderGraph Scope

Keep RenderGraph V0 linear.

Do not implement:

```text
resource handles
texture declarations
buffer declarations
automatic barrier scheduling
pass culling
resource aliasing
async compute
parallel execution
```

Pass execution order should remain:

```text
ClearPass
ForwardMeshPass
PresentPass
```

---

# README Cleanup Requirement

Update `README.md` carefully.

The README should no longer list completed sub-stages as separate top-level stages.

Merge completed sub-stage features into clean stage summaries.

Use this structure:

```text
Stage 0 - Foundation + Engine Loop
Stage 1 - SDL Platform Layer
Stage 2 - Vulkan RHI Clear Screen
Stage 3 - RenderSystem + Linear RenderGraph V0
Stage 4 - ShaderSystem + Slang + Triangle
Stage 5 - Engine CMake Modularization + Basic Mesh Forward Renderer
```

Do not show these as separate top-level stages anymore:

```text
Stage 2A
Stage 2B-1
Stage 2B-2
Stage 4A
Stage 4B
```

Instead, merge their features under their parent stage.

Example:

```text
Stage 2 - Vulkan RHI Clear Screen
- Vulkan SDK detection
- volk
- VMA
- VkInstance
- debug messenger
- SDL Vulkan surface
- physical/logical device
- queues
- swapchain
- command buffer
- sync objects
- clear with vkCmdClearColorImage
- present
- resize handling
```

Example:

```text
Stage 4 - ShaderSystem + Slang + Triangle
- ShaderSystem
- Slang online compilation
- Triangle.slang
- RHIShader
- RHIPipeline
- VulkanShader
- VulkanPipeline
- RHICommandList draw API
- TrianglePass
- first triangle rendering
```

Add Stage 5:

```text
Stage 5 - Engine CMake Modularization + Basic Mesh Forward Renderer
- Modular Engine CMake targets
- RHI buffers
- vertex/index buffers
- depth buffer
- basic mesh pipeline
- MeshForward.slang
- ForwardMeshPass
- hardcoded cube
- fixed camera
```

Also document:

```text
RenderFeature system is planned for Stage 9, not implemented in Stage 5.
GPU-driven rendering is planned later around GPUScene / Bindless / Advanced Renderer stages.
```

Keep README concise and useful.

Do not make README a giant design document.

---

# Do Not Implement

Do not implement:

```text
glTF loading
AssetSystem
MaterialSystem
Texture loading
Texture sampling
PBR
Scene ECS
ImGui
RenderFeature system
Bloom
Tonemap
FXAA
TAA
GPUScene
Bindless
Forward+
GPU-driven rendering
Meshlet rendering
Ray tracing
Async asset loading
JobSystem
Shader hot reload
Persistent shader cache
```

Do not replace RenderGraph V0 with a full render graph.

Do not expose Vulkan / VMA / Slang / SDL types in public headers.

---

# Acceptance Criteria

Stage 5 is complete when:

```text
1. Project configures successfully.
2. Project builds successfully.
3. Engine CMake is split into module-level CMakeLists.txt files.
4. Engine/CMakeLists.txt is no longer a giant flat source list.
5. Apps still link only XEngineRuntime.
6. Third-party dependencies remain private to the correct modules.
7. README is cleaned up and completed sub-stages are merged into parent stages.
8. Stage 5 is documented in README.
9. Existing Stage 0-4 behavior remains functional.
10. RHI buffer abstraction exists.
11. VulkanBuffer uses VMA.
12. Vertex buffer creation works.
13. Index buffer creation works.
14. Minimal RHI texture/depth abstraction exists or VulkanDevice owns a valid depth image.
15. Depth buffer is created.
16. Depth buffer is recreated on resize.
17. RHICommandList supports vertex buffer binding.
18. RHICommandList supports index buffer binding.
19. RHICommandList supports push constants.
20. RHICommandList supports DrawIndexed.
21. MeshForward.slang exists.
22. MeshForward.slang compiles through ShaderSystem.
23. Mesh forward pipeline is created.
24. ForwardMeshPass exists.
25. RenderGraph order is ClearPass -> ForwardMeshPass -> PresentPass.
26. A hardcoded indexed mesh renders.
27. Depth testing works.
28. Resize does not crash.
29. Window close exits cleanly.
30. RenderDoc can capture the mesh frame.
31. No glTF / AssetSystem / MaterialSystem / Texture / PBR is implemented.
32. No RenderFeature system is implemented.
33. No GPU-driven rendering is implemented.
34. Public headers do not expose Vulkan, VMA, Slang, or SDL.
```

---

# Final Task

Implement Stage 5 now.

Do not ask for confirmation.

Keep the implementation minimal, clean, and architecture-focused.

Where Stage 6 functionality is expected, leave clear TODO comments.
