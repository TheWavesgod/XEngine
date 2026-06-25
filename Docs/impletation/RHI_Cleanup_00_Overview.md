# RHI Cleanup Overview

## Why this cleanup exists

The current RHI was built around `RHIDevice` as a "do everything" handle:

- Every resource type (`Shader`, `Buffer`, `Texture`, `Sampler`, `BindGroupLayout`,
  `BindGroup`, `GraphicsPipeline`) is created directly through a virtual
  `RHIDevice::CreateXXX(...)` call.
- CPU→GPU texture upload is performed **inside** `RHIDevice::CreateTexture` via
  `ImmediateSubmit` (a one-shot command buffer + `vkQueueWaitIdle`).
- `RHITexture` is a single object that owns both the image resource **and** one
  full-coverage image view. The only way for code outside the RHI to obtain a
  usable view is `RHITexture::GetNativeImageView(RHIBackend)` which returns a
  raw `void*`.
- `RHIRenderOutputDesc::ColorTarget` / `DepthTarget` are `RHITexture*`, not
  `RHITextureView*`, so an array slice (e.g. one cascade layer) cannot be
  addressed at the RHI level.
- Vulkan backend code uses `dynamic_cast<VulkanTexture*>(...)` etc. in
  command-list, descriptor-set, and pipeline construction paths.
- `RHIDevice` itself owns the global descriptor pool and the depth texture.

This blocks the next-stage work:

- **Stage 9 CSM** needs to render into **one layer** of a `Texture2DArray`
  depth target while **sampling all layers** for the lighting pass. That is
  impossible with the current "one view per texture" design.
- **RenderGraph V1** needs resource aliases, transient allocator, and per-pass
  view selection. A monolithic `RHIDevice` makes that painful.
- **PostProcess / TAA** chains need to ping-pong between two images, which
  requires distinct views on the same texture (read vs write).

The goal of this cleanup is to **separate resource creation from rendering
control**, **separate texture resource from texture view**, and **move uploads
to a dedicated service**, while keeping every existing Renderer feature working
through the migration.

## Final target architecture

```text
RHIResource (base, owner device, backend query)
  ├─ RHIBuffer
  ├─ RHITexture               (resource only, no view)
  ├─ RHITextureView           (mip range, layer range, aspect, usage)
  ├─ RHISampler
  ├─ RHIShader
  ├─ RHIBindGroupLayout
  ├─ RHIBindGroup             (binds texture VIEWs + samplers + buffers)
  └─ RHIPipeline

RHIDevice                     (backend root, capabilities, queues, frame lifecycle)
  ├─ RHIResourceFactory&      (create Buffer / Texture / TextureView / Sampler / Shader / BindGroupLayout / BindGroup / Pipeline)
  ├─ RHIUploadManager&        (UploadBuffer / UploadTexture / FlushUploads)
  ├─ RHICapabilities&         (max textures, max layers, depth-format list, ...)
  └─ BeginFrame / EndFrame / WaitIdle / GetSwapchainFormat

VulkanResourceFactory         (only implements native creation)
VulkanUploadManager V0        (blocking, uses ImmediateSubmit helper)
VulkanCheckedCast<...>        (debug-asserts + static_cast helper)
```

Resource / view split (CSM example):

```text
RHITexture* shadowTexture
  - VkImage, Texture2DArray, D32Float, 4 layers

RHITextureView* wholeArraySampledView      (all layers, sampled, full mip)
RHITextureView* layer0DepthAttachmentView  (layer 0 only, depth attachment)
RHITextureView* layer1DepthAttachmentView  (layer 1 only, depth attachment)
...
```

## Stage list and dependencies

The 8 stages are small, sequential, and build on each other:

```text
Stage 1  RHIResource Base + Owner Device          ─┐
Stage 2  RHITextureView                            │  Resource / view split
Stage 3  RHIResourceFactory                        │  Move CreateXXX off RHIDevice
Stage 4  RHIUploadManager                          │  Move upload off RHIDevice
Stage 5  View-based RenderPass + BindGroup         │  CSM-ready attachments / bindings
Stage 6  Depth-only Pipeline                       │  ShadowDepthPass-ready pipeline
Stage 7  Capabilities, Format Utils, Validation,   │  Diagnostics + safety
         Debug Names, Deferred Deletion            │
Stage 8  Migration Checklist                       │  Switch Renderer to new APIs
       ──────────────────────────────────────────── ┘
```

