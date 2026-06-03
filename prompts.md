# XEngine Stage 3 Prompt - RenderSystem + Linear RenderGraph V0

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
- volk loader initialization
- VkInstance creation
- Vulkan debug messenger
- VkSurfaceKHR creation from SDL window
- VkPhysicalDevice selection
- VkDevice creation
- graphics / present queues
- VmaAllocator creation
- RHISystem observes WindowResize events

Stage 2B-2:
- Vulkan swapchain creation
- swapchain image views
- one-frame command resources
- vkCmdClearColorImage-based clear
- queue submit
- present
- basic resize / out-of-date handling
```

Your task is to implement **Stage 3: RenderSystem + Linear RenderGraph V0**.

This stage moves clear-frame execution out of `RHISystem` and into `RenderSystem + RenderGraph`.

Do not implement Slang, shaders, graphics pipeline, triangle rendering, render pass, framebuffer, dynamic rendering, depth buffer, vertex/index buffers, descriptors, textures, ImGui, Scene, Asset loading, GPU-driven rendering, or a full event system.

---

# Stage 3 Goal

After this stage:

```text
Engine registers:
  PlatformSystem
  RHISystem
  RenderSystem

RHISystem:
  Owns RHI device lifecycle
  Observes resize events
  Forwards resize requests to RHIDevice
  Does not directly clear/present every frame

RenderSystem:
  Owns frame rendering at a high level
  Builds a linear RenderGraph every frame
  Calls RHIDevice::BeginFrame()
  Executes RenderGraph
  Calls RHIDevice::EndFrame()

RenderGraph V0:
  Is a linear named pass list
  Supports AddPass / Clear / Compile / Execute
  Supports pass types:
    Graphics
    Compute
    Transfer
    Present
    External

ClearPass:
  Calls RHIDevice::ClearSwapchain()

PresentPass:
  Exists as a placeholder pass
  Actual present is still performed by RHIDevice::EndFrame()
```

The window should still clear to a fixed color every frame.

This stage is about **frame organization**, not visual features.

---

# Important Architecture Decisions

Follow these decisions strictly:

```text
1. Stage 3 is not a shader or triangle stage.
2. Stage 3 is not a full renderer feature stage.
3. RenderSystem becomes responsible for frame rendering.
4. RHISystem stops doing per-frame clear/present work.
5. RenderGraph V0 is linear, not a full DAG.
6. RenderGraph V0 does not do resource dependency analysis.
7. RenderGraph V0 does not do resource aliasing.
8. RenderGraph V0 does not do automatic barriers.
9. RenderGraph V0 does not do async compute.
10. RenderGraph V0 execution is single-threaded.
11. RenderGraphPassType should reserve future categories:
    - Graphics
    - Compute
    - Transfer
    - Present
    - External
12. Do not create a NeuralPass class.
13. Future neural rendering features should later be represented as Compute or External passes.
14. Do not implement JobSystem or RenderGraph task scheduling.
15. Do not expose Vulkan types in Renderer public headers.
```

---

# CMake Requirement Reminder

Keep the dependency ownership clean.

`find_package(Vulkan REQUIRED)` must remain in:

```text
Engine/CMakeLists.txt
```

Do **not** move it to root `CMakeLists.txt`.

Root `CMakeLists.txt` should only contain:

```text
project setup
global options
add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Apps)
```

`ThirdParty/CMakeLists.txt` should only handle vendored dependencies:

```text
spdlog
SDL
volk
VMA file existence check
```

Do not change this ownership model.

---

# Strict Include Boundaries

Public Renderer headers must not include:

```cpp
#include <volk.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
```

Forbidden locations for Vulkan / SDL / VMA includes:

```text
Engine/Source/Runtime/Renderer/Public/
Engine/Source/Runtime/RHI/Public/
Engine/Source/Runtime/Platform/Public/
Engine/Source/Runtime/Engine/Public/
Apps/
```

Allowed locations remain:

```text
Engine/Source/Runtime/RHI/Private/Vulkan/
Engine/Source/Runtime/Platform/Private/SDL/
```

RenderSystem should depend on RHI public API only.

---

# Files to Implement or Update

Implement or update these files:

```text
Engine/Source/Runtime/Engine/Private/Engine.cpp

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHISystem.h
Engine/Source/Runtime/RHI/Private/RHISystem.cpp

Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderSystem.h
Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderTypes.h

Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp

Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraph.h
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraph.cpp
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphPass.h
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphBuilder.h
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphBuilder.cpp
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphContext.h
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphContext.cpp
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphExecutor.h
Engine/Source/Runtime/Renderer/Private/RenderGraph/RenderGraphExecutor.cpp

