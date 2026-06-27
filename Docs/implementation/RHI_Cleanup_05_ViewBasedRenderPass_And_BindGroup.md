# Stage 5 — View-Based Rendering Boundaries and Explicit View Ownership

## 1. Goal

Finish the texture/view split at rendering boundaries without introducing a
generic texture-view cache:

- render attachments are `RHITextureView*`;
- sampled-image bindings are `RHITextureView*`;
- the Vulkan backend resolves the source image through the supplied view;
- the renderer object that understands a view's meaning owns its
  `std::shared_ptr<RHITextureView>`;
- `RHITexture` remains the image resource, not a cache of arbitrary views.

The Stage 9 CSM ownership model is the reference case:

```text
ShadowResourceCache
  ├─ shared_ptr<RHITexture>                  shadow array image
  ├─ shared_ptr<RHITextureView>              whole-array sampled view
  ├─ array<shared_ptr<RHITextureView>, N>    per-layer attachment views
  └─ shared_ptr<RHISampler>

RenderShadowFrameData / RHIRenderOutputDesc / RHIBindingResource
  └─ non-owning pointers valid for the frame or owner lifetime
```

## 2. Review Decision: No General Cache on `RHITexture`

Do **not** add any of the following APIs or implementation details proposed by
the earlier draft:

```text
RHITexture::GetOrCreateSubresourceView(...)
RHITexture::GetOrCreateWholeArraySampledView()
RHITexture::GetOrCreateLayerDepthView(...)
detail::ViewCache
RHITexture_Views.h
unordered_map<ViewKey, shared_ptr<RHITextureView>>
```

Reasons:

1. A texture cannot decide whether two logically different consumers should
   share a view. That policy belongs to `RenderTextureManager`,
   `EditorViewportRenderTarget`, `ShadowResourceCache`, or a future
   RenderGraph resource registry.
2. A monotonically growing per-texture map has no eviction boundary and hides
   resource creation from the system that owns the feature.
3. Returning raw pointers from a hidden cache makes lifetime rules implicit.
4. Putting the cache in the `RHITexture` base complicates Vulkan destruction:
   every `VkImageView` must be destroyed before its `VkImage`.
5. Stage 9 already specifies explicit `shared_ptr` ownership in
   `ShadowResourceCache`; a second cache duplicates that ownership model.
6. A future RenderGraph may intern transient views using frame/resource
   lifetime information. A permanent cache in `RHITexture` would work against
   that design.

The only view left on `RHITexture` after Stage 4 is the transitional
`GetDefaultView()` created in Stage 2. Stage 5 stops adding new callers to it.
Stage 8 removes it after renderer owners hold explicit default views.

## 3. Ownership Contract

`RHIResourceFactory::CreateTextureView` returns
`std::shared_ptr<RHITextureView>`. The semantic owner stores that pointer.
Descriptors and command recording use non-owning raw pointers.

The source texture must outlive all of its views. In owner structs, declare the
texture before its views so reverse member destruction destroys views first:

```cpp
struct OwnedTextureViews
{
    std::shared_ptr<RHITexture> Texture;
    std::shared_ptr<RHITextureView> SampledView;
    std::vector<std::shared_ptr<RHITextureView>> AttachmentViews;

    void Reset()
    {
        AttachmentViews.clear();
        SampledView.reset();
        Texture.reset();
    }
};
```

Do not solve this with a strong view-to-texture reference: the Stage-2 default
view is temporarily owned by the texture, so that would form a cycle. The
explicit owner and reset order are the Stage 5 contract.

## 4. Current Baseline (after Stages 1–4)

The checked-in source is partially through this migration:

- `RHIRenderOutputDesc::ColorTarget` and `DepthTarget` are already
  `RHITextureView*`. Keep these names; adding a `View` suffix now is churn.
- `RHIBindingResource` already has `TextureView`, while
  `VulkanDescriptor.cpp` still reads the removed `Texture` field.
- `RHITexture::GetDefaultView()` still lazily creates a view in
  `VulkanTexture`.
- `VulkanCommandList.cpp` still treats render-output pointers as textures and
  calls the removed `GetImageView()` path.
