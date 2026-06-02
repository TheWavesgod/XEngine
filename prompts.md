# XEngine Stage 0 Prompt - Foundation + Engine Loop + spdlog

## Role

You are a senior C++ game engine architecture assistant.

You are working inside an existing C++20 project named **XEngine**.

The project scaffold already exists. Your task is to implement **Stage 0: Foundation + Engine Loop**.

Do not implement SDL, Vulkan, Slang, ImGui, RenderGraph, Asset loading, or Scene logic yet.

This stage focuses only on:

```text
Foundation
Logging
Assert
Time
Subsystem lifecycle
Engine lifecycle
Sandbox run loop
Basic CMake cleanup
```

---

# Stage 0 Goal

Implement a clean and stable runtime skeleton for XEngine.

After this stage, the following should work:

```text
XEngineSandbox starts
XEngine::Engine initializes
XEngine::Log initializes through spdlog
Engine owns SubsystemManager
Time updates every frame
SubsystemManager calls lifecycle methods
Engine runs a small loop
Engine shuts down cleanly
```

The output should show clear lifecycle logs.

Example expected output:

```text
[XEngine] Log initialized
[XEngine] Initializing engine: XEngine Sandbox
[XEngine] Engine initialized
[XEngine] Engine started
[XEngine] Frame 0
[XEngine] Frame 1
[XEngine] Frame 2
[XEngine] Engine shutting down
[XEngine] Engine shutdown complete
[XEngine] Log shutdown
```

---

# Important Architecture Decisions

Follow these decisions strictly:

```text
1. Engine owns SubsystemManager.
2. Subsystem creation order is registration order.
3. Subsystem destruction order is reverse registration order.
4. Logging is a static service, not a subsystem.
5. Assert is macro-based and calls DebugBreak.
6. Time is an internal Engine service, not a subsystem.
7. spdlog is stored under ThirdParty/spdlog and linked privately by XEngineFoundation.
8. Public XEngine headers must not expose spdlog types.
9. No SDL dependency in this stage.
10. No Vulkan dependency in this stage.
```

---

# ThirdParty Policy for spdlog

spdlog repo is already in path E:\Project\C++\spdlog, copy the needed file from there 

Use this layout:

```text
ThirdParty/
  spdlog/
```

The `XEngineFoundation` target should link spdlog privately:

```cmake
target_link_libraries(XEngineFoundation
    PRIVATE
        spdlog::spdlog
)
```

Do not include spdlog in public headers.

Allowed:

```cpp
// Engine/Source/Foundation/Logging/Private/Log.cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
```

Forbidden:

```cpp
// Public headers
#include <spdlog/spdlog.h>
```

If `ThirdParty/spdlog/CMakeLists.txt` exists, add it with `add_subdirectory`.

If spdlog is missing, CMake should fail with a clear message:

```text
spdlog is required for Stage 0.
Please place spdlog under ThirdParty/spdlog.
```

Do not silently fall back to std::cout in this stage.

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
#include <XEngine/Logging/Log.h>
```

Do not use long relative includes.

---

# Files to Implement or Update

Implement or update these files.

```text
Engine/Source/Foundation/Core/Public/XEngine/Core/Types.h
Engine/Source/Foundation/Core/Public/XEngine/Core/Defines.h
Engine/Source/Foundation/Core/Public/XEngine/Core/Assert.h
Engine/Source/Foundation/Core/Private/Assert.cpp
Engine/Source/Foundation/Core/Public/XEngine/Core/NonCopyable.h
Engine/Source/Foundation/Core/Public/XEngine/Core/Result.h
Engine/Source/Foundation/Core/Public/XEngine/Core/Handle.h

Engine/Source/Foundation/Logging/Public/XEngine/Logging/Log.h
Engine/Source/Foundation/Logging/Private/Log.cpp

