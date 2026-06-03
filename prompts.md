# XEngine Stage 2B-1 Prompt - Vulkan Instance / Surface / Device / Allocator + PlatformEvent Queue

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

Stage 2A:
- Vulkan SDK detection
- volk integration
- VMA integration
- RHI public API skeleton
- RHISystem skeleton
- Vulkan backend skeleton
```

Your task is to implement **Stage 2B-1: Vulkan Instance / Surface / Device / Allocator + PlatformEvent Queue**.

Do not implement swapchain, command buffers, clear screen, present, RenderGraph, Renderer, Slang, pipelines, descriptors, buffers, textures, ImGui, or asset loading yet.

---

# Stage 2B-1 Goal

This stage should initialize the Vulkan backend far enough to prove that XEngine can create:

```text
volk loader
VkInstance
VkDebugUtilsMessengerEXT
VkSurfaceKHR from SDL window
VkPhysicalDevice selection
VkDevice
graphics queue
present queue
VmaAllocator
```

It should also introduce a minimal platform event queue so future swapchain resize handling has a clean path.

After this stage:

```text
XEngineSandbox starts
SDL window opens
RHISystem creates VulkanDevice
VulkanDevice creates instance / surface / device / queues / VMA allocator
Closing the window exits cleanly
Window resize events are recorded as PlatformEvent::WindowResize
RHISystem can observe resize events and mark a pending resize flag
No swapchain exists yet
No rendering happens yet
```

---

# Important Architecture Decisions

Follow these decisions strictly:

```text
1. Vulkan appears only under Runtime/RHI/Private/Vulkan.
2. Public RHI headers must not include volk, Vulkan, SDL, or VMA.
3. SDL may be included inside RHI/Private/Vulkan only for Vulkan surface creation.
4. PlatformSystem does not depend on RHISystem.
5. RHISystem may query PlatformSystem through SubsystemManager.
6. Resize handling is event/state based, not direct callback based.
7. Do not introduce a full EventBus yet.
8. Add only a minimal PlatformEvent queue.
9. Do not create a swapchain in this stage.
10. Do not render or clear the screen in this stage.
11. MaxFramesInFlight will be 1 later, but no frame resources are created yet.
12. VMA allocator is created in this stage, but no VMA allocations are made yet.
```

---

# Event System Scope

Do **not** implement a full engine-wide event system.

Do **not** implement:

```text
EventBus
Observer system
Listener registration
Callback dispatch
Input action routing
UI event capture
Thread-safe event queues
```

Implement only:

```text
PlatformSystem-owned PlatformEvent queue
Window::PollEvents(events)
PlatformSystem::GetEvents()
RHISystem reads PlatformSystem events
```

This is a temporary minimal event path for window close and window resize.

---

# Strict Include Boundaries

Public headers must not include:

```cpp
#include <volk.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
```

Forbidden locations for Vulkan / SDL / VMA includes:

```text
Engine/Source/Runtime/RHI/Public/
Engine/Source/Runtime/Platform/Public/
Engine/Source/Runtime/Renderer/Public/
Engine/Source/Runtime/Engine/Public/
Apps/
```

Allowed locations:

```text
Engine/Source/Runtime/RHI/Private/Vulkan/
Engine/Source/Runtime/Platform/Private/SDL/
```

Specific rule:

```text
RHI/Private/Vulkan may include SDL_vulkan only for Vulkan surface creation.
```

---

# Files to Implement or Update

Implement or update these files:

```text
Engine/Source/Runtime/Platform/Public/XEngine/Platform/PlatformEvents.h
Engine/Source/Runtime/Platform/Public/XEngine/Platform/Window.h
Engine/Source/Runtime/Platform/Public/XEngine/Platform/PlatformSystem.h
Engine/Source/Runtime/Platform/Private/PlatformSystem.cpp
Engine/Source/Runtime/Platform/Private/SDL/SDLWindow.h
Engine/Source/Runtime/Platform/Private/SDL/SDLWindow.cpp

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHISystem.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h

Engine/Source/Runtime/RHI/Private/RHISystem.cpp

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanInstance.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanInstance.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSurface.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSurface.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanQueue.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanQueue.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanAllocator.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanAllocator.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUtils.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUtils.cpp

