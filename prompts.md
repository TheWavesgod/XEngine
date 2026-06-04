# XEngine Stage 4A Prompt - ShaderSystem + Slang Online Compilation

## Role

You are a senior C++ game engine architecture assistant.

You are working inside an existing C++20 project named **XEngine**.

Previous stages are complete:

```text
Stage 0:
- Foundation
- spdlog logging
- Assert
- Time
- SubsystemManager
- Engine lifecycle

Stage 1:
- SDL3 platform layer
- SDLWindow
- PlatformSystem
- SubsystemContext
- Window close event requests Engine shutdown
- Minimal PlatformEvent queue

Stage 2A:
- Vulkan SDK detection
- volk integration
- VMA integration
- RHI public API skeleton
- RHISystem skeleton
- Vulkan backend skeleton

Stage 2B-1:
- Vulkan instance
- Vulkan debug messenger
- SDL Vulkan surface
- physical device selection
- logical device
- graphics / present queues
- VMA allocator

Stage 2B-2:
- Vulkan swapchain
- swapchain image views
- command buffer
- sync objects
- vkCmdClearColorImage
- present
- basic resize handling

Stage 3:
- RenderSystem subsystem
- Linear RenderGraph V0
- ClearPass
- PresentPass placeholder
- RenderSettings initial version
```

Your task is to implement **Stage 4A: ShaderSystem + Slang Online Compilation**.

This stage should integrate Slang as XEngine's primary shader compiler and compile `.slang` shader source files into SPIR-V at runtime during development.

Do **not** implement Vulkan shader modules, graphics pipelines, TrianglePass, draw calls, render pass, framebuffer, dynamic rendering, descriptors, vertex buffers, textures, materials, Scene, Asset loading, ImGui, JobSystem, RenderFeature system, shader hot reload, or offline shader packaging yet.

---

# Stage 4A Goal

After this stage:

```text
XEngine has a ShaderSystem subsystem.
ShaderSystem performs online shader compilation when XENGINE_ENABLE_SHADER_COMPILER=ON.
Slang is integrated as a private implementation detail.
ShaderSystem can compile Engine/Shaders/Passes/Triangle.slang to SPIR-V.
ShaderSystem public headers expose only XEngine-owned types.
No Slang type appears in public XEngine headers.
No RHI or Vulkan backend code includes Slang.
No Vulkan shader module is created yet.
No graphics pipeline is created yet.
No triangle is drawn yet.
The existing Stage 3 clear-frame behavior still works.
```

The goal is to validate this chain:

```text
ShaderSystem
  -> SlangCompiler
  -> Triangle.slang
  -> SPIR-V bytecode
  -> CompiledShader
```

Stage 4B will later consume the compiled SPIR-V to create:

```text
RHIShader
VkShaderModule
RHIPipeline
TrianglePass
```

---

# Important Architecture Decisions

Follow these strictly:

```text
1. ShaderSystem is a subsystem.
2. ShaderSystem is CPU-side and initializes before RHISystem / RenderSystem.
3. Slang is private to Runtime/Shader/Private/Slang.
4. Public Shader headers must not expose Slang types.
5. RHI must not know Slang.
6. RenderSystem must not directly call Slang.
7. Stage 4A performs online shader compilation only.
8. Stage 4A does not create RHIShader.
9. Stage 4A does not create VkShaderModule.
10. Stage 4A does not create graphics pipelines.
11. Stage 4A does not draw a triangle.
12. ShaderStage must include Compute for future Forward+ / GPU-driven rendering.
13. ShaderTarget must include VulkanSPIRV, D3D12DXIL, MetalMSL for future backends.
14. CompiledShader must support both binary and text outputs.
15. Reflection structures should exist, but reflection can be minimal or empty in Stage 4A.
16. Do not introduce JobSystem yet.
17. Shader compilation is synchronous in Stage 4A.
18. Do not introduce shader hot reload yet.
19. Do not introduce shader permutation system yet.
20. Do not introduce persistent shader cache yet.
```

---

# Online vs Offline Shader Compilation Policy

Stage 4A uses **online shader compilation**.

