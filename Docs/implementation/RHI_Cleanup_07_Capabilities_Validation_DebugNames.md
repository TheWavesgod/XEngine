# Stage 7 — Capabilities, Validation, and Debug Names

## 1. Goal and Scope

Add only the safety and diagnostics needed by the resource/view work:

- a small, truthful `RHICapabilities` snapshot;
- centralized format and descriptor validation in `RHIResourceFactory`;
- Vulkan object debug names.

Implement this as two buildable batches:

```text
Stage 7A  capabilities + format helpers + factory validation
Stage 7B  Vulkan debug names
```

Deferred destruction is explicitly removed from this stage. A callback queue
flushed at `BeginFrame` is not GPU-safe, and an unused placeholder adds public
API without solving lifetime. Add deferred destruction later together with
frame-slot fences and actual resource integration.

## 2. Corrections to the Earlier Draft

Do not implement these earlier proposals:

- `RHIDeferredDeleter` or `RHIDevice::GetDeferredDeleter()`;
- `MaxBufferSize = maxStorageBufferRange` (those are different limits);
- `MaxSampleCount` by casting Vulkan sample-count bitmasks to an integer;
- `MaxBindingsPerBindGroup` by summing unrelated per-stage descriptor limits;
- rejecting `RHIBindGroupLayoutEntry::Count > 1` when bindless is disabled
  (ordinary fixed descriptor arrays are not bindless);
- claiming dynamic rendering is guaranteed by Vulkan 1.0;
- refreshing physical-device limits on swapchain recreation;
- silently narrowing float anisotropy limits to `u32`.

Capabilities describe limits/features actually queried and enabled by the
created logical device. Unsupported or unqueried future features should not be
represented as optimistic booleans.

## 3. Stage 7A — Capabilities

### 3.1 Public shape

Add `Public/XEngine/RHI/RHICapabilities.h`:

```cpp
#pragma once

#include <XEngine/Core/Types.h>

namespace XEngine
{
    struct RHICapabilities
    {
        u32 MaxTextureDimension2D = 0;
        u32 MaxTextureArrayLayers = 0;
        u32 MaxPushConstantSize = 0;
        u32 MaxBoundDescriptorSets = 0;

        bool SupportsSamplerAnisotropy = false;
        f32 MaxSamplerAnisotropy = 1.0f;

        // True only if dynamic rendering was enabled on the logical device.
        bool SupportsDynamicRendering = false;
    };
}
```

Expose it read-only:

```cpp
virtual const RHICapabilities& GetCapabilities() const = 0;
virtual bool SupportsTextureFormat(
    RHIFormat format,
    RHITextureUsageFlags usage) const = 0;
```

`SupportsTextureFormat` is a capability query, not a creation path. The Vulkan
implementation derives it from `vkGetPhysicalDeviceFormatProperties` and may
cache the small result table at device initialization. This keeps validation
policy in the factory without putting Vulkan types in public headers.

Populate once during device initialization, after physical-device selection
and after the enabled logical-device feature set is known. These fields do not
change when the swapchain is recreated.

### 3.2 Vulkan mapping

Use `VkPhysicalDeviceProperties::limits` for limits and the device's enabled
feature/extension record for feature booleans:

```cpp
VkPhysicalDeviceProperties properties {};
vkGetPhysicalDeviceProperties(physicalDevice, &properties);

caps.MaxTextureDimension2D = properties.limits.maxImageDimension2D;
caps.MaxTextureArrayLayers = properties.limits.maxImageArrayLayers;
caps.MaxPushConstantSize = properties.limits.maxPushConstantsSize;
caps.MaxBoundDescriptorSets = properties.limits.maxBoundDescriptorSets;

caps.SupportsSamplerAnisotropy = enabledFeatures.samplerAnisotropy == VK_TRUE;
caps.MaxSamplerAnisotropy = caps.SupportsSamplerAnisotropy
    ? properties.limits.maxSamplerAnisotropy
    : 1.0f;

caps.SupportsDynamicRendering = dynamicRenderingWasEnabled;
```

Use the actual feature/extension state already maintained by `VulkanDevice`;
do not invent `VulkanPhysicalDevice` or `m_PhysicalDeviceCtx` types that are
not in the repository.

