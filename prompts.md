# XEngine Stage 1 Prompt - SDL Platform Layer + SubsystemContext

## Role

You are a senior C++ game engine architecture assistant.

You are working inside an existing C++20 project named **XEngine**.

Stage 0 has already been completed:

```text
Foundation
Logging through spdlog
Assert
Time
SubsystemManager
Engine lifecycle
Sandbox run loop
```

Your task is to implement **Stage 1: SDL Platform Layer + SubsystemContext**.

Do not implement Vulkan, Slang, ImGui, RenderGraph, Asset loading, Scene ECS, or Renderer logic yet.

---

# Stage 1 Goal

Implement a clean SDL3-based platform layer while keeping SDL hidden inside the private Platform implementation.

After this stage:

```text
XEngineSandbox starts
Engine initializes Log and SubsystemManager
Engine creates PlatformSystem
PlatformSystem initializes SDL
PlatformSystem creates the main window
Engine loop runs until the window close event
Closing the SDL window calls Engine::RequestShutdown()
PlatformSystem destroys the window
SDL shuts down cleanly
```

The main focus is:

```text
Engine
  -> SubsystemManager
  -> SubsystemContext
  -> PlatformSystem
  -> SDLWindow
  -> Event polling
  -> Window close
  -> Engine::RequestShutdown()
```

---

# Important Architecture Decisions

Follow these decisions strictly:

```text
1. Engine owns SubsystemManager.
2. Engine owns Time.
3. Logging remains a static service, not a subsystem.
4. PlatformSystem is a subsystem.
5. PlatformSystem owns the main Window.
6. SDL3 is an implementation detail of Platform/Private/SDL.
7. Public Platform headers must not expose SDL types.
8. Engine registers PlatformSystem in Stage 1.
9. Subsystem creation order remains registration order.
10. Subsystem destruction order remains reverse registration order.
11. Vulkan must not be implemented in this stage.
12. Renderer must not be implemented in this stage.
13. SDL3 is built from source under ThirdParty/SDL.
14. Do not use find_package(SDL3).
15. Link SDL3 dynamically by default.
```

---

# SDL Source Code Policy

The SDL3 source code will be manually copied from GitHub into:

```text
ThirdParty/SDL/
```

This directory is expected to contain SDL's own `CMakeLists.txt`.

Do not fetch SDL automatically.

Do not use `find_package(SDL3)`.

Do not assume SDL is installed globally on the system.

Use:

```cmake
add_subdirectory(ThirdParty/SDL)
```

or inside `ThirdParty/CMakeLists.txt`:

```cmake
add_subdirectory(SDL)
```

The intended dependency flow is:

```text
ThirdParty/SDL
  -> SDL3::SDL3-shared
  -> XEngineRuntime PRIVATE link
  -> XEngineSandbox / XEngineEditorApp
```

---

# SDL Linking Policy

SDL3 should be built together with XEngine.

Default linking mode:

```text
Shared / dynamic linking
```

Use the explicit SDL shared target:

```cmake
SDL3::SDL3-shared
```

Do not link against the generic alias unless necessary:

```cmake
SDL3::SDL3
```

Prefer:

```cmake
target_link_libraries(XEngineRuntime
    PRIVATE
        SDL3::SDL3-shared
)
```

Do not link SDL publicly.

Forbidden:

```cmake
target_link_libraries(XEngineRuntime
    PUBLIC
        SDL3::SDL3-shared
)
```

SDL is a private implementation detail of the Platform module.

---

# SDL CMake Options

Add these options in the root `CMakeLists.txt` if they do not already exist:

```cmake
option(XENGINE_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(XENGINE_ENABLE_SDL "Enable SDL platform backend" ON)
option(XENGINE_SDL_LINK_SHARED "Link SDL as a shared library" ON)
option(XENGINE_ENABLE_EDITOR "Build XEngine editor" ON)
option(XENGINE_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
```

For Stage 1:

```text
XENGINE_ENABLE_SDL should default to ON.
XENGINE_SDL_LINK_SHARED should default to ON.
```

---

# ThirdParty/CMakeLists.txt Requirements

Update `ThirdParty/CMakeLists.txt`.

Keep existing spdlog integration from Stage 0.

Add SDL integration like this:

