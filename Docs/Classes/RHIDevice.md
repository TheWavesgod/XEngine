# RHIDevice

## 1. Role

Abstract `RHIDevice` is the entry point through which the renderer (and
the editor overlay path) interact with the GPU. It exposes frame
begin/end, swapchain clear, resize, capability queries, the resource
factory, the upload manager, and a Vulkan-native interop for the
editor overlay.

## 2. Source Location

- Public header: `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h`
- Generic implementation stub: `Engine/Source/Runtime/RHI/Private/RHIDevice.cpp`
- Vulkan implementation: `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.{h,cpp}`

## 3. Owned State (Declared in public header)

The interface holds no state; derived classes own everything. Public
surface includes:

```cpp
virtual RHIBackend               GetBackend() const = 0;
virtual bool                     IsValid() const = 0;
virtual RHICommandList*          BeginFrame() = 0;
virtual void                     EndFrame() = 0;
virtual void                     ClearSwapchain(const RHIColor&) = 0;
virtual void                     RequestResize(u32 width, u32 height) = 0;
virtual RHIFormat                GetSwapchainFormat() const = 0;
virtual const RHICapabilities&   GetCapabilities() const = 0;
virtual RHIResourceFactory&      GetResourceFactory() = 0;
virtual RHIUploadManager&        GetUploadManager() = 0;
virtual void                     WaitIdle() = 0;
virtual RHIClipSpaceConvention   GetClipSpaceConvention() const = 0;
virtual bool                     GetVulkanNativeContext(VulkanNativeContext&) const = 0;
virtual bool                     GetVulkanNativeTextureBinding(...) const = 0;
virtual void                     RenderVulkanOverlay(callback) = 0;
```

## 4. Borrowed Dependencies

- The Vulkan implementation borrows the SDL3 `NativeWindowHandle`
  passed through `VulkanDeviceCreateInfo::NativeWindow`.
- D3D12 / Metal implementations are placeholders.

## 5. Lifetime

`RHISystem::OnCreate` instantiates the concrete device and stores the
unique_ptr. The device is destroyed in `RHISystem::OnDestroy`.

## 6. Callers and Used By

- `RenderSystem::Render` calls `BeginFrame` / `EndFrame`, plus
  `ClearSwapchain` indirectly through `ClearPass`.
- The editor overlay path uses `RenderVulkanOverlay`.
- Shader / resource factories are reached through `GetResourceFactory`.

## 7. Main Collaborators

- `RHICommandList`, `RHIResourceFactory`, `RHIUploadManager`.
- `RHICapabilities`, `RHIClipSpaceConvention`, `RHIRenderOutputDesc`.

## 8. Runtime Sequence

See `Class/VulkanDevice.md` for the per-frame Vulkan sequence. The
abstract layer adds:

- `BeginFrame` returns a (non-owning) `RHICommandList*`.
- `EndFrame` flushes + submits + presents.
- `ClearSwapchain` records a clear at the start of the frame.
- `RequestResize` sets a deferred resize flag processed by the next
  `BeginFrame`.

## 9. Important Invariants

- `BeginFrame` and `EndFrame` must be balanced.
- `RHICommandList*` is only valid within the active frame.
- `RenderVulkanOverlay` requires an active frame; calls outside an
  active frame are no-ops.

## 10. Invalid States and Failure Modes

- Backends may return null from `BeginFrame` if the device is
  invalid or a resize is pending; the caller bails gracefully.
- All resource creation paths return `nullptr` on failure.

## 11. Threading and Synchronization Assumptions

- Main-thread only.
- The validation layer's debug callback may be called from any
  Vulkan thread.

## 12. Design Rationale

- A flat abstract surface keeps the renderer-Vulkan dependency one-
  way. The renderer never names a Vulkan type.
- The native interop surface lets the editor overlay draw to the
  swapchain without exposing Vulkan to the editor public API.

## 13. Alternatives and Trade-offs

- D3D12 / Metal placeholders could be made real once their backends
  land.
- A separate `RHISwapchain` interface (currently minimal) is
  deferred.

## 14. Extension Points

- New backends derive from `RHIDevice` and provide their own
  implementations of the public surface.
- New helpers (e.g. async compute) extend the abstract surface.

## 15. Current Limitations

- Vulkan is the only wired backend.
- Single in-flight frame.
- D3D12 / Metal implementations are placeholders.

## 16. Source References

- `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h`
- `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.{h,cpp}`
- `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Native/VulkanNativeContext.h`
