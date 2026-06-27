# Stage 8 — Migration and Transitional API Removal

## 1. Goal

Complete the cleanup after Stages 1–7:

- every renderer resource is created through `RHIResourceFactory`;
- uploads go through `RHIUploadManager`;
- renderer owners retain explicit texture views;
- render outputs and bind groups use non-owning view pointers;
- the Stage-2 texture-owned default-view compatibility path is removed;
- native handles are available only through intentional backend interop APIs.

This checklist follows the checked-in `shared_ptr` ownership model and current
type names (`RHIPipeline`, `ColorTarget`, `DepthTarget`). It does not introduce
`unique_ptr`, `RHIGraphicsPipeline`, `ColorTargetView`, or fallback texture
fields.

## 2. Final Ownership Model

```text
RHITexture
  └─ image resource only; no default view and no view cache

Renderer semantic owner
  ├─ shared_ptr<RHITexture>
  └─ shared_ptr<RHITextureView> ...

RHIRenderOutputDesc / RHIBindingResource / frame data
  └─ raw, non-owning pointers
```

Required owners:

- `RenderTextureManager::TextureRecord` owns each material sampled view;
- `EditorViewportRenderTarget` owns color/depth attachment views;
- `ShadowResourceCache` owns whole-array and per-layer shadow views;
- a future RenderGraph owns transient/pass-specific views.

Views must be released before their source texture. Bind groups that reference
a view must be released or updated before that view is released.

## 3. Remove the Default-View Compatibility Path

Remove from `RHITexture`:

```text
GetDefaultView()
GetNativeDefaultView(...)
```

Remove from `VulkanTexture`:

```text
m_DefaultView
lazy CreateTextureView call
GetDefaultView override
GetNativeDefaultView override
```

After this change a texture constructor creates only the native image. A view
is created only when a semantic owner requests one from the factory.

Before removal, verify:

```powershell
rg -n "GetDefaultView|GetNativeDefaultView" Engine/Source
```

Every hit must first migrate to an explicitly owned view. Do not mechanically
replace it with a temporary factory call: that would create an immediately
destroyed view or hide ownership at the call site.

## 4. Factory and Upload Migration

`RHIDevice` already exposes `GetResourceFactory()` and `GetUploadManager()` in
the post-Stage-4 baseline. Migrate every remaining compatibility call.

### 4.1 Texture creation

```cpp
auto& factory = m_Device->GetResourceFactory();
auto texture = factory.CreateTexture(desc);
if (!texture)
{
    return failure;
}

if (!pixels.empty())
{
    m_Device->GetUploadManager().UploadTexture(
        *texture, pixels.data(), pixels.size(), AllSubresources());
}
```

The current Stage-4 upload API returns `void`; failures are reported through
its existing assertion/logging contract. If it later returns a status, handle
that status before publishing the renderer record. Create and store the
semantic view only after texture creation/upload completes.

### 4.2 Buffers, shaders, samplers, layouts, bind groups, pipelines

Replace all `device.CreateX` / `m_Device->CreateX` compatibility calls with:

```cpp
device.GetResourceFactory().CreateX(...)
m_Device->GetResourceFactory().CreateX(...)
```

Audit at least:

```text
Renderer/Private/Resources/RenderTextureManager.cpp
Renderer/Private/Resources/RenderMeshManager.cpp
Renderer/Private/Resources/RenderShaderLibrary.cpp
Renderer/Private/Resources/RenderPipelineStateCache.cpp
Renderer/Private/Resources/RenderMaterialSystem.cpp
Renderer/Private/Resources/RenderFrameResources.cpp
Renderer/Private/Mesh/PrimitiveMeshes.cpp
Editor/Private/Viewport/EditorViewportRenderTarget.cpp
```

Preserve `std::shared_ptr` return values. Do not call `.release()`; that API is
for `unique_ptr` and does not match this repository.

## 5. View Consumer Migration

### 5.1 Materials

Material code resolves a view from `RenderTextureManager`, not a texture's
default view:

```cpp
RHITextureView* baseColorView =
    m_TextureManager->GetTextureView(baseColorHandle);

RHIBindingResource resource;
resource.Binding = binding;
resource.Type = RHIBindingType::CombinedImageSampler;
resource.TextureView = baseColorView;
resource.Sampler = sampler;
```

Update fallback resolution to return views as well. A valid texture with a
missing view is an invalid renderer record and should fall back or fail
clearly.

### 5.2 Editor viewport

`EditorViewportRenderTarget` exposes explicit accessors:

```cpp
RHITextureView* GetColorTargetView() const;
RHITextureView* GetDepthTargetView() const;
```

Rendering uses those accessors directly:

```cpp
RHIRenderOutputDesc output;
output.ColorTarget = viewportTarget.GetColorTargetView();
output.DepthTarget = viewportTarget.GetDepthTargetView();
output.RenderToSwapchain = false;
```

On resize, ensure GPU work using the old target is complete under the current
device policy, then destroy old bind groups/interop registrations, views, and
textures in that order.

