# XEngine

XEngine is a renderer-first learning engine.

V0.1 focuses on:
- Clean engine architecture
- C++20
- Public/Private module layout
- SDL3 platform backend placeholder
- RHI abstraction
- Vulkan-first backend placeholder
- Slang-first shader system placeholder
- RenderGraph-ready Renderer structure
- Scene / Asset / Editor placeholders

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