Online compilation means:

```text
.slang source
  -> ShaderSystem
  -> Slang compiler library
  -> SPIR-V / DXIL / MSL
  -> CompiledShader
```

Online shader compilation requires Slang as a runtime dependency.

For Stage 4A:

```text
XENGINE_ENABLE_SHADER_COMPILER = ON
XEngineRuntime privately links Slang
ShaderSystem compiles Triangle.slang online
```

Future release/runtime direction:

```text
Development / Editor builds:
  XENGINE_ENABLE_SHADER_COMPILER=ON
  Runtime or Editor can compile shaders online through Slang.

Release / packaged builds:
  XENGINE_ENABLE_SHADER_COMPILER=OFF
  Runtime should not link Slang.
  Runtime should load precompiled shader outputs instead.
```

Offline shader compilation will be implemented later through:

```text
Tools/ShaderCompiler
```

Future offline outputs may look like:

```text
Build/Generated/Shaders/Vulkan/Triangle.vertex.spv
Build/Generated/Shaders/Vulkan/Triangle.fragment.spv

Build/Generated/Shaders/D3D12/Triangle.vertex.dxil
Build/Generated/Shaders/D3D12/Triangle.fragment.dxil

Build/Generated/Shaders/Metal/Triangle.vertex.msl
Build/Generated/Shaders/Metal/Triangle.fragment.msl
```

Do not implement offline shader compiler in Stage 4A.

Do not implement precompiled shader loading in Stage 4A.

---

# Slang Dependency Policy

Use this expected layout:

```text
ThirdParty/
  slang/
```

The Slang source code is manually copied/downloaded into `ThirdParty/slang`.

Because dependency libraries are downloaded directly as source code, before integrating/building them, remove unnecessary files where safe, such as examples, tests, docs, CI files, and extra samples. Do **not** remove license files, CMake files, include folders, source folders, binaries/tools required by Slang, or files required for the dependency to build.

Do not fetch Slang automatically.

Do not use package managers.

Do not use `find_package(Slang)`.

Do not expose Slang as a public dependency.

Preferred dependency flow:

```text
ThirdParty/slang
  -> Slang compiler library / slangc tool
  -> XEngineRuntime PRIVATE dependency when XENGINE_ENABLE_SHADER_COMPILER=ON
  -> Runtime/Shader/Private/Slang
```

If the vendored Slang CMake exports usable library targets, link them privately to `XEngineRuntime`.

If Slang target names differ locally, adapt cleanly and leave comments.

If direct Slang C++ API integration is not immediately possible with the local source layout, implement a temporary `slangc` command-line fallback inside `SlangCompiler.cpp`, but keep the public ShaderSystem API unchanged.

The fallback must be clearly marked with:

```text
TODO: Replace slangc fallback with Slang C++ API integration.
```

---

# CMake Options

Add this option in the root `CMakeLists.txt` if it does not already exist:

```cmake
option(XENGINE_ENABLE_SHADER_COMPILER "Enable runtime shader compilation" ON)
```

For Stage 4A:

```text
XENGINE_ENABLE_SHADER_COMPILER should default to ON.
```

Keep existing options:

```cmake
option(XENGINE_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(XENGINE_ENABLE_SDL "Enable SDL platform backend" ON)
option(XENGINE_SDL_LINK_SHARED "Link SDL as a shared library" ON)
option(XENGINE_ENABLE_EDITOR "Build XEngine editor" ON)
option(XENGINE_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
```

---

# CMake Ownership Rules

Keep dependency ownership clean.

## Root CMakeLists.txt

Root `CMakeLists.txt` should contain:

```text
project setup
global options
add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Apps)
```

It should not contain Slang-specific target logic.

It should not contain `find_package(Slang)`.

It should not contain Vulkan target wiring either; Vulkan SDK lookup belongs in `Engine/CMakeLists.txt`.

---

## ThirdParty/CMakeLists.txt

`ThirdParty/CMakeLists.txt` should handle vendored dependencies:

```text
spdlog
SDL
volk
VMA file existence check
slang
```

