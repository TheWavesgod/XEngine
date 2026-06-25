# Stage 5 — View-Based RenderPass and BindGroup

## 1. Goal

Switch every "texture" reference at the RHI rendering boundary from
`RHITexture*` to `RHITextureView*`:

- `RHIRenderOutputDesc::ColorTarget` becomes `RHITextureView*` plus a
  fallback `RHITexture*` only for the "whole texture" shortcut.
- `RHIRenderOutputDesc::DepthTarget` becomes `RHITextureView*`.
- `RHIBindingResource::Texture` becomes `RHITextureView*`.
- `RHICommandList::SetRenderOutput` accepts the view-based descriptor.
- `RHICommandList::SetBindGroup` continues to take an `RHIBindGroup*`
  whose resources are now `RHITextureView*`.

This makes the CSM shadow case first-class:

```text
// ShadowDepthPass for cascade 0:
RHIRenderOutputDesc out;
out.ColorTarget = nullptr;                 // no color write
out.DepthTarget = shadowTexture->GetLayerDepthView(0); // layer 0
out.Viewport = { 0, 0, 2048, 2048 };

// LightingPass:
RHIBindingResource shadowBinding;
shadowBinding.Binding = 0;
shadowBinding.Type = RHIBindingType::SampledTexture;
shadowBinding.TextureView = shadowTexture->GetWholeArraySampledView();
shadowBinding.Sampler = shadowSampler;
```

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h
  - RHIRenderOutputDesc: ColorTarget/DepthTarget are RHITexture*

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBindGroup.h
  - RHIBindingResource: Texture is RHITexture*

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICommandList.h
  - SetRenderOutput(const RHIRenderOutputDesc&)
  - SetBindGroup(u32 setIndex, RHIBindGroup*)

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp
  - BeginRenderingIfNeeded (dynamic_cast RHITexture* from output)
  - SetBindGroup (passes through to vkCmdBindDescriptorSets)
  - VulkanDescriptor.cpp::Create (binds texture via GetImageView())