- `RenderTextureManager` stores only `shared_ptr<RHITexture>`.
- `EditorViewportRenderTarget` stores color/depth textures but not their
  views.
- `ShadowResourceCache` has the intended view fields commented out.

Stage 5 is complete only when those mismatches are removed and the tree builds.

## 5. Public API Shape

### 5.1 `RHIRenderOutputDesc`

Keep the current view-only shape:

```cpp
struct RHIRenderOutputDesc
{
    RHITextureView* ColorTarget = nullptr; // null for depth-only
    RHITextureView* DepthTarget = nullptr; // may select one array layer

    RHIRect2D Viewport {};
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    bool RenderToSwapchain = true;
};
```

Do not add fallback `ColorTexture` or `DepthTexture` fields. Two ways to
describe one attachment create ambiguous precedence and allow incomplete
migration to compile.

### 5.2 `RHIBindingResource`

Keep the current field and migrate the backend:

```cpp
struct RHIBindingResource
{
    u32 Binding = 0;
    RHIBindingType Type = RHIBindingType::Unknown;

    RHITextureView* TextureView = nullptr;
    RHISampler* Sampler = nullptr;
    RHIBuffer* Buffer = nullptr;
};
```

### 5.3 View descriptors

`RHITextureViewDesc` remains a creation descriptor with a non-owning source
texture pointer. `RHIResourceFactory` normalizes `MipCount == 0` and
`ArrayLayerCount == 0` to the remaining range before the backend stores the
descriptor. Stage 7 performs the complete compatibility validation.

## 6. Renderer-Owned Default Views

### 6.1 `RenderTextureManager`

Extend `TextureRecord` and expose the sampled view by handle:

```cpp
struct TextureRecord
{
    std::string Path;
    std::shared_ptr<RHITexture> Texture;
    std::shared_ptr<RHITextureView> SampledView;
    u32 Generation = 0;
};

RHITextureView* GetTextureView(TextureHandle handle);
const RHITextureView* GetTextureView(TextureHandle handle) const;
```

After creating and uploading a texture, create exactly one semantic sampled
view:

```cpp
RHITextureViewDesc viewDesc;
viewDesc.Texture = texture.get();
viewDesc.Usage = RHITextureViewUsageFlags::Sampled;
viewDesc.ViewDimension = RHITextureViewDimension::Texture2D;
viewDesc.Aspect = RHITextureAspectFlags::Color;
viewDesc.Format = texture->GetDesc().Format;
viewDesc.BaseMipLevel = 0;
viewDesc.MipCount = texture->GetDesc().MipLevels;
viewDesc.BaseArrayLayer = 0;
viewDesc.ArrayLayerCount = 1;
viewDesc.DebugName = texture->GetDesc().DebugName;

auto view = factory.CreateTextureView(viewDesc);
if (!view)
{
    return m_MissingTexture;
}
```

Change `AddTextureRecord` to accept both owning pointers. Material creation
uses `GetTextureView(handle)` and never calls `texture->GetDefaultView()`.

### 6.2 `EditorViewportRenderTarget`

Store explicit color and depth views next to their textures:

```cpp
std::shared_ptr<RHITexture> m_ColorTexture;
std::shared_ptr<RHITextureView> m_ColorTargetView;
std::shared_ptr<RHITexture> m_DepthTexture;
std::shared_ptr<RHITextureView> m_DepthTargetView;
```

Create the views immediately after the textures. On resize/shutdown, reset
views before textures. Pass the views to `RHIRenderOutputDesc`; expose the
color view to the Stage-8 ImGui interop migration.

### 6.3 `ShadowResourceCache`

Declare the Stage 9 fields now using owning pointers:

```cpp
struct DirectionalShadowResources
{
    std::shared_ptr<RHITexture> Texture;
    std::shared_ptr<RHITextureView> SampledView;
    std::array<std::shared_ptr<RHITextureView>, MaxShadowCascades>
        LayerDepthViews {};
    std::shared_ptr<RHISampler> Sampler;

    u32 Resolution = 0;
    u32 CascadeCount = 0;
    RHIFormat Format = RHIFormat::Undefined;

    void Reset(); // views, sampler, then texture
};
```