Engine/Source/Runtime/Renderer/Private/Passes/ClearPass.h
Engine/Source/Runtime/Renderer/Private/Passes/ClearPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/PresentPass.h
Engine/Source/Runtime/Renderer/Private/Passes/PresentPass.cpp

Engine/CMakeLists.txt
README.md
```

If these files already exist, update them cleanly.

If old placeholder pass files exist, do not delete them unless they conflict. Leave TODOs for future stages.

Do not modify Shader, Asset, Scene, UI, Editor, or Vulkan backend implementation unless absolutely required for build consistency.

---

# Engine Registration Requirements

Update `Engine.cpp`.

Stage 2B-2 registration order was likely:

```text
PlatformSystem
RHISystem
```

Stage 3 registration order must be:

```text
PlatformSystem
RHISystem
RenderSystem
```

Expected logic:

```cpp
if (m_Config.CreateMainWindow)
{
    m_SubsystemManager.AddSubsystem<PlatformSystem>();
}

if (m_Config.CreateGraphicsDevice)
{
    m_SubsystemManager.AddSubsystem<RHISystem>();
    m_SubsystemManager.AddSubsystem<RenderSystem>();
}
```

Do not register SceneSystem.

Do not register UISystem.

Do not register EditorSystem.

---

# RHISystem Behavior Change

Stage 2B-2 may have clear-frame code inside `RHISystem::OnUpdate`.

Remove that responsibility.

`RHISystem::OnUpdate` should only:

```text
- Query PlatformSystem events.
- If WindowResize event:
  - Call RHIDevice::RequestResize(width, height).
  - Log resize forwarding if useful.
- Do not call BeginFrame.
- Do not call ClearSwapchain.
- Do not call EndFrame.
```

In short:

```text
RHISystem = RHI lifecycle + resize forwarding
RenderSystem = frame rendering
```

---

# RHIDevice API

Keep the Stage 2B-2 temporary frame validation API for now:

```cpp
virtual void BeginFrame() = 0;
virtual void ClearSwapchain(const RHIColor& color) = 0;
virtual void EndFrame() = 0;
virtual void RequestResize(u32 width, u32 height) = 0;
```

Do not refactor into full command list / render pass / resource barrier API yet.

Do not add pipeline, shader, descriptor, buffer, or texture creation APIs yet.

---

# RenderSystem Public API

Create or update:

```text
Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderSystem.h
```

Suggested content:

```cpp
#pragma once

#include <XEngine/Engine/Subsystem.h>

namespace XEngine
{
    class RHISystem;

    class RenderSystem final : public ISubsystem
    {
    public:
        RenderSystem();
        ~RenderSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnUpdate(float deltaTime) override;

    private:
        void Render();

    private:
        RHISystem* m_RHISystem = nullptr;
        bool m_Initialized = false;
    };
}
```

Public header must not include Vulkan headers.

---

# RenderSystem Behavior

`RenderSystem::OnCreate`:

```text
- Store RHISystem pointer from Engine -> SubsystemManager.
- Assert or log error if RHISystem is missing.
- Log RenderSystem creation.
```

`RenderSystem::OnUpdate`:

```text
- Call Render().
```

`RenderSystem::OnDestroy`:

```text
- Log RenderSystem shutdown.
- Clear internal state.
```

`RenderSystem::Render`:

```text
- If RHISystem is missing, return.
- Get RHIDevice from RHISystem.
- If device is missing or invalid, return.
- Call device->BeginFrame().
- Build RenderGraph:
  - Clear()
  - AddClearPass()
  - AddPresentPass()
  - Compile()
  - Execute()