Engine/Source/Foundation/Diagnostics/Public/XEngine/Diagnostics/Profiler.h
Engine/Source/Foundation/Diagnostics/Public/XEngine/Diagnostics/ScopedTimer.h
Engine/Source/Foundation/Diagnostics/Public/XEngine/Diagnostics/DebugMarker.h
Engine/Source/Foundation/Diagnostics/Private/Profiler.cpp

Engine/Source/Runtime/Engine/Public/XEngine/Engine/EngineConfig.h
Engine/Source/Runtime/Engine/Public/XEngine/Engine/Subsystem.h
Engine/Source/Runtime/Engine/Public/XEngine/Engine/SubsystemManager.h
Engine/Source/Runtime/Engine/Private/SubsystemManager.cpp
Engine/Source/Runtime/Engine/Public/XEngine/Engine/Time.h
Engine/Source/Runtime/Engine/Private/Time.cpp
Engine/Source/Runtime/Engine/Public/XEngine/Engine/Engine.h
Engine/Source/Runtime/Engine/Private/Engine.cpp

Apps/Sandbox/Source/main.cpp
Apps/EditorApp/Source/main.cpp

ThirdParty/CMakeLists.txt
Engine/CMakeLists.txt
CMakeLists.txt
```

Do not modify unrelated renderer, RHI, shader, asset, scene, or UI implementation beyond what is required to keep the build working.

---

# Required Core Implementation

## Types.h

Ensure this exists:

```cpp
#pragma once

#include <cstdint>

namespace XEngine
{
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using f32 = float;
    using f64 = double;
}
```

---

## Defines.h

Create or update with platform and build config helpers:

```cpp
#pragma once

#if defined(_WIN32)
    #define XENGINE_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define XENGINE_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define XENGINE_PLATFORM_LINUX 1
#else
    #define XENGINE_PLATFORM_UNKNOWN 1
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
    #define XENGINE_DEBUG 1
#else
    #define XENGINE_RELEASE 1
#endif

#if defined(XENGINE_DEBUG)
    #define XENGINE_ENABLE_ASSERTS 1
#endif
```

Do not overcomplicate platform detection in Stage 0.

---

## NonCopyable.h

Create a simple base class:

```cpp
#pragma once

namespace XEngine
{
    class NonCopyable
    {
    public:
        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;

        NonCopyable(NonCopyable&&) = default;
        NonCopyable& operator=(NonCopyable&&) = default;
    };
}
```

---

## Result.h

Create a lightweight placeholder result type.

Do not build a full error system yet.

Suggested design:

```cpp
#pragma once

#include <string>

namespace XEngine
{
    struct Result
    {
        bool Success = true;
        std::string Message;

        static Result Ok();
        static Result Failure(std::string message);

        explicit operator bool() const;
    };
}
```

Implement inline if simple.

---

## Handle.h

Create a simple strongly typed handle template:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    constexpr u32 InvalidHandleIndex = 0xFFFFFFFFu;

    template<typename Tag>
    struct Handle
    {
        u32 Index = InvalidHandleIndex;
        u32 Generation = 0;

        bool IsValid() const
        {
            return Index != InvalidHandleIndex;
        }

        explicit operator bool() const
        {
            return IsValid();
        }

        friend bool operator==(const Handle& lhs, const Handle& rhs)
        {
            return lhs.Index == rhs.Index && lhs.Generation == rhs.Generation;
        }

        friend bool operator!=(const Handle& lhs, const Handle& rhs)
        {
            return !(lhs == rhs);
        }
    };
}
```

---

# Required Logging Implementation

## Log.h

Public header must not expose spdlog.

Create this API:

```cpp
#pragma once

#include <string_view>

namespace XEngine
{
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    class Log
    {
    public:
        static void Initialize();
        static void Shutdown();

        static void SetLevel(LogLevel level);

        static void Trace(std::string_view message);
        static void Debug(std::string_view message);
        static void Info(std::string_view message);
        static void Warn(std::string_view message);
        static void Error(std::string_view message);
        static void Critical(std::string_view message);
    };
}

#define XENGINE_LOG_TRACE(message) ::XEngine::Log::Trace(message)
#define XENGINE_LOG_DEBUG(message) ::XEngine::Log::Debug(message)
#define XENGINE_LOG_INFO(message) ::XEngine::Log::Info(message)
#define XENGINE_LOG_WARN(message) ::XEngine::Log::Warn(message)
#define XENGINE_LOG_ERROR(message) ::XEngine::Log::Error(message)
#define XENGINE_LOG_CRITICAL(message) ::XEngine::Log::Critical(message)
```