Add Slang integration only when shader compiler is enabled:

```cmake
if(XENGINE_ENABLE_SHADER_COMPILER)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/slang/CMakeLists.txt")
        add_subdirectory(slang)
    else()
        message(FATAL_ERROR "Slang is required when XENGINE_ENABLE_SHADER_COMPILER=ON. Please place Slang source under ThirdParty/slang.")
    endif()
endif()
```

Do not fetch Slang.

Do not use `find_package(Slang)`.

Do not add unrelated third-party libraries in this stage.

---

## Engine/CMakeLists.txt

`XEngineRuntime` should link Slang privately when online compilation is enabled.

Use a defensive pattern:

```cmake
if(XENGINE_ENABLE_SHADER_COMPILER)
    target_compile_definitions(XEngineRuntime
        PRIVATE
            XENGINE_ENABLE_SHADER_COMPILER
    )

    if(TARGET slang)
        target_link_libraries(XEngineRuntime PRIVATE slang)
    elseif(TARGET slang::slang)
        target_link_libraries(XEngineRuntime PRIVATE slang::slang)
    elseif(TARGET Slang::slang)
        target_link_libraries(XEngineRuntime PRIVATE Slang::slang)
    else()
        message(WARNING "No Slang library target found. Stage 4A may need slangc fallback or local Slang target adaptation.")
    endif()

    target_include_directories(XEngineRuntime
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ThirdParty/slang/include
    )
endif()
```

If the local Slang source layout uses a different include directory, adapt it cleanly.

Do not expose Slang include directories publicly.

Do not link Slang publicly.

---

# Slang Runtime Copy Policy

If Slang builds as a shared library, app executables must be able to find the Slang runtime library.

If the Slang target is shared and provides a valid target file, add a post-build copy helper similar to SDL.

Example:

```cmake
function(xengine_copy_slang_runtime target)
    if(XENGINE_ENABLE_SHADER_COMPILER)
        if(TARGET slang)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:slang>
                    $<TARGET_FILE_DIR:${target}>
            )
        elseif(TARGET slang::slang)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:slang::slang>
                    $<TARGET_FILE_DIR:${target}>
            )
        elseif(TARGET Slang::slang)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Slang::slang>
                    $<TARGET_FILE_DIR:${target}>
            )
        endif()
    endif()
endfunction()
```

Only use this if the Slang target is actually a runtime library target.

If the local Slang integration differs, add TODO comments and keep the build clean.

---

# Strict Include Boundaries

Public XEngine headers must not include:

```cpp
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
```

Forbidden locations for Slang includes:

```text
Engine/Source/Runtime/Shader/Public/
Engine/Source/Runtime/RHI/Public/
Engine/Source/Runtime/Renderer/Public/
Engine/Source/Runtime/Engine/Public/
Apps/
```

Allowed location:

```text
Engine/Source/Runtime/Shader/Private/Slang/
```

Only Slang private implementation files may include Slang headers.

---

# Engine Registration Order

Update `Engine.cpp`.

Current Stage 3 order is likely:

```text
PlatformSystem
RHISystem
RenderSystem
```

Stage 4A order should become:

```text
PlatformSystem
ShaderSystem
RHISystem
RenderSystem
```

Expected logic:

```cpp
if (m_Config.CreateMainWindow)
{
    m_SubsystemManager.AddSubsystem<PlatformSystem>();
}

if (m_Config.EnableShaderCompiler)
{
    m_SubsystemManager.AddSubsystem<ShaderSystem>();
}
else
{
    // Stage 4A can still register ShaderSystem, but it should report that online compilation is disabled.
    // Choose one approach and keep it consistent.
}

if (m_Config.CreateGraphicsDevice)
{
    m_SubsystemManager.AddSubsystem<RHISystem>();
    m_SubsystemManager.AddSubsystem<RenderSystem>();
}
```

If `EngineConfig` does not yet have a shader compiler flag, add:

```cpp
bool EnableShaderCompiler = true;
```

Do not register AssetSystem.

Do not register SceneSystem.

Do not register UISystem.

Do not register EditorSystem.