## 4. Stage 7A — Format Helpers

Add helpers for the current `RHIFormat` enum only:

```cpp
bool IsDepthFormat(RHIFormat format);
bool IsStencilFormat(RHIFormat format);
bool IsSrgbFormat(RHIFormat format);
u32 GetFormatTexelSize(RHIFormat format); // 0 for Undefined/unsupported
RHITextureAspectFlags GetDefaultAspect(RHIFormat format);
u32 GetMaxMipLevels(u32 width, u32 height);
```

Keep these backend-independent. Do not add depth/stencil formats to helper
switches until they are added to `RHIFormat` and mapped by every active
backend.

Use the name `GetFormatTexelSize`, not `GetBytesPerPixel`: the latter becomes
misleading once block-compressed formats arrive.

## 5. Stage 7A — Factory Validation

Validation and normalization happen once in public NVI methods before calling
`CreateXImpl`. Backend constructors may retain assertions for impossible
states but must not duplicate policy.

### 5.1 Texture validation

`CreateTexture` rejects:

- zero width/height, mip count, or array-layer count;
- extents/layers above device limits;
- mip count above `GetMaxMipLevels(width, height)`;
- `Format == Undefined` or `Usage == None`;
- `Texture2D` with `ArrayLayers != 1`;
- `Texture2DArray` with fewer than one layer;
- `TextureCube` with width != height or `ArrayLayers != 6`;
- depth/stencil attachment usage on a color format;
- color attachment usage on a depth format;
- sampled/storage/attachment usage unsupported by the Vulkan format.

The last check is backend capability, not a generic enum property. The factory
calls `RHIDevice::SupportsTextureFormat(format, usage)`; Vulkan answers from
`vkGetPhysicalDeviceFormatProperties` (preferably a cached table). Reject
unsupported feature combinations and do not silently alter the request.

### 5.2 Texture-view normalization and validation

Normalize `MipCount == 0` and `ArrayLayerCount == 0` to the remaining range,
then validate using overflow-safe comparisons:

```cpp
if (baseMip >= textureDesc.MipLevels ||
    mipCount > textureDesc.MipLevels - baseMip)
{
    return nullptr;
}

if (baseLayer >= textureDesc.ArrayLayers ||
    layerCount > textureDesc.ArrayLayers - baseLayer)
{
    return nullptr;
}
```

Also validate:

- source texture is non-null and belongs to this factory's device;
- aspect is non-empty and compatible with the source format;
- view usage is supported by the texture's creation usage;
- `Format == Undefined` normalizes to source format;
- any other format is rejected until mutable-format images are supported;
- `Texture2D` view has exactly one layer;
- `Texture2DArray` view may cover one or more layers;
- `TextureCube` view covers exactly six layers, starts on a cube boundary, and
  the source texture was created cube-compatible.

The normalized descriptor is the descriptor stored by `RHITextureView`, so
backend code never has to interpret zero counts.

### 5.3 Bind-group validation

For `CreateBindGroupLayout`:

- reject duplicate binding numbers, `Unknown` types, zero counts, and empty
  visibility;
- fixed `Count > 1` is allowed if it fits the backend descriptor limits;
- reject unsupported binding types explicitly.

For `CreateBindGroup`:

- layout and every resource must belong to the same device;
- each resource binding must exist in the layout and have matching type;
- required resources are non-null;
- reject duplicate or missing bindings for the V0 one-resource-per-entry API.

Do not call fixed arrays "bindless". Descriptor indexing/update-after-bind is
a later feature with separate capability checks.

### 5.4 Pipeline validation

Retain Stage-6 validation and add:

- shader and bind-group-layout owner-device checks;
- `PushConstantSize <= MaxPushConstantSize`;
- bind-group layout count `<= MaxBoundDescriptorSets`;
- color/depth formats compatible with the corresponding attachment roles.

## 6. Stage 7A — Sampler Anisotropy

Enable anisotropy only when the feature was enabled on the logical device:

```cpp
const auto& caps = device.GetCapabilities();
const bool useAnisotropy =
    desc.MaxAnisotropy > 1.0f && caps.SupportsSamplerAnisotropy;

samplerInfo.anisotropyEnable = useAnisotropy ? VK_TRUE : VK_FALSE;
samplerInfo.maxAnisotropy = useAnisotropy
    ? std::min(desc.MaxAnisotropy, caps.MaxSamplerAnisotropy)
    : 1.0f;
```