- Call device->EndFrame().
```

Use a fixed clear color:

```cpp
RHIColor clearColor;
clearColor.R = 0.1f;
clearColor.G = 0.1f;
clearColor.B = 0.15f;
clearColor.A = 1.0f;
```

---

# RenderGraph V0 Design

RenderGraph V0 should be a linear pass list.

Do not implement a DAG.

Do not sort passes.

Do not implement resource analysis.

---

## RenderGraphPassType

Create:

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

Important:

```text
Do not create NeuralPass.
Future neural rendering features should be represented as Compute or External passes.
```

---

## RenderGraphPassDesc

Create:

```cpp
struct RenderGraphPassDesc
{
    std::string Name;
    RenderGraphPassType Type = RenderGraphPassType::Graphics;
};
```

---

## RenderGraphBuilder

Create a placeholder class.

```cpp
class RenderGraphBuilder
{
public:
    // TODO Stage 4+:
    // ReadTexture()
    // WriteTexture()
    // ReadBuffer()
    // WriteBuffer()
};
```

No resource declarations are required in Stage 3.

---

## RenderGraphContext

Create:

```cpp
#pragma once

namespace XEngine
{
    class RHIDevice;

    class RenderGraphContext
    {
    public:
        explicit RenderGraphContext(RHIDevice& device);

        RHIDevice& GetDevice();

    private:
        RHIDevice* m_Device = nullptr;
    };
}
```

Implementation should assert `m_Device` is not null when accessed.

---

## RenderGraphPass

Create:

```cpp
#pragma once

#include <functional>
#include <string>

namespace XEngine
{
    class RenderGraphBuilder;
    class RenderGraphContext;

    enum class RenderGraphPassType
    {
        Graphics,
        Compute,
        Transfer,
        Present,
        External
    };

    struct RenderGraphPassDesc
    {
        std::string Name;
        RenderGraphPassType Type = RenderGraphPassType::Graphics;
    };

    struct RenderGraphPass
    {
        RenderGraphPassDesc Desc;

        std::function<void(RenderGraphBuilder&)> Setup;
        std::function<void(RenderGraphContext&)> Execute;
    };
}
```

---

## RenderGraph

Create:

```cpp
#pragma once

#include "RenderGraphPass.h"

#include <vector>

namespace XEngine
{
    class RenderGraph
    {
    public:
        using SetupFunc = std::function<void(RenderGraphBuilder&)>;
        using ExecuteFunc = std::function<void(RenderGraphContext&)>;

        void AddPass(const RenderGraphPassDesc& desc, SetupFunc setup, ExecuteFunc execute);

        void Clear();
        void Compile();
        void Execute(RenderGraphContext& context);

        bool IsCompiled() const;
        std::size_t GetPassCount() const;

    private:
        std::vector<RenderGraphPass> m_Passes;
        bool m_Compiled = false;
    };
}
```

`Compile()` behavior:

```text
- Create a RenderGraphBuilder.
- Call setup function for each pass in order.
- Mark graph as compiled.
- No sorting.
- No dependency analysis.
```

`Execute()` behavior:

```text
- If not compiled, compile or assert.
- Execute pass callbacks in insertion order.
- Log pass names if useful.
```

Keep execution single-threaded.

---

## RenderGraphExecutor

Create a simple placeholder class.

It may be used by `RenderGraph::Execute`, or it may remain a TODO wrapper.

Do not overbuild.

Example:

```cpp
class RenderGraphExecutor
{
public:
    void Execute(RenderGraph& graph, RenderGraphContext& context);
};
```

If this creates unnecessary complexity, keep it minimal with TODOs.

---

# ClearPass

Create:

```text
Engine/Source/Runtime/Renderer/Private/Passes/ClearPass.h
Engine/Source/Runtime/Renderer/Private/Passes/ClearPass.cpp
```

`ClearPass.h`:

```cpp
#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RenderGraph;

    void AddClearPass(RenderGraph& graph, const RHIColor& clearColor);
}
```

`ClearPass.cpp` behavior:

```text
- Add pass named "ClearPass".
- Type = RenderGraphPassType::Graphics.
- Setup function is currently empty / TODO.
- Execute function calls context.GetDevice().ClearSwapchain(clearColor).
```

---

# PresentPass

Create:

```text
Engine/Source/Runtime/Renderer/Private/Passes/PresentPass.h
Engine/Source/Runtime/Renderer/Private/Passes/PresentPass.cpp
```

`PresentPass.h`:

```cpp
#pragma once

namespace XEngine
{
    class RenderGraph;

    void AddPresentPass(RenderGraph& graph);
}
```

`PresentPass.cpp` behavior:

```text
- Add pass named "PresentPass".
- Type = RenderGraphPassType::Present.
- Setup function is currently empty.
- Execute function is currently a placeholder.
- Add TODO comment:
  "Present is currently handled by RHIDevice::EndFrame().
   Future RHI will expose explicit present command."
