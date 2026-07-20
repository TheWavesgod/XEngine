# Vulkan Backend

## 1. Module Purpose

The Vulkan backend implements the public RHI surface against Vulkan 1.3 with
`VK_KHR_dynamic_rendering` and `VK_KHR_shader_draw_parameters`. It owns the
Vulkan instance, surface, logical device, queues, allocator, descriptor
pool, swapchain, frame resources, command list, resource factory, and
upload manager.

All Vulkan-native types stay inside `Engine/Source/Runtime/RHI/Private/Vulkan/`.
The single public exception is `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Native/VulkanNativeContext.h`,
which exposes selected handles as opaque `std::uintptr_t` fields so the
editor overlay can do Vulkan drawing without including Vulkan headers.

## 2. Responsibilities

- Implement every `RHIDevice`, `RHICommandList`, `RHIResource*`,
  `RHIResourceFactory::*Impl`, and `RHIUploadManager` virtual.
- Manage Vulkan-instance lifetime and validation layer detection.
- Build the swapchain, depth target, frame resources, and command pool.
- Translate RHI descriptors into `Vk*` types and validate them.
- Provide the editor overlay path through
  `RHIDevice::RenderVulkanOverlay(callback)`.

## 3. Non-Responsibilities

- Does not own gameplay or rendering data.
- Does not call into Vulkan without going through `VulkanCheckedCast`
  for resource downcasts (defensive + faster than `dynamic_cast`).
- Does not assume any specific Vulkan version beyond 1.3; core 1.3
  features are used (dynamic rendering, shader draw parameters).

## 4. File Inventory

`Engine/Source/Runtime/RHI/Private/Vulkan/`:

- `VulkanDevice.{h,cpp}` - lifecycle, sync, RenderVulkanOverlay.
- `VulkanInstance.{h,cpp}` - VkInstance, validation layer, debug
  messenger.
- `VulkanSurface.{h,cpp}` - VkSurfaceKHR; reads
  `SDL_Vulkan_GetInstanceExtensions`.
- `VulkanSwapchain.{h,cpp}` - VkSwapchainKHR + per-image views.
- `VulkanFrameResources.{h,cpp}` - single command pool/buffer, single
  image-available semaphore, per-image render-finished semaphores, single
  in-flight fence.
- `VulkanAllocator.{h,cpp}` - VMA wrapper.
- `VulkanQueue.{h,cpp}` - VkQueue wrapper with family index.
- `VulkanCommandList.{h,cpp}` - command recording through `vkCmd*`.
- `VulkanPipeline.{h,cpp}` - graphics pipeline + pipeline layout.
- `VulkanShader.{h,cpp}` - VkShaderModule.
- `VulkanTexture.{h,cpp}` - VkImage + VMA backing.
- `VulkanTextureView.{h,cpp}` - VkImageView.
- `VulkanBuffer.{h,cpp}` - VkBuffer + VMA backing.
- `VulkanSampler.{h,cpp}` - VkSampler.
- `VulkanDescriptor.{h,cpp}` - VkDescriptorSetLayout + VkDescriptorSet.
- `VulkanResourceFactory.{h,cpp}` - factory implementations.
- `VulkanUploadManager.{h,cpp}` - blocking staging-buffer upload path.
- `VulkanUtils.{h,cpp}` - RHI <-> Vulkan conversion utilities and
  `XENGINE_VK_CHECK` macro.
- `VulkanCheckedCast.h` - checked downcast helper.

## 5. Public Escape

The only public Vulkan-touching surface is:

```cpp
struct VulkanNativeContext {
    std::uintptr_t Instance;
    std::uintptr_t PhysicalDevice;
    std::uintptr_t Device;
    std::uintptr_t GraphicsQueue;
    u32            GraphicsQueueFamilyIndex;
    u32            MinImageCount;
    u32            ImageCount;
    u32            ColorFormat;
    u32            DepthFormat;
};

struct VulkanNativeTextureBinding {
    std::uintptr_t Sampler;
    std::uintptr_t ImageView;
};
```

These are produced by `VulkanDevice::GetVulkanNativeContext` and
`GetVulkanNativeTextureBinding`. They are consumed by the editor's
overlay path (`RHIDevice::RenderVulkanOverlay` takes a callback whose
parameter is `RHINativeCommandBuffer = std::uintptr_t`).

## 6. Validation Layer

`VulkanInstance::Create` (lines `VulkanInstance.cpp:14-118`):

- `vkEnumerateInstanceLayerProperties` for lookup.
- If `VK_LAYER_KHRONOS_validation` is present, the layer is added to
  `VkInstanceCreateInfo::ppEnabledLayerNames`; otherwise a warning is
  logged and instance creation continues without it.
- A `VkDebugUtilsMessengerEXT` is created and stored; the callback
  routes Vulkan log severity to `XENGINE_LOG_*`.

The engine's `EngineConfig::EnableValidation` defaults to `true`. The
RHISystem sets `VulkanDeviceCreateInfo::EnableValidation` from the
config; if absent, the field defaults to `true` as well.

