# Stage 8 — Migration Checklist

## 1. Goal

Remove the transitional APIs left behind by Stages 1–7 and migrate every
remaining caller to the new factory / upload manager / view-based APIs.

The goal is **a clean cut** that is still safe to ship: every transitional
wrapper has a clearly named replacement, every consumer is updated, and the
new API is the only one that remains.

> Current-source correction (2026-06-25): apply this checklist to the current
> checked-in baseline:
>
> - Replace `ColorTarget` / `DepthTarget` with `ColorTargetView` /
>   `DepthTargetView`.
> - Do not search for `ColorTexture` / `DepthTexture` as existing fields; they
>   are not present in the current `RHIRenderOutputDesc`.
> - Keep `std::shared_ptr` return types unless a separate ownership migration
>   has already landed.
> - Use `RHIPipeline`, not `RHIGraphicsPipeline`.
> - If Stage 2 kept `GetNativeImageView`, migrate from that spelling. If Stage 2
>   renamed it to `GetNativeView`, use that spelling consistently.

## 2. Transitional APIs That This Stage Removes

These were added in earlier stages purely as compatibility shims:

```text
RHIDevice::CreateShader                  → factory.CreateShader
RHIDevice::CreateBuffer                  → factory.CreateBuffer
RHIDevice::CreateTexture                 → factory.CreateTexture
RHIDevice::CreateSampler                 → factory.CreateSampler
RHIDevice::CreateBindGroupLayout         → factory.CreateBindGroupLayout
RHIDevice::CreateBindGroup               → factory.CreateBindGroup
RHIDevice::CreateGraphicsPipeline        → factory.CreateGraphicsPipeline

RHITexture::GetNativeImageView           → texture.GetDefaultView()->GetNativeView
RHITextureView::GetNativeView            → backend.GetNativeHandle(view)

RHIRenderOutputDesc::ColorTexture        → RHIRenderOutputDesc::ColorTargetView
RHIRenderOutputDesc::DepthTexture        → RHIRenderOutputDesc::DepthTargetView
RHIBindingResource::Texture              → RHIBindingResource::TextureView
```

The `VulkanNativeContext` and `RHIDevice::RenderVulkanOverlay` API remain
for the editor ImGui overlay path; they are not transitional, but their
only caller (`Engine/Source/Editor/Private/ImGui/ImGuiVulkanBackend.cpp`)
must migrate to view-based texture access in this stage.

## 3. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
  - remove CreateShader / CreateBuffer / CreateTexture / CreateSampler /
    CreateBindGroupLayout / CreateBindGroup / CreateGraphicsPipeline
    virtual methods (keep CreateXImpl only on the backend).

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
  - remove GetNativeImageView.

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITextureView.h
  - remove GetNativeView (or convert to backend-specific virtual that is
    called only inside the backend).

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h
  - remove RHIRenderOutputDesc::ColorTexture / DepthTexture.

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBindGroup.h
  - remove RHIBindingResource::Texture field.

Engine/Source/Runtime/Renderer/Private/Resources/RenderTextureManager.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderMeshManager.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderLibrary.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderPipelineStateCache.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderMaterialSystem.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp
Engine/Source/Runtime/Renderer/Private/Mesh/PrimitiveMeshes.cpp
  - switch every m_Device->CreateX call to
        m_Device->GetResourceFactory().CreateX(...)
  - update upload callers to m_Device->GetUploadManager().UploadX(...)

Engine/Source/Runtime/Renderer/Private/Resources/RenderMaterialSystem.cpp
  - update every RHIBindingResource to use TextureView not Texture.