Each stage must compile and run the existing Renderer + Sandbox + Editor scenes
without behavioural change before the next stage begins.

Stage 5 depends on Stage 2 (view abstraction) and Stage 3 (factory), so it
must come after both. Stage 6 is independent of Stage 5. Stage 7 can be split
across the whole timeline if desired. Stage 8 is the final migration
sweep — it does **not** require a single big-bang commit.

## Global design rules

These rules are enforced across every stage below:

1. **No backend parameter in resource descriptors.** `RHITextureDesc`,
   `RHITextureViewDesc`, `RHIBufferDesc`, etc. do **not** carry `RHIBackend`.
   The factory that creates the resource already belongs to a backend device.
2. **No widespread `dynamic_cast`.** Backend helpers use a `CheckedVulkanCast`
   helper that asserts `GetOwnerDevice()` matches and uses `static_cast` in
   release. The single editor-native overlay path (`ImGuiVulkanBackend`) is the
   one place where a `static_cast<VkImageView>(...)` from a stored handle is
   acceptable because ImGui-Vulkan is intrinsically a Vulkan backend.
3. **Resource ≠ View.** Every `RHITexture` exposes zero or more
   `RHITextureView*` it owns; consumers always bind views, never textures.
4. **Factory centralises validation.** `RHIResourceFactory::CreateX` runs
   `validate + normalize + capability check` and only then calls the backend
   `CreateXImpl`. Backends do not duplicate validation logic.
5. **UploadManager knows nothing about assets.** No `TextureAsset`, no
   `stb_image`, no glTF, no mesh data. Bytes only.
6. **No bindless yet.** Keep descriptor-set / bind-group model. Reserve
   capability fields, do not implement indexing / UAB / runtime arrays.
7. **No full resource-state tracker.** Only minimal layout transitions needed
   by the existing code path. Full tracker belongs to a later RenderGraph
   stage.
8. **No RHI changes that force a Renderer-wide rewrite in one commit.**
   Old APIs continue to work as transitional wrappers until Stage 8 explicitly
   removes them.

## Global do-not-do list

- Do not implement bindless binding, descriptor indexing, UAB, runtime
  descriptor arrays.
- Do not implement a full automatic resource state / barrier tracker.
- Do not introduce a transient resource allocator or a render-graph allocator.
- Do not introduce async compute or async transfer queues.
- Do not introduce a mega descriptor heap allocator.
- Do not introduce a handle-based RHI rewrite (no `RHITextureHandle` u32 yet).
- Do not introduce runtime backend switching.
- Do not introduce ray-tracing APIs, GPUScene, or any CSM algorithm here.
- Do not modify `Editor/Private/ImGui/ImGuiVulkanBackend.cpp` to bind textures
  via views in this stage. The view-based ImGui path is a Stage 8 task.

## How this supports Stage 9 CSM

After this cleanup, the Stage 9 ShadowDepthPass can:

1. Use `RHIDevice::GetResourceFactory().CreateTexture(...)` to allocate the
   `Texture2DArray` shadow texture with `ArrayLayers = NumCascades`.
2. Use the same factory to allocate `N` per-layer `RHITextureView`s with
   `BaseArrayLayer = i, LayerCount = 1, Aspect = Depth`.
3. Bind one layer view as depth attachment for each cascade draw, while
   sampling the whole-array view for the lighting pass.
4. Use `RHIDevice::GetUploadManager().UploadTexture(...)` only for the (very
   small) cascade-bound CPU data, if any.
5. Construct a depth-only pipeline using the new
   `RHIGraphicsPipelineDesc::ColorAttachmentCount = 0` mode from Stage 6.

Without Stages 1–6, none of the above is expressible.

## How this prepares future RenderGraph / PostProcess / TAA

- A `RHITexture` with multiple views lets RenderGraph pass-construction pick
  whichever view each pass needs without re-allocating GPU memory.
- `RHIResourceFactory` becomes the single chokepoint for "create / reuse /
  alias" decisions in a future transient allocator.