Stage 5 only establishes ownership and types. Stage 9 creates and populates
the shadow resources.

## 7. Vulkan Backend Changes

### 7.1 Descriptor writes

In `VulkanDescriptor.cpp`, validate and cast `resource.TextureView` directly:

```cpp
if (resource.Type == RHIBindingType::CombinedImageSampler)
{
    if (resource.TextureView == nullptr || resource.Sampler == nullptr)
    {
        XENGINE_LOG_ERROR("Combined image sampler requires a view and sampler");
        return false;
    }

    auto* view = CheckedVulkanCast<VulkanTextureView>(
        resource.TextureView, device);
    auto* sampler = CheckedVulkanCast<VulkanSampler>(
        resource.Sampler, device);
    if (view == nullptr || sampler == nullptr ||
        view->GetHandle() == VK_NULL_HANDLE ||
        sampler->GetHandle() == VK_NULL_HANDLE)
    {
        return false;
    }

    imageInfo.imageView = view->GetHandle();
    imageInfo.sampler = sampler->GetHandle();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
```

Also validate that layout, view, sampler, and buffer resources all belong to
the factory's device. Do not recover a device by unchecked casting from a
resource owner.

### 7.2 Dynamic-rendering attachments

In `VulkanCommandList::BeginRenderingIfNeeded`:

1. cast `ColorTarget` / `DepthTarget` to `VulkanTextureView`;
2. obtain each source texture from `view->GetTexture()`;
3. checked-cast that source to `VulkanTexture`;
4. use the view's `VkImageView` for the attachment;
5. use the texture's `VkImage` for barriers.

Never cast an `RHITextureView*` to `VulkanTexture*`.

For swapchain output, the swapchain image/view path remains separate.

### 7.3 Stage-5 layout rule

The current Vulkan texture tracks one `VkImageLayout` for the entire image.
Therefore Stage 5 barriers must conservatively cover the **entire texture**:

```cpp
range.baseMipLevel = 0;
range.levelCount = texture.GetDesc().MipLevels;
range.baseArrayLayer = 0;
range.layerCount = texture.GetDesc().ArrayLayers;
```

The attachment view may select one CSM layer, but the barrier covers the whole
array. This makes the single-layout invariant true and supports the CSM loop:
the array transitions to depth-attachment layout, individual layer views are
rendered, then the whole array transitions to shader-read layout.

Do not transition only layer 0 while recording one layout for the entire
image. Per-subresource layout tracking is deferred to RenderGraph/resource
state work.

## 8. Implementation Order

1. Keep the view-only public descriptor shapes and fix all stale field uses.
2. Fix `VulkanDescriptor.cpp` to consume `TextureView`.
3. Fix `VulkanCommandList.cpp` to resolve image + view correctly and use
   whole-image barriers.
4. Add explicit sampled views to `RenderTextureManager`; migrate material
   bindings.
5. Add explicit attachment views to `EditorViewportRenderTarget`; migrate
   render-output construction.
6. Add owning view fields to `ShadowResourceCache` without implementing CSM.
7. Build, run validation layers, and smoke-test Editor and Sandbox.

## 9. Verification

- `rg "resource\.Texture\b|GetImageView\(" Engine/Source/Runtime/RHI` has no
  stale texture-binding/attachment paths.
- Existing material textures render through `TextureRecord::SampledView`.
- Editor offscreen color/depth attachments use their explicit views.
- A temporary 4-layer depth texture can create one array sampled view and four
  layer attachment views owned by a local test fixture.
- Vulkan validation reports no image-view, descriptor, or layout-range errors.
- Destroy/recreate the editor viewport target repeatedly; views are destroyed
  before their source images.
- No `ViewKey`, `ViewCache`, or `GetOrCreateSubresourceView` exists.

## 10. Out of Scope

- Depth-only pipeline state (Stage 6).
- Full per-subresource layout/state tracking.
- RenderGraph view interning or transient allocation.
- Bindless descriptors.
- CSM algorithms and shadow rendering.
- Native-handle API cleanup (Stage 8).