Engine/CMakeLists.txt
README.md
```

If these files already exist, update them cleanly.

Do not modify Renderer, Shader, Asset, Scene, UI, or Editor code unless absolutely required for build consistency.

---

# PlatformEvent Queue Requirements

## PlatformEvents.h

Update:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class PlatformEventType
    {
        None,
        WindowClose,
        WindowResize,
        WindowMinimized,
        WindowRestored
    };

    struct PlatformEvent
    {
        PlatformEventType Type = PlatformEventType::None;

        u32 Width = 0;
        u32 Height = 0;
    };
}
```

---

## Window.h

Update `Window::PollEvents` to accept an event output container.

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/Platform/PlatformEvents.h>
#include <XEngine/Platform/WindowDesc.h>

#include <string_view>
#include <vector>

namespace XEngine
{
    class Window
    {
    public:
        virtual ~Window() = default;

        virtual void PollEvents(std::vector<PlatformEvent>& events) = 0;

        virtual bool ShouldClose() const = 0;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;

        virtual std::string_view GetTitle() const = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;
    };
}
```

---

## SDLWindow Behavior

Update `SDLWindow`:

```text
- Poll SDL events into std::vector<PlatformEvent>& events.
- On SDL_EVENT_QUIT:
  - Set m_ShouldClose = true.
  - Push PlatformEventType::WindowClose.
- On SDL_EVENT_WINDOW_CLOSE_REQUESTED:
  - Set m_ShouldClose = true.
  - Push PlatformEventType::WindowClose.
- On SDL_EVENT_WINDOW_RESIZED:
  - Update m_Width and m_Height.
  - Push PlatformEventType::WindowResize with Width and Height.
- On minimize/restored events if available in SDL3:
  - Push WindowMinimized / WindowRestored.
```

Use SDL3 event constants, not SDL2 constants.

If local SDL3 event names differ, adapt correctly to SDL3.

Do not implement input events yet.

---

## PlatformSystem.h

Update:

```cpp
#pragma once

#include <XEngine/Engine/Subsystem.h>
#include <XEngine/Platform/PlatformEvents.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class Window;

    class PlatformSystem final : public ISubsystem
    {
    public:
        PlatformSystem();
        ~PlatformSystem() override;

        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
        void OnBeginFrame() override;

        Window* GetMainWindow();
        const Window* GetMainWindow() const;

        const std::vector<PlatformEvent>& GetEvents() const;

    private:
        Engine* m_Engine = nullptr;
        std::unique_ptr<Window> m_MainWindow;
        std::vector<PlatformEvent> m_Events;

        bool m_Initialized = false;
    };
}
```

---

## PlatformSystem.cpp Behavior

`OnBeginFrame`:

```text
- Clear m_Events at the beginning of the frame.
- Poll main window events into m_Events.
- If any WindowClose event is present:
  - Log window close requested.
  - Call m_Engine->RequestShutdown().
```

Do not call RHISystem from PlatformSystem.

Do not do Vulkan work from PlatformSystem.

---

# SDL Window Vulkan Flag Requirement

Stage 2B-1 needs SDL window to support Vulkan surface creation later in the same stage.

Update SDL window creation flags to include:

```cpp
SDL_WINDOW_VULKAN
```

Also keep existing flags:

```text
Resizable
Maximized
```

Do not create Vulkan surface in SDLWindow.

SDLWindow should only own `SDL_Window*`.

---

# RHISystem Requirements

`RHISystem` should now:

```text
- Store SubsystemContext or Engine pointer internally if needed.
- OnCreate:
  - Get PlatformSystem from SubsystemManager.
  - Get main Window.
  - Get NativeWindowHandle.
  - Create VulkanDevice when backend is Vulkan and XENGINE_ENABLE_VULKAN is defined.
- OnUpdate:
  - Read PlatformSystem events.
  - If WindowResize event:
    - Set m_PendingResize = true.
    - Store pending width/height.
    - Log that resize was observed and swapchain recreation is TODO for Stage 2B-2.
  - Do not render.
