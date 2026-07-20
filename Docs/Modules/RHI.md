# RHI

## 1. Module Purpose

RHI is the backend-agnostic GPU abstraction layer. It exposes a small public
surface (Device, CommandList, ResourceFactory, UploadManager, Resources,
Types, ClipSpace) and ships a single concrete Vulkan implementation in a
private backend directory.

The renderer and Editor consume only the public surface. Vulkan handle
escapes are confined to a narrow interop header (`VulkanNativeContext`)
intended exclusively for the editor overlay path.

## 2. Responsibilities

- Define RHI types (formats, usages, binding kinds, capability struct).
- Provide the `RHIDevice` abstract API (BeginFrame, EndFrame, idle wait,
  Resize, swapchain clear, capability query, native interop).
- Define `RHICommandList` for graphics command recording.
- Define `RHIResourceFactory` (NVI) for buffer / texture / view / sampler /
  shader / bind-group / bind-group-layout / pipeline creation.
- Define `RHIUploadManager` (documented as blocking, single-threaded).
- Define resource handles (`RHITexture`, `RHITextureView`, `RHIBuffer`,
  `RHIShader`, `RHIPipeline`, `RHIBindGroup`, `RHIBindGroupLayout`,
  `RHISampler`).

## 3. Non-Responsibilities

- Does not know about Renderer, Scene, Asset, Editor gameplay concepts.
- Does not implement Renderer passes or scene extraction.
- Does not interpret frame data; pass-binding decisions live with the
  Renderer.

## 4. Public API Surface

`Engine/Source/Runtime/RHI/Public/XEngine/RHI/`:

- `RHI.h` - umbrella include.
- `RHIDevice.h`, `RHIResource.h`, `RHIResourceFactory.h`, `RHICommandList.h`,
  `RHIUploadManager.h`, `RHISystem.h`, `RHITypes.h`, `RHIQueue.h`,
  `RHISwapchain.h`, `RHIClipSpace.h`, `RHIUtils.h`.
- `Resources/RHITexture.h`, `Resources/RHITextureView.h`, `Resources/RHIBuffer.h`,
  `Resources/RHIShader.h`, `Resources/RHIPipeline.h`,
  `Resources/RHIBindGroup.h`, `Resources/RHISampler.h`.
- `Native/VulkanNativeContext.h` - opaque handle bridge for the editor
  overlay.

Private:

- `Private/RHIDevice.cpp`, `Private/RHIResourceFactory.cpp`,
  `Private/RHICommandList.cpp`, `Private/RHIResource.cpp`,
  `Private/RHISystem.cpp`, `Private/RHIUtils.cpp`.
- `Private/Resources/...` - backend-neutral cpp shims.
- `Private/Vulkan/...` - Vulkan implementation.
- `Private/D3D12/`, `Private/Metal/` - empty placeholders.

## 5. Dependencies

### Depends on

- `XEngineFoundation` (log, asserts).
- `XEngineCoreRuntime` (include path).
- `XEnginePlatform` (PUBLIC - needs `NativeWindowHandle`).
- `XEngineShader` (PUBLIC - shader compilation lives there; the RHI just
  consumes the compiled bytecode).
- `volk::volk` (PUBLIC when Vulkan is enabled).
- `Vulkan` include dirs (PUBLIC when enabled).
- `VulkanMemoryAllocator/include` (PRIVATE).
- `SDL3` (PRIVATE in `VulkanSurface`).

### Used by

- `Runtime/Renderer` (PUBLIC) - heavy consumer through its resource managers.
- `Engine/Source/Editor` (PUBLIC for the Vulkan overlay path).
- `Apps/Sandbox`, `Apps/EditorApp` (PUBLIC).

## 6. Ownership and Lifetime

- `RHISystem` owns exactly one `RHIDevice*` (currently always `VulkanDevice`).
- `RHIDevice` owns the per-backend state (Vulkan instance, queues,
  swapchain, frame resources, descriptor pool, allocator).
- `RHIResourceFactory` and `RHIUploadManager` (per device) are owned by
  the device and exposed by reference.
- `RHIResource` subclasses only know their owning `RHIDevice&`; derived
  backends cast back to the concrete type.

## 7. Runtime Flow