```cmake
if(XENGINE_ENABLE_SDL)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt")
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

        if(XENGINE_SDL_LINK_SHARED)
            set(SDL_SHARED ON CACHE BOOL "" FORCE)
            set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        else()
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
        endif()

        add_subdirectory(SDL)
    else()
        message(FATAL_ERROR "SDL3 is required when XENGINE_ENABLE_SDL=ON. Please place SDL3 source under ThirdParty/SDL.")
    endif()
endif()
```

Do not add Vulkan, Slang, ImGui, EnTT, GLM, Tracy, or other third-party libraries yet.

---

# Runtime SDL Linking Requirements

Update `Engine/CMakeLists.txt`.

If SDL is enabled:

```cmake
if(XENGINE_ENABLE_SDL)
    target_compile_definitions(XEngineRuntime
        PRIVATE
            XENGINE_ENABLE_SDL
    )

    if(XENGINE_SDL_LINK_SHARED)
        target_link_libraries(XEngineRuntime
            PRIVATE
                SDL3::SDL3-shared
        )
    else()
        target_link_libraries(XEngineRuntime
            PRIVATE
                SDL3::SDL3-static
        )
    endif()
endif()
```

If the vendored SDL project exposes slightly different target names, adapt to the actual SDL3 CMake target names, but prefer `SDL3::SDL3-shared` for dynamic linking.

---

# Runtime Dependency Copy Requirements

Because SDL is dynamically linked by default, app executables must be able to find the SDL runtime library.

Add a CMake helper function, preferably in `Apps/CMakeLists.txt` or a small CMake utility file:

```cmake
function(xengine_copy_sdl_runtime target)
    if(XENGINE_ENABLE_SDL AND XENGINE_SDL_LINK_SHARED)
        if(WIN32)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL3::SDL3-shared>
                    $<TARGET_FILE_DIR:${target}>
            )
        endif()
    endif()
endfunction()
```

Call this for:

```cmake
xengine_copy_sdl_runtime(XEngineSandbox)
xengine_copy_sdl_runtime(XEngineEditorApp)
```

Only call it if the target exists.

For macOS/Linux, do not overcomplicate RPATH in Stage 1 unless needed.
If you add RPATH, keep it minimal and clean.

---

# Coding Style

Follow the existing XEngine style:

```text
Namespace: XEngine
Files: PascalCase.h / PascalCase.cpp
Types: PascalCase
Functions: PascalCase
Local variables: camelCase
Members: m_PascalCase
Static members: s_PascalCase
Macros: XENGINE_...
Braces: Allman
Indentation: 4 spaces
Column limit: 120
```

Use:

```cpp
#pragma once
```

Public include style:

```cpp
#include <XEngine/Core/Types.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Platform/Window.h>
```

Do not use long relative includes.

---

# Strict Include Boundaries

Allowed SDL includes:

```text
Engine/Source/Runtime/Platform/Private/SDL/
```

Forbidden SDL includes:

```text
Engine/Source/Runtime/Platform/Public/
Engine/Source/Runtime/Engine/Public/
Engine/Source/Runtime/RHI/Public/
Engine/Source/Runtime/Renderer/Public/
Apps/
```

Public headers must not include:

```cpp
#include <SDL3/SDL.h>
#include <SDL.h>
```

Only private SDL implementation files may include SDL.

---

# Files to Implement or Update

Implement or update these files:

```text
Engine/Source/Runtime/Engine/Public/XEngine/Engine/Subsystem.h
Engine/Source/Runtime/Engine/Public/XEngine/Engine/SubsystemContext.h
Engine/Source/Runtime/Engine/Public/XEngine/Engine/SubsystemManager.h
Engine/Source/Runtime/Engine/Private/SubsystemManager.cpp

Engine/Source/Runtime/Engine/Public/XEngine/Engine/EngineConfig.h
Engine/Source/Runtime/Engine/Public/XEngine/Engine/Engine.h
Engine/Source/Runtime/Engine/Private/Engine.cpp

Engine/Source/Runtime/Platform/Public/XEngine/Platform/Window.h
Engine/Source/Runtime/Platform/Public/XEngine/Platform/WindowDesc.h
Engine/Source/Runtime/Platform/Public/XEngine/Platform/NativeWindowHandle.h
Engine/Source/Runtime/Platform/Public/XEngine/Platform/PlatformEvents.h
Engine/Source/Runtime/Platform/Public/XEngine/Platform/PlatformSystem.h

Engine/Source/Runtime/Platform/Private/PlatformSystem.cpp
Engine/Source/Runtime/Platform/Private/Window.cpp

Engine/Source/Runtime/Platform/Private/SDL/SDLWindow.h
Engine/Source/Runtime/Platform/Private/SDL/SDLWindow.cpp
Engine/Source/Runtime/Platform/Private/SDL/SDLPlatformUtils.h
Engine/Source/Runtime/Platform/Private/SDL/SDLPlatformUtils.cpp

ThirdParty/CMakeLists.txt
Engine/CMakeLists.txt
Apps/CMakeLists.txt
Apps/Sandbox/CMakeLists.txt
Apps/EditorApp/CMakeLists.txt
Apps/Sandbox/Source/main.cpp
Apps/EditorApp/Source/main.cpp
README.md
```

If some files already exist, update them cleanly.

Do not modify unrelated RHI, Renderer, Shader, Asset, Scene, or UI files unless needed to keep the project building.

---

# Required SubsystemContext Refactor

Stage 1 introduces a lightweight `SubsystemContext`.

Create:

```text
Engine/Source/Runtime/Engine/Public/XEngine/Engine/SubsystemContext.h
```

Content:

```cpp
#pragma once

namespace XEngine
{
    class Engine;
    struct EngineConfig;

    struct SubsystemContext
    {
        Engine* Engine = nullptr;
        const EngineConfig* Config = nullptr;
    };
}
```

Update `Subsystem.h` so `OnCreate` receives context:

```cpp
#pragma once

#include <XEngine/Engine/SubsystemContext.h>

namespace XEngine
{
    class ISubsystem
    {
    public:
        virtual ~ISubsystem() = default;

        virtual void OnCreate(const SubsystemContext& context) {}
        virtual void OnDestroy() {}

        virtual void OnBeginFrame() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnEndFrame() {}
    };
}
```

Update `SubsystemManager`:

```cpp
void CreateAll(const SubsystemContext& context);
```

Create order remains registration order.
Destroy order remains reverse registration order.

Do not add dependency graph, priority sorting, parallel updates, or runtime removal yet.

---

# Required EngineConfig Updates

Update `EngineConfig.h` to include window settings:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

#include <string>

namespace XEngine
{
    struct EngineConfig
    {
        std::string ApplicationName = "XEngine";

        bool EnableValidation = true;
        bool EnableEditor = false;

        // 0 means run until RequestShutdown().
        u32 MaxFrames = 0;

        u32 WindowWidth = 1280;
        u32 WindowHeight = 720;

        bool WindowResizable = true;
        bool WindowMaximized = false;
        bool CreateMainWindow = true;
    };
}
```

Stage 1 apps should set:

```cpp
config.MaxFrames = 0;
config.CreateMainWindow = true;
```

---

# Required Engine Updates

Update `Engine` so it registers `PlatformSystem` when a main window is requested.

Expected behavior:

```text
Initialize:
- Log::Initialize()
- Store EngineConfig
- Reset Time
- Register PlatformSystem if Config.CreateMainWindow is true
- Build SubsystemContext
- Call SubsystemManager.CreateAll(context)
- Set initialized true

Run:
- Assert initialized
- Set running true
- While running:
  - Time.Tick()
  - SubsystemManager.BeginFrame()
  - SubsystemManager.Update(deltaTime)
  - SubsystemManager.EndFrame()
  - Stop if MaxFrames > 0 and frame count reaches MaxFrames
- Log stopped

Shutdown:
- RequestShutdown()
- Destroy all subsystems
- Log shutdown complete
- Log::Shutdown() last
```

Add or keep:

```cpp
void RequestShutdown();
bool IsRunning() const;
SubsystemManager& GetSubsystemManager();
const Time& GetTime() const;
```

The `PlatformSystem` should call `Engine::RequestShutdown()` when the window close event is received.

---

# Required Platform Public API

## WindowDesc.h

Create:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

#include <string>

namespace XEngine
{
    struct WindowDesc
    {
        std::string Title = "XEngine";
        u32 Width = 1280;
        u32 Height = 720;
        bool Resizable = true;
        bool Maximized = false;
    };
}
```