- OnDestroy:
  - If device exists, WaitIdle.
  - Destroy device.
```

Add private members:

```cpp
Engine* m_Engine = nullptr;
bool m_PendingResize = false;
u32 m_PendingResizeWidth = 0;
u32 m_PendingResizeHeight = 0;
```

Do not implement swapchain recreation yet.

---

# RHI Public API Updates

## RHIDevice.h

Update to include initialization-independent interface:

```cpp
#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual RHIBackend GetBackend() const = 0;

        virtual bool IsValid() const = 0;

        virtual void WaitIdle() = 0;
    };
}
```

Do not expose Vulkan handles.

---

## RHITypes.h

Keep it Vulkan-free.

Ensure it contains:

```cpp
enum class RHIBackend
{
    None,
    Vulkan,
    D3D12,
    Metal
};
```

If needed, add:

```cpp
struct RHIPhysicalDeviceInfo
{
    const char* Name = "";
    u32 VendorId = 0;
    u32 DeviceId = 0;
};
```

Do not include Vulkan types.

---

# Vulkan Backend Design

Implement these classes:

```text
VulkanInstance
VulkanSurface
VulkanDevice
VulkanQueue
VulkanAllocator
```

Keep swapchain, command list, buffer, texture, shader, pipeline classes as skeletons.

---

# Vulkan Initialization Order

Implement this order:

```text
VulkanDevice::Initialize(...)
  -> volkInitialize()
  -> VulkanInstance::Create(...)
      - Query required SDL Vulkan instance extensions
      - Enable validation layers if requested
      - Enable VK_EXT_debug_utils if validation is enabled
      - Create VkInstance
      - volkLoadInstance(instance)
      - Create debug messenger if validation is enabled
  -> VulkanSurface::Create(instance, nativeWindowHandle)
      - Use SDL_Vulkan_CreateSurface
  -> PickPhysicalDevice(instance, surface)
      - Find a device with graphics queue and present support
      - Require VK_KHR_swapchain support for future Stage 2B-2
  -> CreateLogicalDevice(...)
      - Create graphics queue
      - Create present queue
      - Enable VK_KHR_swapchain
      - volkLoadDevice(device)
  -> VulkanAllocator::Create(instance, physicalDevice, device)
      - Create VmaAllocator
```

Do not create swapchain yet.

---

# SDL Vulkan Extension Query

Use SDL3 Vulkan helper functions to get required instance extensions.

Expected logic:

```text
- Ask SDL for required Vulkan instance extensions.
- Add those extensions to VkInstanceCreateInfo.
- If validation is enabled, add VK_EXT_debug_utils.
```

Use the appropriate SDL3 API from `SDL3/SDL_vulkan.h`.

If exact function signatures differ by local SDL3 version, adapt correctly.

Do not hardcode platform-specific Vulkan surface extensions unless absolutely necessary.

---

# SDL Vulkan Surface Creation

Create:

```text
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSurface.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSurface.cpp
```

`VulkanSurface` should:

```text
- Store VkSurfaceKHR.
- Create surface from SDL_Window* using SDL_Vulkan_CreateSurface.
- Destroy surface using vkDestroySurfaceKHR.
```

`VulkanSurface` may include:

```cpp
#include <SDL3/SDL_vulkan.h>
#include <volk.h>
```

This is allowed because it is private Vulkan backend code.

---

# Physical Device Selection Requirements

Implement a simple physical device selection.

Requirements:

```text
- Enumerate physical devices.
- Prefer discrete GPU if available.
- Accept integrated GPU if no discrete GPU exists.
- Require graphics queue family.
- Require present queue family for the created surface.
- Require VK_KHR_swapchain extension support.
- Log selected GPU name.
```

Create a private struct if useful:

```cpp
struct VulkanQueueFamilyIndices
{
    u32 GraphicsFamily = InvalidIndex;
    u32 PresentFamily = InvalidIndex;