- `RHISystem::OnCreate` creates the device (`RHISystem.cpp:28-79`).
- Each frame:
  1. `RHIDevice::BeginFrame` returns an `RHICommandList*` (lines
     `RHI/Private/Vulkan/VulkanDevice.cpp:285-362`).
  2. Renderer records commands via the command list (pipeline bind,
     descriptor bind, push constants, draw, transition, etc.).
  3. `RHIDevice::EndFrame` submits and presents (lines
     `RHI/Private/Vulkan/VulkanDevice.cpp:414-514`).
- `RHIDevice::RenderVulkanOverlay` exposes an ImGui-style callback path
  used by the editor.

## 8. Important Invariants

- All `RHITexture`, `RHITextureView`, `RHIBuffer`, `RHIShader`,
  `RHIPipeline`, `RHIBindGroup`, `RHIBindGroupLayout`, `RHISampler`
  resources are kept alive by their owning `shared_ptr`; backends must not
  transfer ownership or release them externally.
- `RHICommandList*` is only valid inside the active frame; do not
  retain across frames.
- `RHIUploadManager::UploadTexture` and `UploadBuffer` are blocking and
  complete before returning - they are not safe to call from inside
  recorded commands.
- Validation layer activation is gated by `EngineConfig::EnableValidation`
  and the runtime presence of `VK_LAYER_KHRONOS_validation`.
- Vulkan handles are **never** exposed through public types except via
  the opaque `VulkanNativeContext` bridge.

## 9. Main Classes and Collaborators

- `RHIResource` (base).
- `RHIDevice`, `RHICommandList`, `RHIResourceFactory`, `RHIUploadManager`.
- `RHITexture`, `RHITextureView`, `RHIBuffer`, `RHIShader`, `RHIPipeline`.
- `RHIBindGroup`, `RHIBindGroupLayout`, `RHISampler`.
- `RHICapabilities` (texture dimension, push-constant size, descriptor
  sets, dynamic rendering, anisotropy).

## 10. Design Rationale

- NVI factory `RHIResourceFactory` keeps the public validation hot path
  in `RHIResourceFactory.cpp` rather than scattered through every backend.
- `RHIResource` holds a reference to its device; backends upcast safely
  via `VulkanCheckedCast`.
- Headers carry only backend-agnostic types so code generation, MSVC
  IntelliSense, and ABI stay clean.
- The `VulkanNativeContext` is the only public escape for the editor
  overlay; using it as a regular RHI resource is discouraged.

### Alternatives considered

- A flat C-style API. Rejected: object lifetime handling is messy.
- A descriptor pool allocator per pass. Deferred to a future stage; the
  single device-owned pool is sufficient today (a TODO is marked).

### Trade-offs

- Vulkan-only ships today. D3D12 / Metal placeholders exist for future
  implementation but are not exercised.
- Single global descriptor pool is finite; a future arena-based
  allocator will be needed once per-material binding count grows.

## 11. Failure Modes and Debugging

- Validation errors from Vulkan are forwarded via `VulkanInstance`'s debug
  messenger to `XENGINE_LOG_*`.
- `VulkanDevice::Initialize` returns false on instance creation failure;
  `RHISystem::OnCreate` logs the message and the engine refuses to start.
- Pipeline layout/push-constant size mismatches show up as a guard
  message in `VulkanPipeline.cpp` rather than an unrecoverable crash.

## 12. Current Limitations

- Single in-flight frame in the Vulkan backend despite the renderer's
  three-frame ring.
- `RHISwapchain` is a minimal interface (no current explicit API; the
  implementation lives inside VulkanDevice).
- `RHIQueue` is minimal; queue creation is not yet exposed as a public
  type.
- D3D12 and Metal backends are placeholders.

## 13. Source References

- `Engine/Source/Runtime/RHI/Public/XEngine/RHI/*.h`
- `Engine/Source/Runtime/RHI/Private/*.cpp`
- `Engine/Source/Runtime/RHI/Private/Resources/*.cpp`
- `Engine/Source/Runtime/RHI/Private/Vulkan/*.h`
- `Engine/Source/Runtime/RHI/Private/Vulkan/*.cpp`
- `Engine/Source/Runtime/RHI/CMakeLists.txt`

## 14. Future Work

- Implement D3D12 / Metal backends behind the same public surface.
- Replace the single descriptor pool with a per-frame or arena allocator.
- Expose queue creation in the public API so async compute can be
  introduced.