---

## NativeWindowHandle.h

Create or update:

```cpp
#pragma once

namespace XEngine
{
    struct NativeWindowHandle
    {
        void* Window = nullptr;
        void* Display = nullptr;
    };
}
```

Do not expose SDL types here.

---

## Window.h

Create or update:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Platform/NativeWindowHandle.h>
#include <XEngine/Platform/WindowDesc.h>

#include <string_view>

namespace XEngine
{
    class Window
    {
    public:
        virtual ~Window() = default;

        virtual void PollEvents() = 0;

        virtual bool ShouldClose() const = 0;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;

        virtual std::string_view GetTitle() const = 0;

        virtual NativeWindowHandle GetNativeHandle() const = 0;
    };
}
```

---

## PlatformEvents.h

Keep this minimal for now.

Create:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    enum class PlatformEventType
    {
        None,
        WindowClose,
        WindowResize
    };

    struct PlatformEvent
    {
        PlatformEventType Type = PlatformEventType::None;
        u32 Width = 0;
        u32 Height = 0;
    };
}
```

Do not build a full event bus yet.

---

## PlatformSystem.h

Create:

```cpp
#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>

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

    private:
        Engine* m_Engine = nullptr;
        std::unique_ptr<Window> m_MainWindow;
        bool m_Initialized = false;
    };
}
```

---

# Required PlatformSystem Behavior

`PlatformSystem::OnCreate`:

```text
- Save Engine pointer from context.
- If XENGINE_ENABLE_SDL is enabled:
  - Initialize SDL video subsystem.
  - Create SDLWindow using config values.
  - Log window creation.
- If SDL is disabled:
  - Log warning that no real platform window is created.
```

`PlatformSystem::OnBeginFrame`:

```text
- If main window exists:
  - Poll window events.
  - If window ShouldClose():
    - Log window close requested.
    - Call m_Engine->RequestShutdown().
```

`PlatformSystem::OnDestroy`:

```text
- Destroy main window before SDL_Quit.
- Quit SDL video subsystem.
- Reset state.
```

Do not call SDL functions from public headers.

---

# Required SDLWindow Implementation

Implement in:

```text
Engine/Source/Runtime/Platform/Private/SDL/SDLWindow.h
Engine/Source/Runtime/Platform/Private/SDL/SDLWindow.cpp
```

`SDLWindow.h` may include SDL because it is private.

Suggested private class:

```cpp
#pragma once

#include <XEngine/Platform/Window.h>

#if defined(XENGINE_ENABLE_SDL)
    #include <SDL3/SDL.h>
#endif

namespace XEngine
{
    class SDLWindow final : public Window
    {
    public:
        explicit SDLWindow(const WindowDesc& desc);
        ~SDLWindow() override;

        void PollEvents() override;

        bool ShouldClose() const override;

        u32 GetWidth() const override;
        u32 GetHeight() const override;

        std::string_view GetTitle() const override;

        NativeWindowHandle GetNativeHandle() const override;

    private:
#if defined(XENGINE_ENABLE_SDL)
        SDL_Window* m_Window = nullptr;
#endif

        std::string m_Title;
        u32 m_Width = 0;
        u32 m_Height = 0;
        bool m_ShouldClose = false;
    };
}
```

`SDLWindow.cpp` behavior:

```text
Constructor:
- Create SDL window with title, width, height, and flags.
- Use resizable/maximized flags from WindowDesc.
- Store width/height/title.
- Log success or assertion failure.

PollEvents:
- Call SDL_PollEvent in a loop.
- If SDL_EVENT_QUIT, set m_ShouldClose = true.
- If SDL_EVENT_WINDOW_CLOSE_REQUESTED, set m_ShouldClose = true.
- If SDL_EVENT_WINDOW_RESIZED, update width/height and log resize.

Destructor:
- Destroy SDL_Window if valid.
```

Use SDL3 event names.
Do not use SDL2 event constants.

If exact SDL3 names differ in the local version, adapt to SDL3 correctly.

---

# SDL Initialization Ownership

`SDL_Init` and `SDL_Quit` should be owned by `PlatformSystem`, not `SDLWindow`.

`SDLWindow` should only create and destroy the window.

This is important because later the platform system may manage more than one window.