## 7. Frame Lifecycle

```text
BeginFrame:
  wait in-flight fence (UINT64_MAX)
  acquire next image using image-available semaphore
  reset fence, reset command pool, begin command buffer with
    ONE_TIME_SUBMIT_BIT
  hand command buffer + swapchain image/view to VulkanCommandList

Frame commands:

EndFrame:
  end dynamic rendering if active
  pipeline barrier -> PRESENT_SRC_KHR
  end command buffer
  queue submit (wait imageAvailable, signal renderFinished[index] + fence)
  queue present (wait renderFinished[index])
```

See `Private/Vulkan/VulkanDevice.cpp:285-514` and
`Private/Vulkan/VulkanFrameResources.{h,cpp}` for the exact calls.

## 8. Native Handle Encapsulation Pattern

`VulkanResourceFactory` produces `shared_ptr<RHIResource>` instances whose
underlying classes (`VulkanBuffer`, etc.) hold `Vk*` directly. Each derived
class implements the public interface by calling into `vk*` functions and
translating `RHITypes` -> `VkTypes` via `VulkanUtils.cpp`.

If a downstream caller (editor overlay, future async compute) needs a raw
handle, it goes through `GetVulkanNativeContext` /
`GetVulkanNativeTextureBinding`. The values are `std::uintptr_t`, not
`Vk*`; consumers are expected to reinterpret_cast back. This avoids a
header-level leak of `vulkan.h` while still letting the editor do
specialized work.

## 9. Important Invariants

- Every Vulkan resource has a clear owner. The single exception is
  `VkCommandBuffer`, owned by `VulkanFrameResources` and handed to
  `VulkanCommandList` for recording within the active frame.
- All Vulkan allocations go through `VulkanAllocator` (VMA). No raw
  `vkAllocateMemory`.
- Bind groups are allocated from a single `VkDescriptorPool`; the pool
  is sized for V0 with `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`
  but **does not allocate statically sized arrays** (`count = 1`).
- The Vulkan swapchain image count is read at startup and at every
  `RecreateSwapchain`. `VulkanFrameResources::Create` is invoked with the
  current swapchain image count.

## 10. Main Classes and Collaborators

- `VulkanDevice` (top owner).
- `VulkanInstance`, `VulkanSurface`, `VulkanSwapchain`,
  `VulkanFrameResources`, `VulkanAllocator`.
- `VulkanCommandList`, `VulkanPipeline`, `VulkanShader`.
- `VulkanTexture`, `VulkanTextureView`, `VulkanBuffer`, `VulkanSampler`.
- `VulkanDescriptor`, `VulkanResourceFactory`, `VulkanUploadManager`.
- `VulkanQueue`, `VulkanCheckedCast`, `VulkanUtils`.

## 11. Design Rationale

- volk-based dynamic loader avoids DLL dependencies on `vulkan-1.dll`.
- VMA centralizes memory allocation.
- `VulkanCheckedCast` replaces `dynamic_cast` with the same set of
  runtime checks at debug build time and `static_cast` in release.
- The pipeline/pipeline-layout creation flow (`VulkanPipeline.cpp`) is
  deliberately verbose to keep every state struct readable against the
  Vulkan spec; this is the highest-risk creation path.

### Alternatives considered

- Vulkan HPP (header-only C++ bindings). Rejected in this revision for
  verbosity reasons; can be evaluated when refactoring.
- A separate RenderGraph-like layout abstraction at the Vulkan layer.
  Rejected; layout stays in `RHIResourceFactory`'s validation.

### Trade-offs

- Per-frame `vkBeginCommandBuffer` + `vkEndCommandBuffer` for the single
  command buffer is the simplest possible model and matches V0.
- No GPU-driven work; everything is recorded by the CPU.

## 12. Failure Modes and Debugging

- `vkCreateInstance` failure: `VulkanDevice::Initialize` returns false;
  `RHISystem::OnCreate` logs and the engine refuses to start.
- `vkAcquireNextImageKHR` returns `VK_ERROR_OUT_OF_DATE_KHR` or
  `VK_SUBOPTIMAL_KHR`: device sets `m_ResizeRequested` and
  `BeginFrame` returns `nullptr`.
- Pipeline guard inside `VulkanPipeline.cpp` validates that
  `desc.PushConstantSize <= device.MaxPushConstantSize` and refuses to
  create the pipeline if it would exceed the spec minimum. Push-constant
  boundaries that violate this should be the responsibility of the
  caller, not the backend.

## 13. Current Limitations

- One in-flight frame in flight; expanding to true multi-frame
  in-flight would require per-frame command buffers and per-frame fences.
- D3D12 and Metal backends are placeholders.
- The global descriptor pool is fixed at startup; large descriptor
  counts will need an arena.

## 14. Source References

- `Engine/Source/Runtime/RHI/Private/Vulkan/*`
- `Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp`
- `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Native/VulkanNativeContext.h`