Engine/Source/Runtime/Renderer/Private/Passes/*
Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp
  - update every RHIRenderOutputDesc construction to use ColorTargetView /
    DepthTargetView only.

Engine/Source/Editor/Private/ImGui/ImGuiVulkanBackend.cpp
  - replace GetNativeImageView with GetDefaultView()->GetNativeView.

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp
  - drop any remaining dynamic_cast and transitional GetNativeImageView
    paths.

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
  - remove GetNativeImageView (now lives on VulkanTextureView only).
```

## 4. Renderer Migration Checklist

For each Renderer system, perform the listed migration in a separate commit
to keep history clean. The "Before / After" code blocks below are the exact
diffs to apply.

### 4.1 `RenderTextureManager.cpp` — texture creation

**Before**:

```cpp
auto texture = m_Device->CreateTexture(
    desc,
    pixels.data(),
    pixels.size_bytes());
```

**After**:

```cpp
auto& factory = m_Device->GetResourceFactory();
auto texture = factory.CreateTexture(desc);
if (texture && !pixels.empty())
{
    m_Device->GetUploadManager().UploadTexture(
        *texture,
        pixels.data(),
        pixels.size_bytes());
}
```

For textures with no initial data, the change simplifies to:

```cpp
auto texture = m_Device->GetResourceFactory().CreateTexture(desc);
```

`GetOrCreateTextureFromAsset` is unchanged externally; its internal call
follows the same pattern.

### 4.2 `RenderMeshManager.cpp` — buffer creation

**Before**:

```cpp
auto vb = m_Device->CreateBuffer(
    vertexDesc,
    asset.Vertices.data(),
    vertexDesc.Size);
auto ib = m_Device->CreateBuffer(
    indexDesc,
    asset.Indices.data(),
    indexDesc.Size);
```

**After**:

```cpp
auto& factory = m_Device->GetResourceFactory();
auto vb = factory.CreateBuffer(
    vertexDesc,
    asset.Vertices.data(),
    vertexDesc.Size);
auto ib = factory.CreateBuffer(
    indexDesc,
    asset.Indices.data(),
    indexDesc.Size);
```

### 4.3 `RenderShaderLibrary.cpp` — shader compilation

**Before**:

```cpp
auto shader = m_Device->CreateShader(shaderDesc);
```

**After**:

```cpp
auto shader = m_Device->GetResourceFactory().CreateShader(shaderDesc);
```

### 4.4 `RenderPipelineStateCache.cpp` — pipeline creation

Already migrated in Stage 6. Stage 8 just removes the now-redundant
wrapper path:

**Before** (Stage 6 result):

```cpp
return m_Device->GetResourceFactory().CreateGraphicsPipeline(desc).release();
```

**After** (optional shorter form):

```cpp
return m_Device->GetResourceFactory().CreateGraphicsPipeline(desc).release();
```

No change in the code; the verification is that **no other callsite** in
the codebase still invokes `m_Device->CreateGraphicsPipeline(...)`. Run:

```bash
grep -rn "m_Device->CreateGraphicsPipeline" Engine/Source/Runtime/Renderer
```

The only acceptable hit is the now-removed wrapper, which compiles to
nothing.

### 4.5 `RenderMaterialSystem.cpp` — bind group layout / sampler / bind group

**Before**:

```cpp
auto layout = m_Device->CreateBindGroupLayout(layoutDesc);
auto sampler = m_Device->CreateSampler(samplerDesc);
// ...
auto bg = m_Device->CreateBindGroup(bindGroupDesc);
```

**After**:

```cpp
auto& factory = m_Device->GetResourceFactory();
auto layout   = factory.CreateBindGroupLayout(layoutDesc);
auto sampler  = factory.CreateSampler(samplerDesc);
// ...
auto bg       = factory.CreateBindGroup(bindGroupDesc);
```

For every `RHIBindingResource` in this file:

**Before**:

```cpp
RHIBindingResource resource;
resource.Binding = 0;
resource.Type = RHIBindingType::CombinedImageSampler;
resource.Texture = diffuseTexture;
resource.Sampler = defaultSampler;
```

**After**:

```cpp
RHIBindingResource resource;
resource.Binding = 0;
resource.Type = RHIBindingType::CombinedImageSampler;
resource.TextureView = diffuseTexture->GetDefaultView();
resource.Sampler = defaultSampler;
```

(Stage 5 already did this; Stage 8 verifies the build is clean.)

### 4.6 `RenderFrameResources.cpp` — same pattern as MaterialSystem

Apply the same three factory redirections and `Texture → TextureView`
substitution. Code shape is identical to 4.5.

### 4.7 `PrimitiveMeshes.cpp` — buffer creation

**Before**:

```cpp
auto vertexBuffer = device.CreateBuffer(vertexDesc, vertices.data(), vertexDesc.Size);
auto indexBuffer  = device.CreateBuffer(indexDesc,  indices.data(),  indexDesc.Size);
```

**After**:

```cpp
auto& factory = device.GetResourceFactory();
auto vertexBuffer = factory.CreateBuffer(vertexDesc, vertices.data(), vertexDesc.Size);
auto indexBuffer  = factory.CreateBuffer(indexDesc,  indices.data(),  indexDesc.Size);
```

### 4.8 Passes — view-based render output

For every pass that constructs an `RHIRenderOutputDesc`, the
Stage-5 migration already added the `*View` fields. Stage 8 removes
the convenience `ColorTexture / DepthTexture` fields. The two
representations are now collapsed to the view-only form.

**Before** (post-Stage 5):

```cpp
RHIRenderOutputDesc output;
output.ColorTargetView = m_ColorTexture->GetDefaultView();
output.ColorTexture    = m_ColorTexture;     // ← remove
output.DepthTargetView = m_DepthTexture->GetDefaultView();
output.DepthTexture    = m_DepthTexture;     // ← remove
output.Viewport = viewport;
output.RenderToSwapchain = false;
```

**After**:

```cpp
RHIRenderOutputDesc output;
output.ColorTargetView = m_ColorTexture->GetDefaultView();
output.DepthTargetView = m_DepthTexture->GetDefaultView();
output.Viewport = viewport;
output.RenderToSwapchain = false;
```

Apply to:

```text
Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/ForwardMeshPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/ForwardPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/SkyboxPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/TonemapPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/TrianglePass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/ClearPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/PresentPass.cpp
Engine/Source/Runtime/Renderer/Private/Passes/DepthPrePass.cpp
```

### 4.9 Editor ImGui backend

**Before** (`Engine/Source/Editor/Private/ImGui/ImGuiVulkanBackend.cpp:33-133`):

```cpp
VkImageView imageView = static_cast<VkImageView>(
    texture.GetNativeImageView(RHIBackend::Vulkan));
VkSampler vkSampler = static_cast<VkSampler>(
    sampler.GetNativeSampler(RHIBackend::Vulkan));
```

**After**:

```cpp
RHITextureView* view = texture.GetDefaultView();
XE_ASSERT(view != nullptr);
VkImageView imageView = static_cast<VkImageView>(view->GetNativeView(RHIBackend::Vulkan));

VkSampler vkSampler = static_cast<VkSampler>(sampler.GetNativeHandle());
```

`RHISampler::GetNativeHandle()` is added in Stage 7 (Section 5.5) and
returns a void* that the editor can `static_cast<VkSampler>`. It is the
sampler-side equivalent of `RHITextureView::GetNativeView`.

If the editor wants to keep its `RHIBackend`-agnostic signature, add a
thin wrapper at the top of `ImGuiVulkanBackend.cpp`:

```cpp
static inline VkImageView ToVkImageView(RHITextureView& view)
{
    void* handle = view.GetNativeView(RHIBackend::Vulkan);
    XE_ASSERT(handle != nullptr);
    return static_cast<VkImageView>(handle);
}

static inline VkSampler ToVkSampler(RHISampler& sampler)
{
    void* handle = sampler.GetNativeHandle();
    XE_ASSERT(handle != nullptr);
    return static_cast<VkSampler>(handle);
}
```

The conversion sites then read:

```cpp
VkImageView imageView = ToVkImageView(*texture.GetDefaultView());
VkSampler   vkSampler = ToVkSampler(sampler);
```

## 5. RHI Module Cleanup

In `Engine/Source/Runtime/RHI/`:

### 5.1 Modify `Public/XEngine/RHI/RHIDevice.h` — remove `CreateX` virtuals

**Before** (the public virtual section after Stage 3):

```cpp
class RHI_API RHIDevice
{
public:
    // ... lifecycle ...

    virtual std::unique_ptr<RHIShader>          CreateShader(...) = 0;
    virtual std::unique_ptr<RHIBuffer>          CreateBuffer(...) = 0;
    virtual std::unique_ptr<RHITexture>         CreateTexture(...) = 0;
    virtual std::unique_ptr<RHISampler>         CreateSampler(...) = 0;
    virtual std::unique_ptr<RHIBindGroupLayout> CreateBindGroupLayout(...) = 0;
    virtual std::unique_ptr<RHIBindGroup>       CreateBindGroup(...) = 0;
    virtual std::unique_ptr<RHIGraphicsPipeline> CreateGraphicsPipeline(...) = 0;
    virtual std::unique_ptr<RHITextureView>     CreateTextureView(...) = 0;

    // factory / upload / caps ...
};
```

**After**:

```cpp
class RHI_API RHIDevice
{
public:
    // ... lifecycle ...

    // No more CreateX. Callers go through GetResourceFactory().

    virtual RHIResourceFactory& GetResourceFactory() = 0;
    virtual RHIUploadManager&   GetUploadManager() = 0;
    virtual const RHICapabilities& GetCapabilities() const = 0;
    virtual RHIDeferredDeleter&    GetDeferredDeleter() = 0;
};
```

Add a `[[deprecated]]` shim if any external test or sample still calls
`CreateX` after the Renderer migration:

```cpp
// Transitional. Will be removed once all callers migrate.
template <typename... Args>
[[deprecated("Use GetResourceFactory().CreateX(...) instead")]]
auto CreateShader(Args&&... args) -> decltype(GetResourceFactory().CreateShader(std::forward<Args>(args)...))
{
    return GetResourceFactory().CreateShader(std::forward<Args>(args)...);
}
// ... repeat for CreateBuffer / CreateTexture / CreateSampler / CreateBindGroupLayout / CreateBindGroup / CreateGraphicsPipeline / CreateTextureView
```

If the template approach is too noisy, write 8 explicit `[[deprecated]]`
inline forwarders.

### 5.2 Modify `Public/XEngine/RHI/Resources/RHITexture.h` — remove `GetNativeImageView`

**Before**:

```cpp
class RHI_API RHITexture : public RHIResource
{
public:
    // ... view helpers (Stage 5) ...

    void* GetNativeImageView(RHIBackend backend) const;     // ← remove
};
```

**After** — delete the function and its implementation in
`Private/Resources/RHITexture.cpp`:

```cpp
class RHI_API RHITexture : public RHIResource
{
public:
    // ... view helpers (Stage 5) ...
};
```

### 5.3 Modify `Public/XEngine/RHI/Resources/RHITextureView.h` — restrict `GetNativeView`

**Before**:

```cpp
class RHI_API RHITextureView : public RHIResource
{
public:
    virtual void* GetNativeView(RHIBackend backend) const = 0;
};
```

**After** — make it pure-virtual on the backend type, gated by an
internal tag the editor ImGui path cannot see. Simpler: change
visibility to `protected` on the base and expose a backend-only
accessor on the concrete `VulkanTextureView`:

```cpp
class RHI_API RHITextureView : public RHIResource
{
protected:
    virtual void* GetNativeView() const = 0;

    friend class VulkanTextureView;        // backend-only
};
```

In `VulkanTextureView.h`, expose `GetHandle()` as the public
backend-specific accessor:

```cpp
class VulkanTextureView : public RHITextureView
{
public:
    VkImageView GetHandle() const { return m_ImageView; }
protected:
    void* GetNativeView() const override { return m_ImageView; }
};
```

The editor ImGui backend already calls
`view->GetNativeView(RHIBackend::Vulkan)`. Update it to call
`static_cast<VulkanTextureView*>(view)->GetHandle()` directly, since
the editor is intrinsically Vulkan (Section 4.9).

If you prefer not to break the existing call site, add a `public` alias
on `RHITextureView`:

```cpp
class RHI_API RHITextureView : public RHIResource
{
public:
    void* GetNativeView(RHIBackend backend) const
    {
        if (backend != RHIBackend::Vulkan) return nullptr;     // Stage 7 single-backend
        return GetNativeView();
    }
protected:
    virtual void* GetNativeView() const = 0;
};
```

This keeps the call site compiling while still routing through the
virtual override.

### 5.4 Modify `Public/XEngine/RHI/RHITypes.h` — drop `ColorTexture / DepthTexture`

**Before**:

```cpp
struct RHIRenderOutputDesc
{
    RHITextureView* ColorTargetView = nullptr;
    RHITexture*     ColorTexture    = nullptr;     // ← remove
    RHITextureView* DepthTargetView = nullptr;
    RHITexture*     DepthTexture    = nullptr;     // ← remove

    RHIRect2D Viewport {};
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    bool RenderToSwapchain = true;
};
```

**After**:

```cpp
struct RHIRenderOutputDesc
{
    RHITextureView* ColorTargetView = nullptr;
    RHITextureView* DepthTargetView = nullptr;

    RHIRect2D Viewport {};
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    bool RenderToSwapchain = true;
};
```

### 5.5 Modify `Public/XEngine/RHI/Resources/RHIBindGroup.h` — drop `Texture`

**Before**:

```cpp
struct RHIBindingResource
{
    u32 Binding = 0;
    RHIBindingType Type = RHIBindingType::Unknown;

    RHITextureView* TextureView = nullptr;
    RHISampler*     Sampler     = nullptr;
    RHIBuffer*      Buffer      = nullptr;
};
```

**After** (no field removal — the rename was already done in Stage 5).
Stage 8 just verifies that no `Texture` field exists and that the
`TextureView` field is the only one in use.

### 5.6 Modify `Private/Vulkan/VulkanDevice.h` — drop `CreateX` overrides

**Before** (Stage 3 result):

```cpp
class VulkanDevice : public RHIDevice
{
public:
    // ... lifecycle ...
    std::unique_ptr<RHIShader> CreateShader(...) override;
    std::unique_ptr<RHIBuffer> CreateBuffer(...) override;
    std::unique_ptr<RHITexture> CreateTexture(...) override;
    std::unique_ptr<RHISampler> CreateSampler(...) override;
    std::unique_ptr<RHIBindGroupLayout> CreateBindGroupLayout(...) override;
    std::unique_ptr<RHIBindGroup> CreateBindGroup(...) override;
    std::unique_ptr<RHIGraphicsPipeline> CreateGraphicsPipeline(...) override;
    std::unique_ptr<RHITextureView> CreateTextureView(...) override;
    // ...
};
```

**After**:

```cpp
class VulkanDevice : public RHIDevice
{
public:
    // ... lifecycle ...
    RHIResourceFactory& GetResourceFactory() override { return *m_ResourceFactory; }
    RHIUploadManager&   GetUploadManager()   override { return *m_UploadManager; }
    // ...
};
```

`VulkanDevice.cpp` no longer has 7 `CreateX` bodies. The factory owns
the bodies.

### 5.7 Modify `Private/Vulkan/VulkanCommandList.cpp` — drop the colour/depth
shortcut path in `BeginRenderingIfNeeded`

**Before** (post-Stage 5):

```cpp
RHITextureView* colorView = m_RenderOutput.ColorTargetView != nullptr
                                ? m_RenderOutput.ColorTargetView
                                : (m_RenderOutput.ColorTexture != nullptr
                                    ? m_RenderOutput.ColorTexture->GetDefaultView()
                                    : nullptr);
```

**After**:

```cpp
RHITextureView* colorView = m_RenderOutput.ColorTargetView;
```

Same for depth.

### 5.8 Modify `Private/Vulkan/VulkanTexture.h/.cpp` — remove `GetNativeImageView`

`VulkanTexture` already lost `GetImageView` in Stage 2. `GetNativeImageView`
on the base `RHITexture` is removed in 5.2. `VulkanTexture.cpp` no longer
needs any view-related code; only the `m_DefaultView` is owned.

## 6. Verification

- **Build:** Final cleanup commit compiles.
- **Editor / Sandbox smoke test:** All previously-working scenes still
  render identically. No use of `GetNativeImageView` outside the backend
  remains.
- **RenderDoc:** Every Vulkan object has a debug name; every image
  attachment binds via a view; every descriptor set binding references a
  view.
- **`grep` audit:**

```bash
grep -rn "GetNativeImageView"     Engine/Source
grep -rn "m_Device->Create"       Engine/Source
grep -rn "dynamic_cast<.*Vulkan"   Engine/Source
grep -rn "ColorTexture\|DepthTexture" Engine/Source/Runtime/RHI/Public
```

Expected results:

```text
GetNativeImageView:        0 hits
m_Device->Create:          0 hits (allowed: only inside RHIDevice.h
                                    as deprecated inline forwarders
                                    during the one-commit deprecation
                                    window)
dynamic_cast<Vulkan*:       0 hits
RHIRenderOutputDesc::ColorTexture / DepthTexture:  0 hits
```

## 7. Common Mistakes

- Removing `RHIDevice::CreateX` wrappers but forgetting to update
  `RenderMaterialSystem` or `RenderFrameResources`. The compiler catches
  this; do not skip the build after the removal commit.
- Removing `RHITexture::GetNativeImageView` but forgetting
  `ImGuiVulkanBackend.cpp`. The editor ImGui overlay will show a black
  viewport.
- Forgetting that `RHIRenderOutputDesc::ColorTexture` and `DepthTexture`
  were convenience fields; some passes may have only used them. The
  compiler will catch the field references but not the semantic shift
  (default view vs explicit view).
- Updating one `m_Device->CreateX` call to use the factory but leaving
  another to use the wrapper. Both compile but the inconsistency is
  confusing.
- Forgetting to update `VulkanDevice.cpp` after removing `CreateX` from
  its header. The overrides will be declared but undefined.
- Leaving `[[deprecated]]` forwarders in the codebase after a single
  release cycle. They should be removed in the follow-up commit.
- Removing `GetNativeImageView` but leaving the `void*` API in
  `RHITextureView::GetNativeView(RHIBackend)`. The intent is to restrict
  view-handle access to the backend.

## 8. What This Stage Intentionally Does Not Do

- Does **not** implement CSM algorithm. Stage 9.
- Does **not** implement RenderGraph V1.
- Does **not** implement async upload / transfer queue.
- Does **not** implement bindless.
- Does **not** refactor `RHISystem` / `Engine` / `Editor` subsystems beyond
  the migration of the listed callers.

## 9. Migration Order (Recommended)

The order minimises compile breakage:

1. Migrate Renderer callers to factory / upload manager / views, leaving
   `RHIDevice::CreateX` wrappers in place. Verify all Renderer compiles
   and runs identically.
2. Migrate `ImGuiVulkanBackend.cpp` to use view handles. Verify editor
   still renders the UI correctly.
3. Remove `RHIRenderOutputDesc::ColorTexture / DepthTexture` fields.
4. Remove `RHIBindingResource::Texture` field (already done in Stage 5;
   verify no caller still references the field).
5. Remove `RHITexture::GetNativeImageView`.
6. Remove `RHIDevice::CreateX` virtual wrappers. `VulkanDevice` no longer
   implements them; `RHIDevice` declares them only as `[[deprecated]]`
   inline forwarders that forward to the factory.
7. Remove the `[[deprecated]]` wrappers in a follow-up commit.

This ordering keeps every commit building and rendering.