### 5.3 Shadows

`ShadowResourceCache` retains owning pointers. Frame data copies only raw
pointers:

```cpp
frame.ShadowTexture = resources.Texture.get();
frame.SampledView = resources.SampledView.get();
frame.CascadeDepthViews[i] = resources.LayerDepthViews[i].get();
```

Rebuilding shadow settings releases or retires old bind groups and views
before the old texture. Stage 8 establishes the contract; Stage 9 implements
the feature.

## 6. Native Handle Interop

Do not expose concrete private `VulkanTextureView` types to the Editor and do
not propose a `static_cast` from `RHITextureView*` to a backend class whose
header is private to the RHI module.

Keep a narrow, explicit native-handle escape hatch on resources required by
the Vulkan ImGui backend. Use consistent naming:

```cpp
virtual void* GetNativeHandle(RHIBackend backend) const;
```

Implement it for `RHITextureView` and `RHISampler`. It returns null when the
requested backend does not match the owner device. Ordinary Renderer code must
not use it; the backend-specific ImGui bridge is the allowed consumer.

`ImGuiVulkanBackend` receives the explicit viewport color view:

```cpp
VkImageView imageView = static_cast<VkImageView>(
    colorView.GetNativeHandle(RHIBackend::Vulkan));
VkSampler sampler = static_cast<VkSampler>(
    viewportSampler.GetNativeHandle(RHIBackend::Vulkan));
```

If platform rules make Vulkan non-dispatchable handles non-convertible through
`void*`, use an integer native-handle carrier (`uintptr_t`/`u64`) or an RHI
native-interop query struct consistently for both resources. Do not mix
unrelated `GetNativeView`, `GetNativeImageView`, `GetNativeDefaultView`, and
`GetNativeSampler` spellings in the final API.

## 7. Bind-Group Update API Review

The current `RHIDevice::UpdateBindGroupSampledTexture(...)` TODO should not
remain as an unexplained device-level special case.

For Stage 8 choose one explicit V0 policy:

1. **Recommended:** rebuild the small frame bind group when shadow resources
   change. Resource changes are infrequent and this avoids a one-off mutable
   API.
2. If in-place updates are required, put a generic, validated update operation
   on the resource factory/descriptor service, not directly on `RHIDevice`, and
   define synchronization and retained-resource lifetime.

Do not keep a shadow-motivated sampled-texture-only virtual on the generic
device merely because Stage 9 might use it.

## 8. Cleanup Searches

Run from the repository root:

```powershell
rg -n "GetDefaultView|GetNativeDefaultView|GetNativeImageView" Engine/Source
rg -n "\b(m_Device|device)->?Create(Shader|Buffer|Texture|TextureView|Sampler|BindGroup|BindGroupLayout|GraphicsPipeline)" Engine/Source
rg -n "resource\.Texture\b|\.Texture\s*=.*" Engine/Source/Runtime/RHI Engine/Source/Runtime/Renderer
rg -n "dynamic_cast<.*Vulkan" Engine/Source
rg -n "ColorTexture|DepthTexture|ColorTargetView|DepthTargetView" Engine/Source
rg -n "ViewCache|ViewKey|GetOrCreateSubresourceView" Engine/Source
```

Interpret results rather than demanding blind zero hits: `.Texture` remains
valid in `RHITextureViewDesc`, and resource factories legitimately contain
`CreateX` implementation names. The important result is no transitional call
site and no hidden view cache.

## 9. Recommended Commit Order

1. Add explicit views to renderer/editor owner records and migrate their
   consumers while the Stage-2 default view still exists.
2. Migrate all resource creation and uploads to factory/upload services.
3. Migrate ImGui Vulkan interop to the explicit color view and unified native
   handle API.
4. Remove `GetDefaultView`, native-default-view functions, and
   `VulkanTexture::m_DefaultView`.
5. Remove any remaining `RHIDevice::CreateX` compatibility wrappers.
6. Resolve/remove the one-off bind-group update API.
7. Run full build, validation layers, Editor/Sandbox smoke tests, and searches.

Each commit should build. If the project cannot preserve that property, merge
only the tightly coupled header/caller changes—not unrelated stages.

## 10. Verification

- Runtime, Editor, and Sandbox build with the current `shared_ptr` API.
- Existing forward/material rendering is unchanged.
- Editor viewport survives repeated resize and ImGui texture registration.
- Depth-only test from Stage 6 still records zero color attachments.
- Vulkan validation reports no destroyed-image/still-live-view errors.
- Renderer feature owners visibly own all views they expose as raw pointers.
- `RHITexture` contains neither a default view nor an arbitrary view cache.
- No `.release()` appears on factory results.

## 11. Out of Scope

- CSM algorithm and shadow draw implementation.
- RenderGraph V1 and transient view interning.
- Fence-aware deferred resource destruction.
- Bindless descriptors.
- Handle-based RHI rewrite.