For Stage 0, do not implement variadic formatting macros yet.

This is allowed:

```cpp
XENGINE_LOG_INFO("Engine initialized");
```

This is not required yet:

```cpp
XENGINE_LOG_INFO("Engine initialized: {}", appName);
```

---

## Log.cpp

Implement with spdlog.

Requirements:

```text
Use stdout color sink.
Set logger name to "XEngine".
Set a readable pattern.
Flush on warn or error.
Initialize must be safe to call once.
Shutdown should reset logger and call spdlog::shutdown().
```

Suggested pattern:

```text
[%T] [%n] [%^%l%$] %v
```

Implementation should store the logger internally in `Log.cpp`, for example:

```cpp
namespace
{
    std::shared_ptr<spdlog::logger> s_Logger;
}
```

Do not expose this pointer.

---

# Required Assert Implementation

## Assert.h

Create macro-based assert.

Public API:

```cpp
#pragma once

#include <XEngine/Core/Defines.h>

namespace XEngine
{
    void DebugBreak();
    void ReportAssertionFailure(const char* expression, const char* file, int line, const char* message);
}

#if defined(XENGINE_ENABLE_ASSERTS)
    #define XENGINE_ASSERT(expression, message)                                                         \
        do                                                                                              \
        {                                                                                               \
            if (!(expression))                                                                          \
            {                                                                                           \
                ::XEngine::ReportAssertionFailure(#expression, __FILE__, __LINE__, message);            \
                ::XEngine::DebugBreak();                                                                \
            }                                                                                           \
        } while (false)
#else
    #define XENGINE_ASSERT(expression, message) ((void)0)
#endif
```

---

## Assert.cpp

Implement:

```text
ReportAssertionFailure should log:
- expression
- file
- line
- message

DebugBreak should use:
- __debugbreak() on MSVC
- __builtin_trap() on Clang/GCC
- std::abort() fallback
```

`Assert.cpp` may include `<XEngine/Logging/Log.h>`.

---

# Required Time Implementation

## Time.h

Create:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

#include <chrono>

namespace XEngine
{
    class Time
    {
    public:
        void Reset();
        void Tick();

        f32 GetDeltaTime() const;
        f32 GetTotalTime() const;
        u64 GetFrameIndex() const;

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point m_StartTime {};
        Clock::time_point m_PreviousTime {};

        f32 m_DeltaTime = 0.0f;
        f32 m_TotalTime = 0.0f;
        u64 m_FrameIndex = 0;
    };
}
```

## Time.cpp

Implement with `std::chrono::steady_clock`.

Requirements:

```text
Reset sets start time and previous time to now.
Tick updates delta time, total time, and frame index.
GetDeltaTime returns seconds.
GetTotalTime returns seconds.
GetFrameIndex returns current frame index.
```

---

# Required Subsystem Implementation

## Subsystem.h

Ensure this exists:

```cpp
#pragma once

namespace XEngine
{
    class ISubsystem
    {
    public:
        virtual ~ISubsystem() = default;

        virtual void OnCreate() {}
        virtual void OnDestroy() {}