- `RHIUploadManager` becomes the single chokepoint for staging-buffer reuse
  and (later) async upload.
- `RHICapabilities` exposes per-backend limits (max sampled image layers,
  alignment, supported depth formats) that RenderGraph / PostProcess / TAA
  will read instead of hard-coding numbers.

## Reference files for the audit

The current code in `Engine/Source/Runtime/RHI/` was audited as the basis for
this plan. Key files inspected:

```text
Public/XEngine/RHI/RHIDevice.h
Public/XEngine/RHI/Resources/RHITexture.h
Public/XEngine/RHI/Resources/RHIBuffer.h
Public/XEngine/RHI/Resources/RHIShader.h
Public/XEngine/RHI/Resources/RHIPipeline.h
Public/XEngine/RHI/Resources/RHISampler.h
Public/XEngine/RHI/Resources/RHIBindGroup.h
Public/XEngine/RHI/RHICommandList.h
Public/XEngine/RHI/RHITypes.h
Public/XEngine/RHI/RHIClipSpace.h
Public/XEngine/RHI/RHISystem.h
Public/XEngine/RHI/RHISwapchain.h
Public/XEngine/RHI/RHIQueue.h
Public/XEngine/RHI/Native/VulkanNativeContext.h

Private/RHIDevice.cpp
Private/RHICommandList.cpp
Private/RHISystem.cpp

Private/Vulkan/VulkanDevice.h
Private/Vulkan/VulkanDevice.cpp
Private/Vulkan/VulkanTexture.h
Private/Vulkan/VulkanTexture.cpp
Private/Vulkan/VulkanBuffer.h
Private/Vulkan/VulkanBuffer.cpp
Private/Vulkan/VulkanShader.h
Private/Vulkan/VulkanPipeline.h
Private/Vulkan/VulkanPipeline.cpp
Private/Vulkan/VulkanSampler.h
Private/Vulkan/VulkanSampler.cpp
Private/Vulkan/VulkanDescriptor.h
Private/Vulkan/VulkanDescriptor.cpp
Private/Vulkan/VulkanCommandList.h
Private/Vulkan/VulkanCommandList.cpp
Private/Vulkan/VulkanAllocator.h
Private/Vulkan/VulkanFrameResources.h
Private/Vulkan/VulkanUtils.h

CMakeLists.txt
```

Renderer-side callers of `m_Device->CreateXXX(...)` that will eventually move
to the factory + upload manager:

```text
Engine/Source/Runtime/Renderer/Private/Resources/RenderTextureManager.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderMeshManager.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderLibrary.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderPipelineStateCache.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderMaterialSystem.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp
Engine/Source/Runtime/Renderer/Private/Mesh/PrimitiveMeshes.cpp
```

The single Editor consumer that currently depends on
`RHITexture::GetNativeImageView`:

```text
Engine/Source/Editor/Private/ImGui/ImGuiVulkanBackend.cpp   (Stage 8 migration)
```

The Stage 9 consumer that already has TODO markers for `RHITextureView`:

```text
Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h
```

## Stage file index and per-stage content

Each stage file is a self-contained implementation guide. The Detailed
Code Plan section of every stage contains concrete before/after code
blocks, full new file contents, and exact diff anchors.