If the request is above 1 but the feature is disabled, either reject it in the
factory or log a single clear warning and normalize to 1. Choose one policy and
test it; do not enable a device feature merely because the limit is non-zero.

## 7. Stage 7B — Vulkan Debug Names

Add one backend-private helper:

```cpp
void VulkanSetDebugName(
    VulkanDevice& device,
    VkObjectType objectType,
    u64 objectHandle,
    const char* debugName);
```

The helper no-ops for null names/handles or when debug utils are unavailable.
Load the device command after logical-device creation (or use Volk's loaded
device function) and call it only when `VK_EXT_debug_utils` was enabled.

Apply names after successful creation:

| RHI object | Vulkan object type |
|---|---|
| buffer | `VK_OBJECT_TYPE_BUFFER` |
| texture | `VK_OBJECT_TYPE_IMAGE` |
| texture view | `VK_OBJECT_TYPE_IMAGE_VIEW` |
| sampler | `VK_OBJECT_TYPE_SAMPLER` |
| shader | `VK_OBJECT_TYPE_SHADER_MODULE` |
| pipeline | `VK_OBJECT_TYPE_PIPELINE` |
| pipeline layout | `VK_OBJECT_TYPE_PIPELINE_LAYOUT` |
| bind-group layout | `VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT` |
| bind group | `VK_OBJECT_TYPE_DESCRIPTOR_SET` |

Do not label a descriptor-set layout as a pipeline layout. If one RHI object
owns multiple Vulkan handles, derive stable suffixes such as
`"ForwardOpaque.Layout"` rather than assigning the same name to all handles.

`DebugName` is borrowed during creation. Backend objects must not assume the
`const char*` remains valid unless the descriptor stores an owned string; the
Vulkan naming call itself copies the name.

## 8. Files

Add:

```text
Public/XEngine/RHI/RHICapabilities.h
Private/Vulkan/VulkanDebugName.h
Private/Vulkan/VulkanDebugName.cpp
```

Modify:

```text
Public/XEngine/RHI/RHI.h
Public/XEngine/RHI/RHIDevice.h
Public/XEngine/RHI/RHIUtils.h
Private/RHIUtils.cpp
Private/RHIResourceFactory.cpp
Private/Vulkan/VulkanDevice.h/.cpp
Private/Vulkan/VulkanResourceFactory.cpp
Private/Vulkan/VulkanBuffer.cpp
Private/Vulkan/VulkanTexture.cpp
Private/Vulkan/VulkanTextureView.cpp
Private/Vulkan/VulkanSampler.cpp
Private/Vulkan/VulkanShader.cpp
Private/Vulkan/VulkanPipeline.cpp
Private/Vulkan/VulkanDescriptor.cpp
```

Do not add `RHIDeferredDeleter.*` in this stage.

## 9. Implementation Order

1. Add the minimal capability struct and populate it from real Vulkan state.
2. Add format helpers for formats that currently exist.
3. Harden texture and view validation/normalization.
4. Harden bind-group and pipeline validation.
5. Wire anisotropy using enabled-feature state.
6. Build and run validation tests (Stage 7A checkpoint).
7. Add the debug-name helper and name each successfully created handle.
8. Verify names in RenderDoc (Stage 7B checkpoint).

## 10. Verification

- Capability values match `VkPhysicalDeviceProperties` on the selected GPU.
- Invalid mip/layer ranges, aspects, dimensions, usage combinations, and
  cross-device resources are rejected before backend creation.
- `MipCount == 0` / `ArrayLayerCount == 0` are stored normalized.
- A fixed descriptor array is not rejected merely because bindless is absent.
- Anisotropy never becomes enabled unless the logical-device feature is on.
- RenderDoc shows correct names and object types.
- No deferred-deletion API or unused queue was introduced.

## 11. Out of Scope

- Deferred destruction and frame-fence retirement.
- Bindless/descriptor indexing/update-after-bind.
- Mutable-format images and view reinterpretation.
- Compressed formats.
- MRT, MSAA, multiview, and conservative rasterization.
- Full resource-state tracking.