ShaderSystem should initialize before RenderSystem because RenderSystem will later request compiled shaders.

---

# Files to Implement or Update

Implement or update these files:

```text
Engine/Source/Runtime/Engine/Public/XEngine/Engine/EngineConfig.h
Engine/Source/Runtime/Engine/Private/Engine.cpp

Engine/Source/Runtime/Shader/Public/XEngine/Shader/ShaderTypes.h
Engine/Source/Runtime/Shader/Public/XEngine/Shader/ShaderReflection.h
Engine/Source/Runtime/Shader/Public/XEngine/Shader/ShaderModule.h
Engine/Source/Runtime/Shader/Public/XEngine/Shader/ShaderCompiler.h
Engine/Source/Runtime/Shader/Public/XEngine/Shader/ShaderSystem.h

Engine/Source/Runtime/Shader/Private/ShaderCompiler.cpp
Engine/Source/Runtime/Shader/Private/ShaderSystem.cpp
Engine/Source/Runtime/Shader/Private/ShaderCache.cpp

Engine/Source/Runtime/Shader/Private/Slang/SlangCompiler.h
Engine/Source/Runtime/Shader/Private/Slang/SlangCompiler.cpp
Engine/Source/Runtime/Shader/Private/Slang/SlangReflection.cpp

Engine/Shaders/Passes/Triangle.slang

ThirdParty/CMakeLists.txt
Engine/CMakeLists.txt
Apps/CMakeLists.txt
README.md
```

If files already exist, update them cleanly.

Do not modify Vulkan pipeline files for drawing yet.

Do not implement TrianglePass yet.

Do not modify RenderSystem pass order except if needed to keep Stage 3 clear working.

---

# Public Shader API

## ShaderTypes.h

Create or update:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

#include <string>
#include <vector>

namespace XEngine
{
    enum class ShaderStage
    {
        Unknown,
        Vertex,
        Fragment,
        Compute
    };

    enum class ShaderTarget
    {
        Unknown,
        VulkanSPIRV,
        D3D12DXIL,
        MetalMSL
    };

    enum class ShaderCodeFormat
    {
        Unknown,
        Binary,
        Text
    };

    enum class ShaderCompileResult
    {
        Success,
        Failed,
        UnsupportedTarget,
        CompilerUnavailable
    };

    struct ShaderDefine
    {
        std::string Name;
        std::string Value;
    };

    struct ShaderCompileDesc
    {
        std::string Path;
        std::string EntryPoint;

        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::VulkanSPIRV;

        std::string Profile;

        std::vector<std::string> IncludeDirectories;
        std::vector<ShaderDefine> Defines;

        bool GenerateDebugInfo = true;
        bool EnableOptimization = false;
    };

    ShaderTarget ShaderTargetFromRHIBackendName(const std::string& backendName);
}
```

Do not include Slang headers.

The helper `ShaderTargetFromRHIBackendName` can be minimal or TODO.
Do not include RHI headers in ShaderTypes unless absolutely necessary.

---

## ShaderReflection.h

Create or update target-neutral reflection structures:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <string>
#include <vector>

namespace XEngine
{
    enum class ShaderResourceType
    {
        Unknown,
        UniformBuffer,
        StorageBuffer,
        Texture,
        Sampler,
        CombinedImageSampler,
        PushConstant
    };

    struct ShaderBindingLocation
    {
        // Vulkan descriptor set / logical bind group.
        u32 Set = 0;

        // Vulkan binding / logical binding.
        u32 Binding = 0;

        // D3D12 register space.
        u32 Space = 0;

        // D3D12 register index.
        u32 Register = 0;

        // Metal resource index / generic backend index.
        u32 Index = 0;
    };

    struct ShaderResourceBinding
    {
        std::string Name;
        ShaderResourceType Type = ShaderResourceType::Unknown;

        ShaderBindingLocation Location;

        u32 ArraySize = 1;

        ShaderStage Visibility = ShaderStage::Unknown;
    };

    struct ShaderReflection
    {
        std::vector<ShaderResourceBinding> Resources;
    };
}
```

Reflection can remain empty in Stage 4A.