Engine/Source/Runtime/Renderer/Private/RenderResourceContext.h
Engine/Source/Runtime/Renderer/Private/Passes/*
Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h
  - TODO comments for RHITextureView fields
```

What already exists:

- `RHITexture::GetDefaultView()` is added in Stage 2.
- `VulkanDescriptor.cpp` writes a single combined-image-sampler binding
  pulling `VkImageView` from `texture->GetImageView()`.

What is missing:

- `RHIRenderOutputDesc` / `RHIBindingResource` still use `RHITexture*`.
- No helper on `RHITexture` to produce a "layer-i depth view" or a
  "whole-array sampled view".

What should **not** be changed yet:

- `RHIGraphicsPipelineDesc::ColorFormat` is still required (Stage 6).
- `RHITexture::GetNativeImageView` is still transitional (Stage 8).
- Renderer callers do not need to be migrated yet — transitional
  compatibility helpers are added in this stage.

## 3. Files to Add

None — this stage is a public header shape change plus backend wiring. New
helpers can live in existing files.

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h
  - RHIRenderOutputDesc: ColorTarget/DepthTarget -> RHITextureView*

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
  - add GetOrCreateLayerDepthView(u32 layer)
  - add GetOrCreateWholeArraySampledView()
  - add GetOrCreateSubresourceView(RHITextureAspect aspect,
                                    u32 baseMip, u32 mipCount,
                                    u32 baseLayer, u32 layerCount,
                                    RHITextureViewUsageFlags usage)

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBindGroup.h
  - RHIBindingResource: Texture -> TextureView

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICommandList.h
  - SetRenderOutput doc updated (signature unchanged)

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp
  - BeginRenderingIfNeeded: read VkImageView from view
  - EndRenderingIfActive: unchanged

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp
  - CombinedImageSampler: read VkImageView from view

Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h
  - Fill in the TODO fields with RHITextureView* (no Renderer-side
    code change yet — just unblock Stage 9).
```

## 5. Detailed Code Plan

### 5.1 Modify: `RHITypes.h` — `RHIRenderOutputDesc` adds view fields

**Before** (lines 143–151 of `RHITypes.h`):

```cpp
struct RHIRenderOutputDesc
{
    RHITexture* ColorTarget = nullptr;
    RHITexture* DepthTarget = nullptr;
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
    // Either ColorTargetView or ColorTexture. View wins if both set.
    RHITextureView* ColorTargetView = nullptr;
    RHITexture*     ColorTexture    = nullptr;

    RHITextureView* DepthTargetView = nullptr;
    RHITexture*     DepthTexture    = nullptr;

    RHIRect2D Viewport {};
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    bool RenderToSwapchain = true;
};
```

Add at the top of `RHITypes.h`:

```cpp
class RHITextureView;     // NEW forward decl
```

(or rely on the transitive include from `RHI.h` already pulling in
`RHITextureView.h`)

### 5.2 Modify: `Resources/RHIBindGroup.h` — `RHIBindingResource` uses view

**Before** (lines 36–44 of `RHIBindGroup.h`):

```cpp
struct RHIBindingResource
{
    u32 Binding = 0;
    RHIBindingType Type = RHIBindingType::Unknown;

    RHITexture* Texture = nullptr;
    RHISampler* Sampler = nullptr;
    RHIBuffer* Buffer = nullptr;
};
```

**After**:

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

### 5.3 Modify: `Resources/RHITexture.h` — add view-creation helpers

**Before** (the body of the `RHITexture` class, after `GetDefaultView`):

```cpp
    virtual RHITextureView* GetDefaultView() const = 0;
```

**After** — add three helpers below `GetDefaultView`:

```cpp
    virtual RHITextureView* GetDefaultView() const = 0;

    // CSM helpers. Both cache results on the texture.
    RHITextureView* GetOrCreateWholeArraySampledView();
    RHITextureView* GetOrCreateLayerDepthView(u32 layer);

    // General subresource view cache. Keyed by (aspect, base mip,
    // mip count, base layer, layer count, usage).
    RHITextureView* GetOrCreateSubresourceView(
        RHITextureAspect aspect,
        u32 baseMip, u32 mipCount,
        u32 baseLayer, u32 layerCount,
        RHITextureViewUsageFlags usage);

protected:
    explicit RHITexture(RHIDevice& ownerDevice);
};
```

### 5.4 Modify: `Resources/RHITexture.cpp` — implement the helpers

**Before** (the existing `GetNativeImageView` implementation):

```cpp
void* RHITexture::GetNativeImageView(RHIBackend backend) const
{
    RHITextureView* view = GetDefaultView();
    return view != nullptr ? view->GetNativeView(backend) : nullptr;
}
```

**After** — keep the existing function and add the cache and three
helpers. Replace the entire file body with:

```cpp
// Engine/Source/Runtime/RHI/Private/Resources/RHITexture.cpp
#include "XEngine/RHI/Resources/RHITexture.h"

#include "XEngine/RHI/RHIDevice.h"
#include "XEngine/RHI/RHIResourceFactory.h"
#include "XEngine/RHI/Resources/RHITextureView.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

#include <functional>
#include <memory>
#include <unordered_map>

namespace XEngine
{
    namespace
    {
        struct ViewKey
        {
            RHITextureAspect aspect;
            u32 baseMip;
            u32 mipCount;
            u32 baseLayer;
            u32 layerCount;
            u32 usage;     // RHITextureViewUsageFlags

            bool operator==(const ViewKey& other) const
            {
                return aspect == other.aspect
                    && baseMip == other.baseMip
                    && mipCount == other.mipCount
                    && baseLayer == other.baseLayer
                    && layerCount == other.layerCount
                    && usage == other.usage;
            }
        };

        struct ViewKeyHash
        {
            std::size_t operator()(const ViewKey& key) const
            {
                std::size_t h = 0;
                auto combine = [&](std::size_t v)
                {
                    h ^= v + 0x9e3779b9u + (h << 6u) + (h >> 2u);
                };
                combine(static_cast<std::size_t>(key.aspect));
                combine(key.baseMip);
                combine(key.mipCount);
                combine(key.baseLayer);
                combine(key.layerCount);
                combine(key.usage);
                return h;
            }
        };
    }

    void* RHITexture::GetNativeImageView(RHIBackend backend) const
    {
        RHITextureView* view = GetDefaultView();
        return view != nullptr ? view->GetNativeView(backend) : nullptr;
    }

    // Texture-local view cache lives as a mutable unordered_map on the
    // texture instance. We implement it in RHITextureView.cpp (see 5.5).
    // Forward declarations here; actual cache access goes through the
    // file's detail::ViewCache class.

    // RHITexture helper bodies are added in 5.5.
}
```

### 5.5 New file: `Private/Resources/RHITexture_Views.h` (cache header)

The texture-local view cache has to live somewhere accessible from both
`RHITexture::GetOrCreateWholeArraySampledView` and the `RHITexture`
base methods. Stage 5 splits it into a small detail class:

```cpp
// Engine/Source/Runtime/RHI/Private/Resources/RHITexture_Views.h
#pragma once

#include "XEngine/RHI/Resources/RHITexture.h"
#include "XEngine/RHI/Resources/RHITextureView.h"

#include <memory>
#include <unordered_map>

namespace XEngine::detail
{
    struct ViewKey
    {
        RHITextureAspect aspect;
        u32 baseMip;
        u32 mipCount;
        u32 baseLayer;
        u32 layerCount;
        u32 usage;

        bool operator==(const ViewKey& other) const
        {
            return aspect == other.aspect
                && baseMip == other.baseMip
                && mipCount == other.mipCount
                && baseLayer == other.baseLayer
                && layerCount == other.layerCount
                && usage == other.usage;
        }
    };

    struct ViewKeyHash
    {
        std::size_t operator()(const ViewKey& k) const
        {
            std::size_t h = 0;
            auto combine = [&](std::size_t v) { h ^= v + 0x9e3779b9u + (h << 6u) + (h >> 2u); };
            combine(static_cast<std::size_t>(k.aspect));
            combine(k.baseMip); combine(k.mipCount);
            combine(k.baseLayer); combine(k.layerCount);
            combine(k.usage);
            return h;
        }
    };

    // Each RHITexture owns one of these as `mutable std::unique_ptr<ViewCache>`.
    class ViewCache
    {
    public:
        RHITextureView* GetOrCreate(
            RHIDevice& device,
            RHITexture& texture,
            const ViewKey& key,
            const char* debugName);

    private:
        std::unordered_map<ViewKey, std::shared_ptr<RHITextureView>, ViewKeyHash> m_Cache;
    };
}
```

### 5.6 Modify: `Resources/RHITexture.h` — add cache member

**Before** (the protected section of `RHITexture`):

```cpp
protected:
    explicit RHITexture(RHIDevice& ownerDevice);
};
```

**After**:

```cpp
protected:
    explicit RHITexture(RHIDevice& ownerDevice);
    ~RHITexture() override;       // NEW: declared here so the cache dtor runs.

private:
    mutable std::unique_ptr<detail::ViewCache> m_ViewCache;     // NEW
};
```

Add forward declaration at the top:

```cpp
namespace XEngine::detail { class ViewCache; }
```

And `#include` the `ViewCache` header inside `RHITexture.cpp` only — not in
the public header.

### 5.7 New file: `Private/Resources/RHITexture.cpp` — full implementation

```cpp
// Engine/Source/Runtime/RHI/Private/Resources/RHITexture.cpp
#include "XEngine/RHI/Resources/RHITexture.h"

#include "XEngine/Core/Assert.h"
#include "XEngine/Logging/Log.h"

#include "Resources/RHITexture_Views.h"     // private detail header

#include "XEngine/RHI/RHIDevice.h"
#include "XEngine/RHI/RHIResourceFactory.h"

namespace XEngine
{
    namespace detail
    {
        RHITextureView* ViewCache::GetOrCreate(
            RHIDevice& device,
            RHITexture& texture,
            const ViewKey& key,
            const char* debugName)
        {
            auto it = m_Cache.find(key);
            if (it != m_Cache.end())
            {
                return it->second.get();
            }

            RHITextureViewDesc desc;
            desc.Texture = &texture;
            desc.Aspect = key.aspect;
            desc.BaseMipLevel = key.baseMip;
            desc.MipCount = key.mipCount;
            desc.BaseArrayLayer = key.baseLayer;
            desc.ArrayLayerCount = key.layerCount;
            desc.Usage = static_cast<RHITextureViewUsageFlags>(key.usage);
            desc.DebugName = debugName;

            auto view = device.GetResourceFactory().CreateTextureView(desc);
            if (!view)
            {
                return nullptr;
            }
            RHITextureView* raw = view.get();
            m_Cache.emplace(key, std::move(view));
            return raw;
        }
    }

    RHITexture::~RHITexture() = default;

    void* RHITexture::GetNativeImageView(RHIBackend backend) const
    {
        RHITextureView* view = GetDefaultView();
        return view != nullptr ? view->GetNativeView(backend) : nullptr;
    }

    RHITextureView* RHITexture::GetOrCreateSubresourceView(
        RHITextureAspect aspect,
        u32 baseMip, u32 mipCount,
        u32 baseLayer, u32 layerCount,
        RHITextureViewUsageFlags usage)
    {
        if (!m_ViewCache) { m_ViewCache = std::make_unique<detail::ViewCache>(); }

        detail::ViewKey key;
        key.aspect = aspect;
        key.baseMip = baseMip;
        key.mipCount = mipCount;
        key.baseLayer = baseLayer;
        key.layerCount = layerCount;
        key.usage = static_cast<u32>(usage);

        return m_ViewCache->GetOrCreate(GetOwnerDevice(), *this, key, GetDesc().DebugName);
    }

    RHITextureView* RHITexture::GetOrCreateWholeArraySampledView()
    {
        const RHITextureDesc& desc = GetDesc();
        const RHITextureAspect aspect =
            (desc.Format == RHIFormat::D32Float) ? RHITextureAspect::Depth : RHITextureAspect::Color;

        return GetOrCreateSubresourceView(
            aspect, 0, desc.MipLevels, 0, desc.ArrayLayers,
            RHITextureViewUsageFlags::Sampled);
    }

    RHITextureView* RHITexture::GetOrCreateLayerDepthView(u32 layer)
    {
        const RHITextureDesc& desc = GetDesc();
        XE_ASSERT(layer < desc.ArrayLayers);

        return GetOrCreateSubresourceView(
            RHITextureAspect::Depth,
            0, 1, layer, 1,
            RHITextureViewUsageFlags::DepthAttachment);
    }
}
```

### 5.8 Modify: `Vulkan/VulkanDescriptor.cpp` — bind via view

**Before** (lines 137–165, the `CombinedImageSampler` branch):

```cpp
if (resource.Type == RHIBindingType::CombinedImageSampler)
{
    auto* texture = static_cast<VulkanTexture*>(resource.Texture);
    auto* sampler = static_cast<VulkanSampler*>(resource.Sampler);
    if (texture == nullptr || sampler == nullptr ||
        texture->GetDefaultView() == nullptr || sampler->GetHandle() == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Combined image sampler binding requires a valid Vulkan texture and sampler");
        return false;
    }

    auto* view = static_cast<VulkanTextureView*>(texture->GetDefaultView());
    if (view == nullptr || view->GetHandle() == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Combined image sampler binding requires a valid Vulkan texture view");
        return false;
    }

    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view->GetHandle();
    imageInfo.sampler = sampler->GetHandle();
    imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_Set;
    write.dstBinding = resource.Binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfos.back();
    writes.push_back(write);
    continue;
}
```

**After**:

```cpp
if (resource.Type == RHIBindingType::CombinedImageSampler)
{
    XE_ASSERT(resource.TextureView != nullptr && resource.Sampler != nullptr);
    VulkanDevice& device = static_cast<VulkanDevice&>(resource.TextureView->GetOwnerDevice());

    auto* view = CheckedVulkanCast<VulkanTextureView>(resource.TextureView, device);
    auto* sampler = CheckedVulkanCast<VulkanSampler>(resource.Sampler, device);
    if (view == nullptr || sampler == nullptr || view->GetHandle() == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Combined image sampler binding requires a valid Vulkan texture view and sampler");
        return false;
    }

    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view->GetHandle();
    imageInfo.sampler = sampler->GetHandle();
    imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_Set;
    write.dstBinding = resource.Binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfos.back();
    writes.push_back(write);
    continue;
}
```

The `UniformBuffer` / `StorageBuffer` branches (lines 167–191) are
**unchanged** — buffers do not go through views.

### 5.9 Modify: `Vulkan/VulkanCommandList.cpp::BeginRenderingIfNeeded` — read view

**Before** (lines 271–283):

```cpp
if (!renderToSwapchain)
{
    colorTexture = dynamic_cast<VulkanTexture*>(m_RenderOutput.ColorTarget);
    depthTexture = dynamic_cast<VulkanTexture*>(m_RenderOutput.DepthTarget);
    if (colorTexture == nullptr || !colorTexture->IsValid())
    {
        XENGINE_LOG_ERROR("Offscreen render output requires a valid Vulkan color texture");
        return;
    }

    colorImageView = colorTexture->GetImageView();
}
```

**After**:

```cpp
if (!renderToSwapchain)
{
    // Resolve the colour view: explicit view wins, otherwise the texture's
    // default view.
    RHITextureView* colorView =
        m_RenderOutput.ColorTargetView != nullptr
            ? m_RenderOutput.ColorTargetView
            : (m_RenderOutput.ColorTexture != nullptr
                    ? m_RenderOutput.ColorTexture->GetDefaultView()
                    : nullptr);
    if (colorView == nullptr)
    {
        XENGINE_LOG_ERROR("Offscreen render output requires a valid color texture view");
        return;
    }

    colorTexture = static_cast<VulkanTexture*>(colorView->GetTexture());
    if (colorTexture == nullptr || !colorTexture->IsValid())
    {
        XENGINE_LOG_ERROR("Offscreen render output requires a valid Vulkan color texture");
        return;
    }

    XE_ASSERT(m_Device != nullptr);
    auto* vkColorView = CheckedVulkanCast<VulkanTextureView>(colorView, *m_Device);
    colorImageView = vkColorView->GetHandle();

    // Same resolution for depth.
    RHITextureView* depthView =
        m_RenderOutput.DepthTargetView != nullptr
            ? m_RenderOutput.DepthTargetView
            : (m_RenderOutput.DepthTexture != nullptr
                    ? m_RenderOutput.DepthTexture->GetDefaultView()
                    : nullptr);
    depthTexture = (depthView != nullptr)
        ? static_cast<VulkanTexture*>(depthView->GetTexture())
        : nullptr;
}
```

### 5.10 Modify: `Vulkan/VulkanCommandList.cpp::BeginRenderingIfNeeded` — depth attachment view

**Before** (lines 313–322):

```cpp
VkRenderingAttachmentInfo depthAttachment {};
if (depthTexture != nullptr)
{
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthTexture->GetImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
}
```

**After** — resolve the depth view, then take its handle:

```cpp
VkRenderingAttachmentInfo depthAttachment {};
if (depthTexture != nullptr)
{
    RHITextureView* depthView =
        m_RenderOutput.DepthTargetView != nullptr
            ? m_RenderOutput.DepthTargetView
            : (m_RenderOutput.DepthTexture != nullptr
                    ? m_RenderOutput.DepthTexture->GetDefaultView()
                    : nullptr);
    if (depthView == nullptr)
    {
        XENGINE_LOG_ERROR("Offscreen render output requires a valid depth texture view");
        return;
    }
    XE_ASSERT(m_Device != nullptr);
    auto* vkDepthView = CheckedVulkanCast<VulkanTextureView>(depthView, *m_Device);
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = vkDepthView->GetHandle();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
}
```

### 5.11 Modify Renderer callers — `ForwardOpaquePass`

**Before** (wherever the pass constructs an `RHIRenderOutputDesc`):

```cpp
RHIRenderOutputDesc output;
output.ColorTarget = m_ColorTexture;
output.DepthTarget = m_DepthTexture;
output.Viewport = viewport;
output.ColorFormat = RHIFormat::BGRA8Unorm;
output.DepthFormat = RHIFormat::D32Float;
output.RenderToSwapchain = false;
```

**After**:

```cpp
RHIRenderOutputDesc output;
output.ColorTargetView = m_ColorTexture->GetDefaultView();
output.DepthTargetView = m_DepthTexture->GetDefaultView();
output.Viewport = viewport;
output.ColorFormat = RHIFormat::BGRA8Unorm;
output.DepthFormat = RHIFormat::D32Float;
output.RenderToSwapchain = false;
```

### 5.12 Modify Renderer callers — `RenderMaterialSystem`

Every `RHIBindingResource` constructed in this file currently does:

**Before**:

```cpp
RHIBindingResource resource;
resource.Binding = 0;
resource.Type = RHIBindingType::CombinedImageSampler;
resource.Texture = texture;
resource.Sampler = sampler;
```

**After**:

```cpp
RHIBindingResource resource;
resource.Binding = 0;
resource.Type = RHIBindingType::CombinedImageSampler;
resource.TextureView = texture->GetDefaultView();
resource.Sampler = sampler;
```

The same pattern applies to every `RHIBindingResource` construction in:

```text
Engine/Source/Runtime/Renderer/Private/Resources/RenderMaterialSystem.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp
```

### 5.13 Modify `Shadows/ShadowResourceCache.h` — fill in view fields

**Before** (lines 23–34):

```cpp
struct DirectionalShadowResources
{
    RHITexture* Texture = nullptr;
    //RHITextureView* SampledView = nullptr; // TODO: need implement RHITexture

    //std::array<RHITextureView*, MaxShadowCascades> LayerDepthViews {};

    RHISampler* Sampler = nullptr;
    u32 Resolution = 0;
    u32 CascadeCount = 0;
    RHIFormat Format = RHIFormat::Undefined;
};
```

**After**:

```cpp
struct DirectionalShadowResources
{
    RHITexture*      Texture     = nullptr;
    RHITextureView*  SampledView = nullptr;     // whole-array sampled
    std::array<RHITextureView*, MaxShadowCascades> LayerDepthViews {};
    RHISampler*      Sampler     = nullptr;
    u32 Resolution   = 0;
    u32 CascadeCount = 0;
    RHIFormat Format = RHIFormat::Undefined;
};
```

Stage 9 populates these. Stage 5 only unblocks the type.

### 5.14 CMake

No edits. New files `Private/Resources/RHITexture.cpp` and
`Private/Resources/RHITexture_Views.h` are picked up by `GLOB_RECURSE`.

## 6. Implementation Order

1. Modify `RHIRenderOutputDesc` and `RHIBindingResource` in the public
   headers. Compile to find every call site.
2. Add `RHITexture` view-creation helpers (`GetOrCreateLayerDepthView` etc.)
   and the `RHITexture.cpp` implementation file.
3. Update `VulkanDescriptor.cpp` to consume the view.
4. Update `VulkanCommandList.cpp::BeginRenderingIfNeeded` to consume views.
5. Update every Renderer caller that constructs an `RHIRenderOutputDesc` or
   `RHIBindingResource`. Most will replace `texture` with
   `texture->GetDefaultView()`.
6. Update `ShadowResourceCache.h` to fill in the view fields.
7. Compile and run Editor + Sandbox to confirm parity.

## 7. Verification

- **Build:** All Renderer passes compile.
- **Editor smoke test:** Offscreen color target still binds correctly. The
  offscreen view is the default view of the editor color texture.
- **Sandbox smoke test:** Forward pass, depth-prepass, skybox, tonemap,
  present all still bind their attachments via views.
- **ShadowResourceCache readiness:** Compile `ShadowResourceCache.cpp`
  with a temporary stub populating `SampledView` and `LayerDepthViews` and
  confirm RenderDoc shows the expected image views.
- **RenderDoc:** Confirm no `dynamic_cast` calls remain in
  `VulkanCommandList::BeginRenderingIfNeeded` (only the
  `CheckedVulkanCast<VulkanTextureView>` helper).
- **Vulkan validation:** No new validation-layer warnings.

## 8. Common Mistakes

- Forgetting to call `texture->GetDefaultView()` in a Renderer caller that
  was previously passing the texture directly — compile will catch the
  type mismatch, but it's easy to write `RHIBindingResource { .TextureView
  = nullptr }` accidentally and end up with a null descriptor write.
- Caching views by a partial key (forgetting `usage` flag). Two views of
  the same subresource with different usage flags (sampled vs depth
  attachment) must be distinct cached entries.
- Allowing a `RHITextureView` to outlive its source `RHITexture`. The
  `GetOrCreate*` helpers store the view in a map on the texture; when the
  texture is destroyed, the views are destroyed too. A Renderer-side
  `RHITextureView*` pointer is only safe while the texture is alive.
  Document this prominently.
- Forgetting to migrate one Renderer pass that still uses
  `RHIRenderOutputDesc::ColorTarget` (texture) instead of
  `ColorTargetView`. The compiler will not catch this if the convenience
  field is still present; add a `#pragma message` or assert.

## 9. What This Stage Intentionally Does Not Do

- Does **not** introduce depth-only pipeline. Stage 6.
- Does **not** add a generic per-pass view API (e.g.
  `RenderPassBuilder::UseTextureView(...)`). RenderGraph V1 owns that.
- Does **not** change the descriptor set / bind group layout shapes.
- Does **not** migrate the editor `ImGuiVulkanBackend` from
  `GetNativeImageView` to view-based. Stage 8.
- Does **not** add new shader bindings for shadow sampling. That is
  Stage 9 + a follow-up shader change.
- Does **not** remove `RHITexture::GetNativeImageView`. Stage 8.
- Does **not** remove `RHIDevice::CreateX` wrappers. Stage 8.