        virtual void OnBeginFrame() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnEndFrame() {}
    };
}
```

---

## SubsystemManager.h

Implement simple manual registration.

Required API:

```cpp
#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    class SubsystemManager
    {
    public:
        SubsystemManager() = default;
        ~SubsystemManager();

        SubsystemManager(const SubsystemManager&) = delete;
        SubsystemManager& operator=(const SubsystemManager&) = delete;

        template<typename T, typename... Args>
        T& AddSubsystem(Args&&... args)
        {
            static_assert(std::is_base_of_v<ISubsystem, T>, "T must derive from ISubsystem");

            auto subsystem = std::make_unique<T>(std::forward<Args>(args)...);
            T* rawSubsystem = subsystem.get();

            m_SubsystemLookup[typeid(T)] = rawSubsystem;
            m_Subsystems.emplace_back(std::move(subsystem));

            return *rawSubsystem;
        }

        template<typename T>
        T* GetSubsystem()
        {
            auto it = m_SubsystemLookup.find(typeid(T));
            if (it == m_SubsystemLookup.end())
            {
                return nullptr;
            }

            return static_cast<T*>(it->second);
        }

        void CreateAll();
        void DestroyAll();

        void BeginFrame();
        void Update(float deltaTime);
        void EndFrame();

        bool IsCreated() const;

    private:
        std::vector<std::unique_ptr<ISubsystem>> m_Subsystems;
        std::unordered_map<std::type_index, ISubsystem*> m_SubsystemLookup;

        bool m_Created = false;
    };
}
```

## SubsystemManager.cpp

Implement behavior:

```text
CreateAll:
- If already created, return.
- Call OnCreate in registration order.
- Set m_Created = true.
- Log lifecycle event.

DestroyAll:
- If not created, return.
- Call OnDestroy in reverse registration order.
- Set m_Created = false.
- Clear containers only after destroying.
- Log lifecycle event.

BeginFrame:
- Call OnBeginFrame in registration order.

Update:
- Call OnUpdate(deltaTime) in registration order.

EndFrame:
- Call OnEndFrame in registration order.
```

Do not implement dependency graph yet.

Do not implement priority sorting yet.

Do not implement parallel subsystem updates yet.

---

# Required Engine Implementation

## EngineConfig.h

Create or update:

```cpp
#pragma once

#include <string>

namespace XEngine
{
    struct EngineConfig
    {
        std::string ApplicationName = "XEngine";
        bool EnableValidation = true;
        bool EnableEditor = false;
        u32 MaxFrames = 3;
    };
}
```

Remember to include `Types.h` for `u32`.

---

## Engine.h

Create or update:

```cpp
#pragma once

#include <XEngine/Engine/EngineConfig.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Engine/Time.h>

namespace XEngine
{
    class Engine
    {
    public:
        Engine();
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void Initialize(const EngineConfig& config);
        void Run();
        void Shutdown();

        void RequestShutdown();

        bool IsRunning() const;

        SubsystemManager& GetSubsystemManager();
        const Time& GetTime() const;

    private:
        EngineConfig m_Config;
        SubsystemManager m_SubsystemManager;
        Time m_Time;

        bool m_Initialized = false;
        bool m_Running = false;
    };
}
```

---

## Engine.cpp

Implement behavior:

```text
Constructor:
- Do minimal work.

Destructor:
- If initialized, call Shutdown().

Initialize:
- If already initialized, return or assert.
- Store config.
- Initialize Log.
- Log application name.
- Reset Time.
- Register no real subsystems yet.
- Call m_SubsystemManager.CreateAll().
- Set initialized true.

Run:
- Assert initialized.
- Set running true.
- Log engine started.
- Run a simple loop.
- For Stage 0, loop until RequestShutdown() or until MaxFrames is reached.
- Every frame:
  - Time.Tick()
  - SubsystemManager.BeginFrame()
  - SubsystemManager.Update(deltaTime)
  - SubsystemManager.EndFrame()
  - Log frame index
- Log engine stopped.

Shutdown:
- If not initialized, return.
- Log shutting down.
- RequestShutdown.
- Destroy all subsystems.
- Log shutdown complete.
- Shutdown Log last.
- Set initialized false.

RequestShutdown:
- Set m_Running = false.