Do not overbuild reflection yet.

---

## ShaderModule.h

Create or update:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Shader/ShaderReflection.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <string>
#include <vector>

namespace XEngine
{
    struct CompiledShader
    {
        ShaderStage Stage = ShaderStage::Unknown;
        ShaderTarget Target = ShaderTarget::Unknown;
        ShaderCodeFormat Format = ShaderCodeFormat::Unknown;

        std::string EntryPoint;
        std::string SourcePath;

        // Used by binary targets such as SPIR-V and DXIL.
        std::vector<u8> Bytecode;

        // Used by textual targets such as MSL.
        std::string SourceCode;

        ShaderReflection Reflection;

        ShaderCompileResult Result = ShaderCompileResult::Failed;
        std::string Diagnostics;

        bool IsValid() const
        {
            return Result == ShaderCompileResult::Success &&
                   (!Bytecode.empty() || !SourceCode.empty());
        }
    };
}
```

Target usage:

```text
VulkanSPIRV:
  Format = Binary
  Bytecode = SPIR-V

D3D12DXIL:
  Format = Binary
  Bytecode = DXIL

MetalMSL:
  Format = Text
  SourceCode = MSL
```

Stage 4A only needs to produce VulkanSPIRV.

---

## ShaderCompiler.h

Create or update:

```cpp
#pragma once

#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderTypes.h>

namespace XEngine
{
    class ShaderCompiler
    {
    public:
        virtual ~ShaderCompiler() = default;

        virtual bool IsAvailable() const = 0;

        virtual CompiledShader Compile(const ShaderCompileDesc& desc) = 0;
    };
}
```

---

## ShaderSystem.h

Create or update:

```cpp
#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Shader/ShaderModule.h>
#include <XEngine/Shader/ShaderTypes.h>

#include <memory>

namespace XEngine
{
    class ShaderCompiler;

    class ShaderSystem final : public ISubsystem
    {
    public:
        ShaderSystem();
        ~ShaderSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;

        bool IsCompilerAvailable() const;

        CompiledShader Compile(const ShaderCompileDesc& desc);

    private:
        std::unique_ptr<ShaderCompiler> m_Compiler;
        bool m_Initialized = false;
    };
}
```

Public header must not expose Slang.

---

# SlangCompiler Private API

Create:

```text
Engine/Source/Runtime/Shader/Private/Slang/SlangCompiler.h
Engine/Source/Runtime/Shader/Private/Slang/SlangCompiler.cpp
```

`SlangCompiler.h` can include Slang headers because it is private.

Suggested design:

```cpp
#pragma once

#include <XEngine/Shader/ShaderCompiler.h>

namespace XEngine
{
    class SlangCompiler final : public ShaderCompiler
    {
    public:
        SlangCompiler();
        ~SlangCompiler() override;

        bool IsAvailable() const override;

        CompiledShader Compile(const ShaderCompileDesc& desc) override;

    private:
        bool Initialize();
        void Shutdown();

    private:
        bool m_Initialized = false;
    };
}
```

All Slang-specific objects must stay private to `SlangCompiler.cpp`.

---

# Slang Compilation Requirements

Stage 4A should support:

```text
Target:
  VulkanSPIRV

Stages:
  Vertex
  Fragment
  Compute reserved for future
```

For `ShaderTarget::VulkanSPIRV`, output should be SPIR-V bytecode:

```text
CompiledShader.Format = ShaderCodeFormat::Binary
CompiledShader.Bytecode = SPIR-V bytes
CompiledShader.Result = ShaderCompileResult::Success
```

For unsupported targets in Stage 4A:

```text
D3D12DXIL:
  Return invalid CompiledShader with Result = UnsupportedTarget.

MetalMSL:
  Return invalid CompiledShader with Result = UnsupportedTarget.