    bool IsComplete() const;
};
```

Use `InvalidIndex` from Core handle/constants or define a private Vulkan constant.

Do not expose this in public RHI headers.

---

# Logical Device Requirements

Create logical device with:

```text
- Required queue families
- Graphics queue
- Present queue
- VK_KHR_swapchain extension enabled
```

Do not enable advanced features yet.

Do not enable descriptor indexing yet.

Do not enable synchronization2 yet.

Those are future stages.

---

# Validation Layer Requirements

If `EngineConfig.EnableValidation` is true:

```text
- Check that VK_LAYER_KHRONOS_validation is available.
- Enable it if available.
- If unavailable, log warning and continue or assert depending on current XEngine assert policy.
```

Recommended Stage 2B-1 behavior:

```text
Log warning and continue if validation layer is unavailable.
```

Enable debug messenger if possible.

Debug callback should log:

```text
Trace / Info / Warn / Error based on Vulkan severity
```

Keep implementation simple.

---

# volk Requirements

Use volk correctly:

```text
1. Call volkInitialize() before creating VkInstance.
2. Create VkInstance.
3. Call volkLoadInstance(instance).
4. Create VkDevice.
5. Call volkLoadDevice(device).
```

Do not include `<vulkan/vulkan.h>` directly.

Use `<volk.h>`.

---

# VMA Allocator Requirements

Create:

```text
VulkanAllocator.h
VulkanAllocator.cpp
```

`VulkanAllocator` should own:

```cpp
VmaAllocator m_Allocator = VK_NULL_HANDLE;
```

Behavior:

```text
Create:
- Fill VmaAllocatorCreateInfo.
- Provide instance, physical device, device.
- Provide VmaVulkanFunctions using vkGetInstanceProcAddr and vkGetDeviceProcAddr.
- Call vmaCreateAllocator.

Destroy:
- Call vmaDestroyAllocator if valid.
```

Do not allocate buffers/images yet.

Do not expose VMA in public headers.

`VulkanAllocator.h` is private and may include:

```cpp
#include <volk.h>
#include <vk_mem_alloc.h>
```

---

# VulkanUtils Requirements

Update `VulkanUtils.h/.cpp`.

Add:

```cpp
const char* VulkanResultToString(VkResult result);
```

Add a private check macro usable inside Vulkan backend:

```cpp
#define XENGINE_VK_CHECK(expression) ...
```

Behavior:

```text
- Execute expression.
- If result is not VK_SUCCESS:
  - Log error with VulkanResultToString.
  - Assert / DebugBreak.
```

Keep this macro private to Vulkan backend.

Do not put it in public headers.

---

# VulkanDevice Requirements

`VulkanDevice` should now own:

```text
VulkanInstance
VulkanSurface
VulkanAllocator

VkPhysicalDevice
VkDevice
VkQueue graphics queue
VkQueue present queue
u32 graphics family index
u32 present family index
```

Suggested public methods:

```cpp
class VulkanDevice final : public RHIDevice
{
public:
    VulkanDevice();
    ~VulkanDevice() override;

    bool Initialize(const VulkanDeviceCreateInfo& createInfo);
    void Shutdown();

    RHIBackend GetBackend() const override;
    bool IsValid() const override;
    void WaitIdle() override;
};
```

Create private `VulkanDeviceCreateInfo`:

```cpp
struct VulkanDeviceCreateInfo
{
    NativeWindowHandle NativeWindow;
    bool EnableValidation = true;
    bool EnableVSync = true;
};
```

This is private, not public RHI API.

---

# RHISystem + PlatformSystem Interaction

`RHISystem::OnCreate` should:

```text
- Use context.Engine.
- Get PlatformSystem through context.Engine->GetSubsystemManager().GetSubsystem<PlatformSystem>().
- Assert PlatformSystem exists when creating Vulkan backend.
- Get main window.
- Get NativeWindowHandle.
- Create VulkanDevice.
- Call VulkanDevice::Initialize(...).
```

Do not let PlatformSystem know about RHI.

Do not let PlatformSystem call RHISystem.

---

# Window Resize Handling in Stage 2B-1

This stage should only record resize events and mark pending resize.

Behavior:

```text
On PlatformEventType::WindowResize:
  - RHISystem sets m_PendingResize = true.
  - Store width and height.
  - Log:
    "Window resized to WxH. Swapchain recreation is TODO for Stage 2B-2."