| File                                                 | What it adds                                                                                                                                                                                                                                                                                                                                                                                                                          |
| ---------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| [Stage 1](RHI_Cleanup_01_ResourceBase_OwnerDevice.md) | `RHIResource` base + `RHIUtils`, `VulkanCheckedCast`, owner-device pattern for every `VulkanX` class, 12 `dynamic_cast` sites replaced, `VulkanDevice::GetHandle()`.                                                                                                                                                                                                                                                                  |
| [Stage 2](RHI_Cleanup_02_TextureView.md)              | `RHITextureView` with `RHITextureViewDesc` / `RHITextureAspect` / `RHITextureViewUsageFlags`, `VulkanTextureView`, per-texture `m_DefaultView`, `CreateTextureView` factory.                                                                                                                                                                                                                                                          |
| [Stage 3](RHI_Cleanup_03_ResourceFactory.md)          | `RHIResourceFactory` NVI base, `VulkanResourceFactory` with verbatim creation bodies, validation wrappers, factory lifetime managed by `VulkanDevice`.                                                                                                                                                                                                                                                                                |
| [Stage 4](RHI_Cleanup_04_UploadManager.md)            | `RHITextureSubresourceRange`, `RHIUploadManager` abstract base, `VulkanUploadManager` V0 with staging-buffer pool, `CreateTextureImpl` no longer inlines upload.                                                                                                                                                                                                                                                                     |
| [Stage 5](RHI_Cleanup_05_ViewBasedRenderPass_And_BindGroup.md) | `RHIRenderOutputDesc::ColorTargetView/DepthTargetView`, `RHIBindingResource::TextureView`, `RHITexture::GetOrCreateWholeArraySampledView` / `GetOrCreateLayerDepthView` / `GetOrCreateSubresourceView` with detail cache, 12+ Renderer passes migrated, `ShadowResourceCache.h` view fields filled in.                                                                                                                                  |
| [Stage 6](RHI_Cleanup_06_DepthOnlyPipeline.md)       | `RHIGraphicsPipelineDesc::ColorAttachmentCount = 0` with `MakeDepthOnly` helper, Vulkan `colorAttachmentCount = 0` + `pColorAttachmentFormats = nullptr` + `colorBlend.attachmentCount = 0` + `depthBiasEnable`, `RenderPassKind::ShadowDepth` pipeline construction in `RenderPipelineStateCache`.                                                                                                                                  |
| [Stage 7](RHI_Cleanup_07_Capabilities_Validation_DebugNames.md) | `RHICapabilities` struct populated from `VkPhysicalDeviceProperties`, `RHIUtils` format helpers (`IsDepth` / `IsStencil` / `IsSrgb` / `GetBytesPerPixel` / `GetMaxMipLevels` / `GetDefaultAspect`), capability-aware factory validation, `VulkanDebugName` helper wired into every `VulkanX` constructor, `RHIDeferredDeleter` placeholder drained at `BeginFrame`, anisotropy wired through `caps.MaxSamplerAnisotropy`.        |
| [Stage 8](RHI_Cleanup_08_Migration_Checklist.md)      | Migration of every Renderer caller (8 resource managers, 9 passes, editor ImGui), removal of `RHITexture::GetNativeImageView`, removal of `RHIRenderOutputDesc::ColorTexture / DepthTexture`, removal of `RHIDevice::CreateX` virtuals (with `[[deprecated]]` window), restricted `RHITextureView::GetNativeView`.                                                                                                                       |

## After completing all 8 stages

The RHI surface collapses to:

```text
RHIDevice
  ├─ GetResourceFactory()         → RHIResourceFactory
  ├─ GetUploadManager()           → RHIUploadManager
  ├─ GetCapabilities()            → const RHICapabilities&
  └─ GetDeferredDeleter()         → RHIDeferredDeleter&

RHIResourceFactory
  ├─ CreateBuffer
  ├─ CreateTexture
  ├─ CreateTextureView
  ├─ CreateSampler
  ├─ CreateShader
  ├─ CreateBindGroupLayout
  ├─ CreateBindGroup
  └─ CreateGraphicsPipeline

RHIUploadManager
  ├─ UploadBuffer
  ├─ UploadTexture
  └─ FlushUploads (drained at BeginFrame)

RHIResource (base)
  ├─ GetOwnerDevice()             → RHIDevice&
  └─ (virtual dtor)

RHITexture : RHIResource
  ├─ GetDefaultView()             → RHITextureView*
  ├─ GetOrCreateWholeArraySampledView()
  ├─ GetOrCreateLayerDepthView(layer)
  └─ GetOrCreateSubresourceView(...)

RHITextureView : RHIResource
  └─ GetNativeView(RHIBackend)    (backend-private override)

RHIBindingResource
  └─ TextureView (not Texture)

RHIRenderOutputDesc
  └─ ColorTargetView / DepthTargetView (not ColorTexture / DepthTexture)

RHIGraphicsPipelineDesc
  ├─ ColorAttachmentCount         (0 = depth-only)
  └─ MakeDepthOnly(...)           (CSM helper)
```

Stage 9 (CSM) is then a Renderer-side implementation effort only; no
RHI changes are required to support the algorithm.