```

Do not implement DXIL or MSL yet.

However, keep the API compatible with future DXIL/MSL support.

---

# Slang C++ API Preferred Behavior

If using the Slang C++ API, implement these conceptual steps:

```text
1. Create Slang global session.
2. Create a compile session targeting SPIR-V.
3. Load or compile the module from file.
4. Find entry point by name.
5. Compose module + entry point.
6. Request target code as SPIR-V.
7. Copy SPIR-V bytes into CompiledShader.Bytecode.
8. Fill CompiledShader metadata.
9. Fill Diagnostics if compilation fails.
```

Use RAII where practical.

Do not expose Slang COM pointers or Slang types in public headers.

---

# Command-line slangc Fallback

If Slang C++ API target/library integration is not ready, implement a temporary fallback using `slangc`.

Rules:

```text
- Fallback lives only inside SlangCompiler.cpp.
- Public ShaderSystem API must not change.
- Use a temporary output path under Build/Generated/Shaders or another clearly named intermediate directory.
- Invoke slangc to compile one entry point at a time to SPIR-V.
- Read compiled SPIR-V binary into CompiledShader.Bytecode.
- Log the exact command used.
- Mark fallback with TODO:
  "Replace slangc fallback with Slang C++ API integration."
```

Do not rely on slangc permanently.

Do not put slangc command logic into public headers.

---

# Shader Path Policy

For Stage 4A, the shader path may be a normal filesystem path.

Do not implement a full FileSystem subsystem yet unless already present.

Recommended behavior:

```text
ShaderCompileDesc.Path can be:
  Engine/Shaders/Passes/Triangle.slang
  or an absolute/relative path from the working directory.