```

Do not call present directly from PresentPass in Stage 3.

---

# RenderGraph Single-threading

Stage 3 RenderGraph must be single-threaded.

Do not implement:

```text
JobSystem
parallel graph compile
parallel graph execute
task graph
parallel command recording
async compute scheduling
```

But the API should not prevent these later.

Add comments where appropriate:

```text
TODO: future stages may compile/execute passes through JobSystem.
```

---

# GPU-driven / Compute / External Future-proofing

Stage 3 should not implement GPU-driven rendering.

However:

```text
RenderGraphPassType::Compute must exist for future:
- GPU culling
- Forward+ light culling
- Hi-Z generation
- Compute post-process

RenderGraphPassType::External must exist for future:
- DLSS
- XeSS
- FSR
- neural denoiser
- external vendor SDK passes
```

Do not create a `NeuralPass` class.

Do not create neural rendering systems.

---

# Runtime Behavior

Expected logs may include:

```text
[XEngine] Creating RenderSystem
[XEngine] RenderGraph compile: 2 passes
[XEngine] Executing RenderGraph pass: ClearPass
[XEngine] Executing RenderGraph pass: PresentPass
```

Exact formatting is flexible.

The visible result should remain:

```text
SDL window clears to fixed Vulkan clear color every frame.
```

---

# README Update

Update `README.md` to mention Stage 3:

```text
Stage 3 adds:
- RenderSystem subsystem
- Linear RenderGraph V0
- ClearPass
- PresentPass placeholder
- Pass type categories: Graphics / Compute / Transfer / Present / External
- RHISystem no longer directly clears every frame
```

Also mention:

```text
Stage 3 does not implement shaders, triangle rendering, RenderGraph resource dependencies, async compute, neural rendering, or GPU-driven rendering.
Future neural rendering features should be represented as Compute or External passes.
```

---

# Do Not Implement

Do not implement:

```text
Slang
ShaderSystem
Shader modules
Graphics pipeline
Triangle
Render pass
Framebuffer
Dynamic rendering
Depth buffer
Vertex buffer
Index buffer
Descriptor sets
Textures
Asset loading
Scene ECS
ImGui
Tracy GPU profiling
JobSystem
Parallel RenderGraph execution
Full EventBus
NeuralPass
GPU-driven rendering
Forward+
```

Do not enable advanced Vulkan features yet:

```text
VK_KHR_dynamic_rendering
VK_KHR_synchronization2
VK_EXT_descriptor_indexing
VK_EXT_descriptor_buffer
Ray tracing extensions
Mesh shader extensions
```

---

# Acceptance Criteria

Stage 3 is complete when:

```text
1. Project configures successfully.
2. Project builds successfully with XENGINE_ENABLE_VULKAN=ON.
3. Engine registers PlatformSystem -> RHISystem -> RenderSystem.
4. RHISystem no longer calls BeginFrame / ClearSwapchain / EndFrame in OnUpdate.
5. RenderSystem is an ISubsystem.
6. RenderSystem obtains RHISystem through SubsystemManager.
7. RenderSystem performs frame rendering.
8. RenderGraph V0 exists.
9. RenderGraph supports AddPass / Clear / Compile / Execute.
10. RenderGraph execution is single-threaded and insertion-order based.
11. RenderGraphPassType includes Graphics / Compute / Transfer / Present / External.
12. RenderGraphBuilder exists as a placeholder.
13. RenderGraphContext provides access to RHIDevice.
14. ClearPass calls RHIDevice::ClearSwapchain.
15. PresentPass exists as a placeholder.
16. Window still clears to fixed color.
17. Window close exits cleanly.
18. Window resize does not crash.
19. RenderDoc can still capture a clear frame.
20. No shaders or triangle rendering are implemented.
21. No graphics pipeline is implemented.
22. No RenderGraph resource dependency system is implemented.
23. No NeuralPass class is created.
24. Public Renderer headers do not expose Vulkan / SDL / VMA / volk.
```

---

# Final Task

Implement Stage 3 now.

Do not ask for confirmation.

Keep the implementation minimal, clean, and architecture-focused.

Where later stages are expected, leave clear TODO comments.
