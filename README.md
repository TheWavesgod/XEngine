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