SlangCompiler should log the path being compiled.
```

Keep the code structured so it can later be replaced by FileSystem.

Do not implement file watching.

Do not implement hot reload.

Do not implement virtual shader paths yet unless already available.

---

# Triangle.slang

Create:

```text
Engine/Shaders/Passes/Triangle.slang
```

The shader should not require vertex buffers, descriptor sets, uniform buffers, textures, or push constants.

Use vertex ID to generate triangle vertices.

Example content:

```hlsl
struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
VSOutput vertexMain(uint vertexId : SV_VertexID)
{
    float2 positions[3] =
    {
        float2(0.0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5)
    };

    float3 colors[3] =
    {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    VSOutput output;
    output.position = float4(positions[vertexId], 0.0, 1.0);
    output.color = colors[vertexId];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target0
{
    return float4(input.color, 1.0);
}
```

If local Slang syntax requires slight adjustments, adapt it.

---

# ShaderSystem Startup Validation

In Stage 4A, `ShaderSystem::OnCreate` should validate compiler integration by compiling the test shader.

Behavior:

```text
- Create SlangCompiler when XENGINE_ENABLE_SHADER_COMPILER is defined.
- Initialize compiler.
- Compile Triangle.slang vertex entry point.
- Compile Triangle.slang fragment entry point.
- Log bytecode sizes.
- If compilation fails, log diagnostics clearly.
- Do not create RHIShader.
- Do not create Vulkan shader module.
- Do not create pipeline.
```

Suggested compile descriptors:

```cpp
ShaderCompileDesc vertexDesc;
vertexDesc.Path = "Engine/Shaders/Passes/Triangle.slang";
vertexDesc.EntryPoint = "vertexMain";
vertexDesc.Stage = ShaderStage::Vertex;
vertexDesc.Target = ShaderTarget::VulkanSPIRV;
vertexDesc.GenerateDebugInfo = true;
vertexDesc.EnableOptimization = false;

ShaderCompileDesc fragmentDesc;
fragmentDesc.Path = "Engine/Shaders/Passes/Triangle.slang";
fragmentDesc.EntryPoint = "fragmentMain";
fragmentDesc.Stage = ShaderStage::Fragment;
fragmentDesc.Target = ShaderTarget::VulkanSPIRV;
fragmentDesc.GenerateDebugInfo = true;
fragmentDesc.EnableOptimization = false;
```

For Stage 4A, it is acceptable to assert on compilation failure.

---

# ShaderCache

`ShaderCache.cpp` can remain minimal.

For Stage 4A, it may contain TODO comments only.

Do not implement:

```text
Persistent shader cache
Hash-based cache
Shader dependency tracking
Shader hot reload
Permutation system
```

Future cache key should include:

```text
Path
Entry point
Stage
Target
Defines
Profile
Debug/optimization flags
```

But do not implement full cache in Stage 4A.

---

# RenderSystem Integration

Do not make RenderSystem draw a triangle in Stage 4A.

RenderSystem should continue using Stage 3:

```text
ClearPass
PresentPass
```

Do not add TrianglePass yet.

Do not create pipelines in RenderSystem yet.

Stage 4B will connect RenderSystem to compiled shader objects.

---

# RHI / Vulkan Restrictions

Do not implement:

```text
RHIShader creation
RHIPipeline creation
VulkanShader
VulkanPipeline
TrianglePass
Draw commands
Dynamic rendering
Pipeline layout
Descriptor set layout
```

These belong to Stage 4B.

Existing Stage 2B-2 and Stage 3 clear-screen behavior must keep working.

---

# README Update

Update `README.md` to mention Stage 4A:

```text
Stage 4A adds:
- ShaderSystem subsystem
- Online shader compilation
- XENGINE_ENABLE_SHADER_COMPILER option
- Slang integration
- ShaderStage / ShaderTarget / ShaderCodeFormat public types
- CompiledShader public structure
- ShaderReflection placeholder
- Triangle.slang sample shader
- SPIR-V compilation validation
```

Also mention:

```text
Stage 4A links Slang privately when XENGINE_ENABLE_SHADER_COMPILER=ON.
Stage 4A does not create Vulkan shader modules.
Stage 4A does not create graphics pipelines.
Stage 4A does not draw a triangle yet.
Stage 4B will create RHIShader / RHIPipeline and TrianglePass.
Future release/runtime builds may disable online shader compilation and load precompiled shader outputs instead.
```

---

# Do Not Implement

Do not implement:

```text
RHIShader
RHIPipeline
VkShaderModule
VulkanPipeline
Graphics pipeline
Pipeline layout
Descriptor set layout
TrianglePass
Draw calls
Vertex buffer
Index buffer
Render pass
Framebuffer
Dynamic rendering
Textures
Materials
Scene
Asset system
ImGui
JobSystem
Shader hot reload
Shader permutation system
Persistent shader cache
Offline shader compiler tool
Precompiled shader package loader
RenderFeature system
```

Do not modify RenderGraph into a full resource graph yet.

---

# Acceptance Criteria

Stage 4A is complete when:

```text
1. Project configures successfully.
2. Project builds successfully.
3. XENGINE_ENABLE_SHADER_COMPILER option exists and defaults to ON.
4. Slang source under ThirdParty/slang is integrated or a clearly marked slangc fallback is implemented.
5. Slang is linked privately to XEngineRuntime when online shader compilation is enabled.
6. ShaderSystem is registered before RHISystem and RenderSystem.
7. ShaderSystem is an ISubsystem.
8. ShaderSystem public API exposes only XEngine-owned types.
9. Public headers do not include Slang headers.
10. Slang headers appear only under Runtime/Shader/Private/Slang.
11. Triangle.slang exists under Engine/Shaders/Passes/.
12. ShaderSystem compiles vertexMain to Vulkan SPIR-V.
13. ShaderSystem compiles fragmentMain to Vulkan SPIR-V.
14. CompiledShader supports Binary and Text formats.
15. VulkanSPIRV output uses Binary bytecode.
16. MetalMSL is represented as a future Text output target but is not implemented yet.
17. D3D12DXIL is represented as a future Binary output target but is not implemented yet.
18. Logs show successful compilation and bytecode sizes.
19. Unsupported targets fail cleanly or return invalid shaders.
20. Existing Stage 3 clear-screen frame still works.
21. No Vulkan shader modules are created.
22. No graphics pipeline is created.
23. No triangle is drawn yet.
24. No RenderFeature system is implemented.
25. Runtime/offline shader compilation separation is documented in README.
```

---

# Final Task

Implement Stage 4A now.

Do not ask for confirmation.

Keep the implementation minimal, clean, and architecture-focused.

Where Stage 4B functionality or future offline shader compilation is expected, leave clear TODO comments.