---

# SDLPlatformUtils

Create placeholder helper files:

```text
SDLPlatformUtils.h
SDLPlatformUtils.cpp
```

For now, they can contain small utility functions or TODO comments.

Do not overbuild utilities yet.

---

# Native Window Handle

`SDLWindow::GetNativeHandle()` should return a generic handle without exposing SDL in public headers.

For Stage 1, it can simply return:

```cpp
NativeWindowHandle handle;
handle.Window = static_cast<void*>(m_Window);
handle.Display = nullptr;
return handle;
```

Later Vulkan/Metal/D3D12 can refine this if needed.

Do not implement Vulkan surface creation in this stage.

---

# Input System

Do not implement a full InputSystem yet.

If an InputSystem already exists as a stub, leave it as a stub.

Do not add input action mapping, keyboard state tracking, mouse state tracking, or gamepad support in Stage 1.

Window close event handling is enough.

---

# App Updates

## Apps/Sandbox/Source/main.cpp

Update:

```cpp
#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Sandbox";
    config.WindowWidth = 1280;
    config.WindowHeight = 720;
    config.WindowResizable = true;
    config.CreateMainWindow = true;
    config.MaxFrames = 0;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
```

## Apps/EditorApp/Source/main.cpp

Update:

```cpp
#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Editor";
    config.EnableEditor = true;
    config.WindowWidth = 1600;
    config.WindowHeight = 900;
    config.WindowResizable = true;
    config.CreateMainWindow = true;
    config.MaxFrames = 0;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
```

Do not initialize ImGui yet.

---

# Expected Logs

When running Sandbox, expected logs should be similar to:

```text
[XEngine] Log initialized
[XEngine] Initializing engine: XEngine Sandbox
[XEngine] Creating SDL window: XEngine Sandbox 1280x720
[XEngine] Engine initialized
[XEngine] Engine started
[XEngine] Window close requested
[XEngine] Engine stopped
[XEngine] Engine shutting down
[XEngine] Destroying SDL window
[XEngine] Engine shutdown complete
[XEngine] Log shutdown
```

Exact formatting can differ based on spdlog pattern.

---

# README Update

Update `README.md` to mention Stage 1:

```text
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
```

Also mention:

```text
Vulkan is not implemented yet.
SDL is hidden inside Platform/Private/SDL.
Public headers do not expose SDL types.
```

---

# Do Not Implement

Do not implement:

```text
Vulkan surface creation
Vulkan instance/device/swapchain
Renderer
RenderGraph
ShaderSystem Slang compilation
Asset loading
Scene ECS
ImGui
Input action mapping
File drag and drop
Clipboard
Multiple windows
High-DPI scaling
Gamepad support
```

Do not add these third-party libraries yet:

```text
Vulkan SDK
volk
VMA
Slang
Dear ImGui
GLM
Tracy
EnTT
fastgltf
stb_image
nlohmann/json
meshoptimizer
```

Only add SDL3 in this stage, assuming spdlog already exists from Stage 0.

---

# Acceptance Criteria

Stage 1 is complete when:

```text
1. Project configures with CMake.
2. Project builds successfully with XENGINE_ENABLE_SDL=ON.
3. SDL3 is built from ThirdParty/SDL source.
4. XEngineRuntime links SDL3 privately.
5. XEngineRuntime links SDL3 dynamically by default using SDL3::SDL3-shared.
6. Windows builds copy SDL3.dll next to XEngineSandbox and XEngineEditorApp.
7. XEngineSandbox launches an SDL window.
8. XEngineEditorApp launches an SDL window if editor build is enabled.
9. Closing the window exits the engine loop.
10. PlatformSystem is registered by Engine.
11. PlatformSystem is an ISubsystem.
12. SubsystemContext exists and is passed into OnCreate().
13. SDL headers appear only under Platform/Private/SDL.
14. Public Platform headers do not expose SDL types.
15. Vulkan is not implemented.
16. Renderer is not implemented.
17. ImGui is not implemented.
18. Shutdown order is clean: window destroyed before SDL_Quit, Log shutdown last.
```

---

# Final Task

Implement Stage 1 now.

Do not ask for confirmation.

Keep the implementation minimal, clean, and architecture-focused.

Where future platform features are not implemented yet, leave clear TODO comments.