```

Do not recreate swapchain.

Do not create swapchain.

Do not allocate frame resources.

If width or height is 0, still record it. Future stage can skip rendering while minimized.

---

# Engine Registration

Ensure Engine registers systems in this order when appropriate:

```text
PlatformSystem
RHISystem
```

Do not register Renderer.

Do not register UI.

Do not register Scene.

---

# CMake Requirements

Ensure the following:

```text
- Vulkan backend files compile only when XENGINE_ENABLE_VULKAN=ON.
- Files including volk / VMA are excluded when XENGINE_ENABLE_VULKAN=OFF.
- XEngineRuntime privately links volk.
- XEngineRuntime privately includes VMA.
- XEngineRuntime does not expose Vulkan include paths publicly.
- XEngineRuntime still privately links SDL from Stage 1.
```

If `VulkanSurface.cpp` includes SDL Vulkan headers, make sure SDL include paths are available privately through `XEngineRuntime` linkage.

---

# Runtime Behavior

When running `XEngineSandbox`, expected logs should include something similar to:

```text
[XEngine] Log initialized
[XEngine] Initializing engine: XEngine Sandbox
[XEngine] Creating SDL window: XEngine Sandbox 1280x720
[XEngine] Creating RHI system
[XEngine] Initializing Vulkan backend
[XEngine] Vulkan instance created
[XEngine] Vulkan surface created
[XEngine] Selected GPU: <GPU name>
[XEngine] Vulkan logical device created
[XEngine] VMA allocator created
[XEngine] Engine started
```

On window resize:

```text
[XEngine] Window resized to 1400x900. Swapchain recreation is TODO for Stage 2B-2.
```

On shutdown:

```text
[XEngine] Engine stopped
[XEngine] Destroying RHI system
[XEngine] Destroying VMA allocator
[XEngine] Destroying Vulkan device
[XEngine] Destroying Vulkan surface
[XEngine] Destroying Vulkan instance
[XEngine] Destroying SDL window
[XEngine] Log shutdown
```

Exact formatting can differ.

---

# README Update

Update `README.md` to mention Stage 2B-1:

```text
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
```

Also mention:

```text
Stage 2B-1 does not create a swapchain.
Stage 2B-1 does not render.
Stage 2B-1 does not clear the screen.
Swapchain and clear screen are planned for Stage 2B-2.
```

---

# Do Not Implement

Do not implement:

```text
VkSwapchainKHR
Swapchain image views
Command pool
Command buffers
Semaphores
Fences
Clear screen
Present
RenderGraph
Renderer
Slang
Pipeline
Triangle
Descriptor sets
Buffers
Textures
Images
ImGui
Scene
Asset loading
Full event bus
Input system
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

These are future stages.

---

# Acceptance Criteria

Stage 2B-1 is complete when:

```text
1. Project configures with XENGINE_ENABLE_VULKAN=ON.
2. Project builds with XENGINE_ENABLE_VULKAN=ON.
3. XEngineSandbox launches an SDL Vulkan-capable window.
4. volkInitialize is called successfully.
5. VkInstance is created successfully.
6. volkLoadInstance is called successfully.
7. Debug messenger is created when validation is enabled and available.
8. VkSurfaceKHR is created from SDL window.
9. Physical device is selected.
10. Graphics queue family is found.
11. Present queue family is found.
12. VkDevice is created successfully.
13. volkLoadDevice is called successfully.
14. Graphics and present queues are retrieved.
15. VmaAllocator is created successfully.
16. RHISystem observes WindowResize events and logs pending swapchain recreation.
17. Closing the SDL window exits cleanly.
18. Shutdown destroys VMA allocator, device, surface, debug messenger, and instance in correct order.
19. No swapchain is created.
20. No command buffer is created.
21. No clear screen or present is implemented.
22. Public headers do not include Vulkan, volk, VMA, SDL, or SDL_vulkan.
23. Vulkan/SDL/VMA includes appear only in allowed private implementation locations.
```

---

# Final Task

Implement Stage 2B-1 now.

Do not ask for confirmation.

Keep the implementation minimal, clean, and architecture-focused.

Where Stage 2B-2 functionality is expected, leave clear TODO comments.