IsRunning:
- Return m_Running.
```

Important:

```text
Log::Initialize() must happen before subsystem creation.
Log::Shutdown() must happen after subsystem destruction.
```

---

# Diagnostics Placeholder

Keep Diagnostics minimal.

## Profiler.h

Create placeholder macros:

```cpp
#pragma once

#define XENGINE_PROFILE_SCOPE(name) ((void)0)
#define XENGINE_PROFILE_FUNCTION() ((void)0)
#define XENGINE_PROFILE_FRAME() ((void)0)
```

## ScopedTimer.h

Create a small timer class or leave as a TODO stub.

Do not add Tracy yet.

## DebugMarker.h

Create placeholder macros or empty class.

Do not add GPU debug markers yet.

---

# App Updates

## Apps/Sandbox/Source/main.cpp

Should be:

```cpp
#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Sandbox";
    config.MaxFrames = 3;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
```

## Apps/EditorApp/Source/main.cpp

Should be:

```cpp
#include <XEngine/Engine/Engine.h>

int main()
{
    XEngine::EngineConfig config;
    config.ApplicationName = "XEngine Editor";
    config.EnableEditor = true;
    config.MaxFrames = 3;

    XEngine::Engine engine;
    engine.Initialize(config);
    engine.Run();
    engine.Shutdown();

    return 0;
}
```

EditorApp should not initialize ImGui yet.

---

# CMake Requirements

## Root CMakeLists.txt

Ensure project uses C++20:

```cmake
cmake_minimum_required(VERSION 3.25)

project(XEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(XENGINE_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(XENGINE_ENABLE_SDL "Enable SDL platform backend" OFF)
option(XENGINE_ENABLE_EDITOR "Build XEngine editor" ON)
option(XENGINE_ENABLE_TRACY "Enable Tracy profiler integration" OFF)

add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Apps)
```

## ThirdParty/CMakeLists.txt

Add spdlog:

```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/CMakeLists.txt")
    add_subdirectory(spdlog)
else()
    message(FATAL_ERROR "spdlog is required for Stage 0. Please place spdlog under ThirdParty/spdlog.")
endif()
```

Do not add SDL, Vulkan, Slang, ImGui, EnTT, or other dependencies yet.

## Engine/CMakeLists.txt

Ensure:

```cmake
target_link_libraries(XEngineFoundation
    PRIVATE
        spdlog::spdlog
)
```

Do not expose spdlog publicly.

Ensure `XEngineRuntime` links to `XEngineFoundation`.

Ensure `XEngineEditor` links to `XEngineRuntime`.

## Apps CMake

Ensure:

```text
XEngineSandbox links XEngineRuntime
XEngineEditorApp links XEngineEditor
```

If `XENGINE_ENABLE_EDITOR` is OFF, EditorApp can be skipped.

---

# Do Not Implement

Do not implement:

```text
SDL window creation
Input system
Vulkan RHI
RenderGraph
ShaderSystem Slang compilation
Asset loading
Scene ECS
ImGui
Tracy
FileSystem
JobSystem
```

Do not add these third-party libraries yet:

```text
SDL3
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

Only integrate spdlog in this stage.

---

# Acceptance Criteria

The stage is complete when:

```text
1. Project configures with CMake.
2. Project builds successfully.
3. XEngineSandbox runs.
4. XEngineEditorApp runs if editor build is enabled.
5. Logs are printed through spdlog.
6. spdlog is only included in Log.cpp.
7. XEngine public headers do not expose spdlog types.
8. Engine owns SubsystemManager.
9. SubsystemManager creates subsystems in registration order.
10. SubsystemManager destroys subsystems in reverse registration order.
11. Engine loop calculates delta time.
12. Engine loop calls BeginFrame / Update / EndFrame.
13. Engine shuts down cleanly.
14. No SDL, Vulkan, Slang, ImGui, or RenderGraph implementation is added.
```

---

# Final Task

Implement Stage 0 now.

Do not ask for confirmation.

Keep the implementation minimal, clean, and architecture-focused.

Where future systems are not implemented yet, leave clear TODO comments.


