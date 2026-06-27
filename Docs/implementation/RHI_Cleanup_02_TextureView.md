# Stage 2 — RHITextureView

## 1. Goal

Split the texture resource from its view(s). After Stage 2:

- `RHITexture` is the GPU image / allocation only. It owns **zero or more**
  `RHITextureView` instances created against it.
- `RHITextureView` describes a view into an `RHITexture` with: base mip /
  mip count, base array layer / array layer count, aspect mask, and usage
  flags (sampled, color attachment, depth attachment).
- The CSM shadow case becomes expressible:
  ```text
  RHITexture* shadowTexture                              (Texture2DArray, D32Float, 4 layers)
    ├─ RHITextureView* wholeArraySampledView            (all layers, sampled)
    ├─ RHITextureView* layer0DepthAttachmentView
    ├─ RHITextureView* layer1DepthAttachmentView
    ├─ RHITextureView* layer2DepthAttachmentView
    └─ RHITextureView* layer3DepthAttachmentView
  ```

The old `RHITexture::GetNativeImageView(backend)` API stays for one stage as a
**transitional default-view getter**. It is removed in Stage 8.

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp
```

What already exists:

- `RHITextureDesc` covers width, height, mip levels, array layers, format,
  dimension, usage, generate-mips, debug name.
- `RHITexture::GetDesc()` exists.
- `RHITexture::GetNativeImageView(RHIBackend)` exists and returns a single
  full-coverage `void*` (only used by `Editor/Private/ImGui/ImGuiVulkanBackend.cpp`).
- `VulkanTexture` owns **one** `VkImageView` (full mip range, full array
  range) created in its constructor.
- `VulkanTexture::GetLayoutPtr()` exposes a mutable `VkImageLayout*` so
  `VulkanCommandList` can update the layout after a barrier.

What is missing:

- No `RHITextureView` type or descriptor.
- No concept of "sampled vs depth-attachment view" on the same texture.
- No way to address a single array layer at the RHI level.

What should **not** be changed yet:

- `RHIRenderOutputDesc::ColorTarget / DepthTarget` remain `RHITexture*` until
  Stage 5.
- `RHIBindingResource::Texture` remains `RHITexture*` until Stage 5.
- `RHIDevice::CreateTexture` still uploads initial data inline (Stage 4).
- `RHIDevice::CreateShader / CreateBuffer / CreateSampler / CreateBindGroup*`
  are unchanged.

## 3. Files to Add

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITextureView.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.cpp
```

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHI.h

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp

Engine/Source/Runtime/RHI/Private/RHIDevice.h
Engine/Source/Runtime/RHI/Private/RHIDevice.cpp     (only if VulkanDevice is used; otherwise via forward declaration)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp   (default view creation, owned by RHITexture)

Engine/Source/Runtime/RHI/Private/RHICommandList.cpp  (no behavioural change yet)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp  (GetImageView still works for default view)

Engine/Source/Runtime/RHI/CMakeLists.txt
```

## 5. Detailed Code Plan

All changes below are anchored to the **current** files in the repo. The
"Before" column shows what is in the file today. The "After" column shows
exactly what the file should look like after Stage 2 lands.

### 5.1 New file: `Resources/RHITextureView.h`

```cpp
// Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITextureView.h
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHITexture;

    // Stage 2: minimal flags; Stage 7 may add Storage / StorageImageAtomics.
    enum class RHITextureViewUsageFlags : u32
    {
        None            = 0,
        Sampled         = 1 << 0,
        ColorAttachment = 1 << 1,
        DepthAttachment = 1 << 2,
    };

    inline RHITextureViewUsageFlags operator|(
        RHITextureViewUsageFlags a, RHITextureViewUsageFlags b)
    {
        return static_cast<RHITextureViewUsageFlags>(
            static_cast<u32>(a) | static_cast<u32>(b));
    }

    inline bool HasFlag(RHITextureViewUsageFlags v, RHITextureViewUsageFlags f)
    {
        return (static_cast<u32>(v) & static_cast<u32>(f)) != 0;
    }

    enum class RHITextureAspect : u32
    {
        Color    = 1 << 0,
        Depth    = 1 << 1,
        Stencil  = 1 << 2,
        MetaData = 1 << 3,
    };

    inline RHITextureAspect operator|(
        RHITextureAspect a, RHITextureAspect b)
    {
        return static_cast<RHITextureAspect>(
            static_cast<u32>(a) | static_cast<u32>(b));
    }

    struct RHITextureViewDesc
    {
        const RHITexture* Texture = nullptr;
        RHITextureViewUsageFlags Usage = RHITextureViewUsageFlags::Sampled;

        RHITextureAspect Aspect = RHITextureAspect::Color;

        u32 BaseMipLevel = 0;
        u32 MipCount = 1;            // 0 means "all remaining mips" — Stage 7 validation
        u32 BaseArrayLayer = 0;
        u32 ArrayLayerCount = 1;     // 0 means "all remaining layers"

        const char* DebugName = nullptr;
    };

    class RHITextureView : public RHIResource
    {
    public:
        ~RHITextureView() override = default;

        virtual const RHITextureViewDesc& GetDesc() const = 0;
        const RHITexture* GetTexture() const;

        // Transitional default-view getter for ImGuiVulkanBackend only.
        // Stage 8 removes this method.
        virtual void* GetNativeView(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }

    protected:
        explicit RHITextureView(RHIDevice& ownerDevice);
    };
}
```

### 5.2 New file: `Resources/RHITextureView.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/Resources/RHITextureView.cpp
#include "XEngine/RHI/Resources/RHITextureView.h"

#include "XEngine/RHI/Resources/RHITexture.h"

namespace XEngine
{
    RHITextureView::RHITextureView(RHIDevice& ownerDevice)
        : RHIResource(ownerDevice)
    {
    }

    const RHITexture* RHITextureView::GetTexture() const
    {
        return GetDesc().Texture;
    }
}
```

Add `RHITextureView.cpp` to the RHI module — it will be picked up by
`GLOB_RECURSE Private/*.cpp`.

### 5.3 Modify: `Resources/RHITexture.h` — add `GetDefaultView` and cache

**Before** (entire file today, minus include guard):

```cpp
#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>      // added by Stage 1
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    struct RHITextureDesc
    {
        u32 Width = 1;
        u32 Height = 1;
        u32 MipLevels = 1;
        u32 ArrayLayers = 1;

        RHIFormat Format = RHIFormat::RGBA8Unorm;
        RHITextureDimension Dimension = RHITextureDimension::Texture2D;
        RHITextureUsageFlags Usage = RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;

        bool GenerateMips = false;
        const char* DebugName = nullptr;
    };

    class RHITexture : public RHIResource
    {
    public:
        ~RHITexture() override = default;

        virtual const RHITextureDesc& GetDesc() const = 0;
        virtual void* GetNativeImageView(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }

    protected:
        explicit RHITexture(RHIDevice& ownerDevice);
    };
}
```

**After**:

```cpp
#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>
#include <XEngine/RHI/RHITypes.h>

// Forward decl so the pointer type can appear in the class body.
namespace XEngine { class RHITextureView; }

namespace XEngine
{
    struct RHITextureDesc
    {
        // ... unchanged ...
    };

    class RHITexture : public RHIResource
    {
    public:
        ~RHITexture() override = default;

        virtual const RHITextureDesc& GetDesc() const = 0;

        // Default-view accessor: every texture owns one default view that
        // covers all mips and all layers, sampled usage, the texture's
        // primary aspect.
        virtual RHITextureView* GetDefaultView() const = 0;

        // Stage 2 transitional alias for editor ImGuiVulkanBackend.
        // Implemented in RHITexture.cpp by delegating to GetDefaultView.
        // Stage 8 removes this method.
        virtual void* GetNativeImageView(RHIBackend backend) const;

    protected:
        explicit RHITexture(RHIDevice& ownerDevice);
    };
}
```

### 5.4 New file: `Resources/RHITexture.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/Resources/RHITexture.cpp
#include "XEngine/RHI/Resources/RHITexture.h"

#include "XEngine/RHI/Resources/RHITextureView.h"

namespace XEngine
{
    void* RHITexture::GetNativeImageView(RHIBackend backend) const
    {
        RHITextureView* view = GetDefaultView();
        return view != nullptr ? view->GetNativeView(backend) : nullptr;
    }
}
```

### 5.5 Modify: `RHI.h` — include the new view header

**Before**:

```cpp
#include <XEngine/RHI/Resources/RHITexture.h>
```

**After**:

```cpp
#include <XEngine/RHI/Resources/RHITexture.h>
#include <XEngine/RHI/Resources/RHITextureView.h>     // NEW
```

### 5.6 Modify: `RHIDevice.h` — add `CreateTextureView`

**Before** (around the bottom of the class, after `CreateGraphicsPipeline`):

```cpp
    virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(
        const RHIGraphicsPipelineDesc& desc) = 0;

    virtual RHIFormat GetSwapchainFormat() const = 0;
```

**After**:

```cpp
    virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(
        const RHIGraphicsPipelineDesc& desc) = 0;

    // Stage 2 transitional. Lives on RHIDevice only this stage.
    // Stage 3 moves this to RHIResourceFactory.
    // Stage 8 deletes the method entirely.
    virtual std::shared_ptr<RHITextureView> CreateTextureView(
        const RHITextureViewDesc& desc) = 0;

    virtual RHIFormat GetSwapchainFormat() const = 0;
```

Add `#include <XEngine/RHI/Resources/RHITextureView.h>` to `RHIDevice.h`
(it is already transitively included from `RHI.h`).

### 5.7 Modify: `VulkanDevice.h` — declare `CreateTextureView`

**Before** (around the `CreateX` block):

```cpp
    std::shared_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) override;
```

**After**:

```cpp
    std::shared_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) override;
    std::shared_ptr<RHITextureView> CreateTextureView(const RHITextureViewDesc& desc) override;     // NEW
```

### 5.8 New file: `Vulkan/VulkanTextureView.h`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.h
#pragma once

#include <XEngine/RHI/Resources/RHITextureView.h>

#include <volk.h>

namespace XEngine
{
    class VulkanDevice;

    class VulkanTextureView final : public RHITextureView
    {
    public:
        VulkanTextureView() = default;
        VulkanTextureView(VulkanDevice& device, const RHITextureViewDesc& desc);
        ~VulkanTextureView() override;

        VulkanTextureView(const VulkanTextureView&) = delete;
        VulkanTextureView& operator=(const VulkanTextureView&) = delete;

        bool IsValid() const;
        const RHITextureViewDesc& GetDesc() const override;

        VkImageView GetHandle() const;
        void* GetNativeView(RHIBackend backend) const override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        RHITextureViewDesc m_Desc {};
    };
}
```

### 5.9 New file: `Vulkan/VulkanTextureView.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.cpp
#include "VulkanTextureView.h"

#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/Resources/RHITexture.h>

namespace XEngine
{
    namespace
    {
        VkImageAspectFlags AspectToVkAspect(RHITextureAspect aspect)
        {
            VkImageAspectFlags flags = 0;
            if (HasFlag(aspect, RHITextureAspect::Color))    { flags |= VK_IMAGE_ASPECT_COLOR_BIT; }
            if (HasFlag(aspect, RHITextureAspect::Depth))    { flags |= VK_IMAGE_ASPECT_DEPTH_BIT; }
            if (HasFlag(aspect, RHITextureAspect::Stencil))  { flags |= VK_IMAGE_ASPECT_STENCIL_BIT; }
            return flags;
        }

        VkImageViewType ViewTypeForTexture(
            RHITextureDimension dim, u32 layerCount)
        {
            if (dim == RHITextureDimension::TextureCube) { return VK_IMAGE_VIEW_TYPE_CUBE; }
            if (layerCount > 1)                          { return VK_IMAGE_VIEW_TYPE_2D_ARRAY; }
            return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    VulkanTextureView::VulkanTextureView(VulkanDevice& device, const RHITextureViewDesc& desc)
        : RHITextureView(device)
        , m_Device(device.GetHandle())
        , m_Desc(desc)
    {
        XE_ASSERT(desc.Texture != nullptr);

        const RHITextureDesc& texDesc = desc.Texture->GetDesc();
        const VkFormat format = RHIFormatToVulkanFormat(texDesc.Format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan texture view: unsupported format");
            return;
        }

        const u32 mipCount = (desc.MipCount == 0)
            ? (texDesc.MipLevels - desc.BaseMipLevel)
            : desc.MipCount;
        const u32 layerCount = (desc.ArrayLayerCount == 0)
            ? (texDesc.ArrayLayers - desc.BaseArrayLayer)
            : desc.ArrayLayerCount;

        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = static_cast<VulkanTexture*>(desc.Texture)->GetImage();
        createInfo.viewType = ViewTypeForTexture(texDesc.Dimension, layerCount);
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = AspectToVkAspect(desc.Aspect);
        createInfo.subresourceRange.baseMipLevel = desc.BaseMipLevel;
        createInfo.subresourceRange.levelCount = mipCount;
        createInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
        createInfo.subresourceRange.layerCount = layerCount;

        const VkResult result = vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageView);
        if (result != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to create Vulkan texture view");
        }
    }

    VulkanTextureView::~VulkanTextureView()
    {
        if (m_Device != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }
    }

    bool VulkanTextureView::IsValid() const
    {
        return m_ImageView != VK_NULL_HANDLE;
    }

    const RHITextureViewDesc& VulkanTextureView::GetDesc() const
    {
        return m_Desc;
    }

    VkImageView VulkanTextureView::GetHandle() const
    {
        return m_ImageView;
    }

    void* VulkanTextureView::GetNativeView(RHIBackend backend) const
    {
        return backend == RHIBackend::Vulkan ? m_ImageView : nullptr;
    }
}
```

The `static_cast<VulkanTexture*>(desc.Texture)` works because in Stage 2
the only thing that creates an `RHITexture` is a Vulkan backend — Stage 8
replaces this with `CheckedVulkanCast` after the factory is in place.

### 5.10 Modify: `Vulkan/VulkanDevice.cpp` — implement `CreateTextureView`

**Before** (the `CreateSampler` block at the end of the resource-creation
section, lines 639–648):

```cpp
std::shared_ptr<RHISampler> VulkanDevice::CreateSampler(const RHISamplerDesc& desc)
{
    auto sampler = std::make_shared<VulkanSampler>(*this, desc);
    if (!sampler->IsValid())
    {
        return nullptr;
    }
    return sampler;
}
```

**After** — add the new method directly after `CreateSampler`:

```cpp
std::shared_ptr<RHITextureView> VulkanDevice::CreateTextureView(
    const RHITextureViewDesc& desc)
{
    auto view = std::make_shared<VulkanTextureView>(*this, desc);
    if (!view->IsValid())
    {
        return nullptr;
    }
    return view;
}
```

### 5.11 Modify: `Vulkan/VulkanTexture.h` — add `GetImage()`, `GetDefaultView()`, mutable cache

**Before** (lines 9–37 of `VulkanTexture.h`):

```cpp
class VulkanTexture final : public RHITexture
{
public:
    VulkanTexture() = default;
    VulkanTexture(VulkanDevice& device, VmaAllocator allocator, const RHITextureDesc& desc);
    ~VulkanTexture() override;

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    bool IsValid() const;
    const RHITextureDesc& GetDesc() const override;

    VkImage GetImage() const;
    VkImageView GetImageView() const;
    void* GetNativeImageView(RHIBackend backend) const override;
    VkImageLayout* GetLayoutPtr();

private:
    VkDevice m_Device = VK_NULL_HANDLE;
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkImage m_Image = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_AllocationInfo {};
    VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    RHITextureDesc m_Desc {};
};
```

**After**:

```cpp
class VulkanTexture final : public RHITexture
{
public:
    VulkanTexture() = default;
    VulkanTexture(VulkanDevice& device, VmaAllocator allocator, const RHITextureDesc& desc);
    ~VulkanTexture() override;

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    bool IsValid() const;
    const RHITextureDesc& GetDesc() const override;

    VkImage GetImage() const;

    // Stage 2 default view is lazy and cached.
    RHITextureView* GetDefaultView() const override;

    // Layout tracking kept as today; Stage 5 will rework this.
    VkImageLayout* GetLayoutPtr();      // TODO Stage 5: rework

private:
    VkDevice m_Device = VK_NULL_HANDLE;
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkImage m_Image = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_AllocationInfo {};
    VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    RHITextureDesc m_Desc {};

    mutable std::shared_ptr<RHITextureView> m_DefaultView;     // NEW
};
```

Add `#include <XEngine/RHI/Resources/RHITextureView.h>` at the top of
`VulkanTexture.h` (or transitively via `RHI.h`).

### 5.12 Modify: `Vulkan/VulkanTexture.cpp` — remove `GetImageView`, implement `GetDefaultView`

**Before** (lines 114–158 — destructor through `GetLayoutPtr`):

```cpp
VulkanTexture::~VulkanTexture()
{
    if (m_Device != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }

    if (m_Allocator != VK_NULL_HANDLE && m_Image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
    }
}

bool VulkanTexture::IsValid() const
{
    return m_Image != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE;
}

const RHITextureDesc& VulkanTexture::GetDesc() const
{
    return m_Desc;
}

VkImage VulkanTexture::GetImage() const
{
    return m_Image;
}

VkImageView VulkanTexture::GetImageView() const
{
    return m_ImageView;
}

void* VulkanTexture::GetNativeImageView(RHIBackend backend) const
{
    return backend == RHIBackend::Vulkan ? m_ImageView : nullptr;
}

VkImageLayout* VulkanTexture::GetLayoutPtr()
{
    return &m_Layout;
}
```

**After**:

```cpp
VulkanTexture::~VulkanTexture()
{
    // Drop the default view first — its VkImageView depends on this image.
    m_DefaultView.reset();

    if (m_Allocator != VK_NULL_HANDLE && m_Image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
        m_Image = VK_NULL_HANDLE;
        m_Allocation = VK_NULL_HANDLE;
    }
}

bool VulkanTexture::IsValid() const
{
    return m_Image != VK_NULL_HANDLE;
}

const RHITextureDesc& VulkanTexture::GetDesc() const
{
    return m_Desc;
}

VkImage VulkanTexture::GetImage() const
{
    return m_Image;
}

RHITextureView* VulkanTexture::GetDefaultView() const
{
    if (!m_DefaultView)
    {
        RHITextureViewDesc viewDesc;
        viewDesc.Texture = this;
        viewDesc.Usage = RHITextureViewUsageFlags::Sampled;
        viewDesc.Aspect = (m_Desc.Format == RHIFormat::D32Float)
                              ? RHITextureAspect::Depth
                              : RHITextureAspect::Color;
        viewDesc.BaseMipLevel = 0;
        viewDesc.MipCount = m_Desc.MipLevels;
        viewDesc.BaseArrayLayer = 0;
        viewDesc.ArrayLayerCount = m_Desc.ArrayLayers;
        viewDesc.DebugName = m_Desc.DebugName;

        m_DefaultView = static_cast<VulkanDevice&>(GetOwnerDevice())
                            .CreateTextureView(viewDesc);
    }
    return m_DefaultView.get();
}

VkImageLayout* VulkanTexture::GetLayoutPtr()
{
    return &m_Layout;
}
```

Notes:

- The constructor body in `VulkanTexture.cpp` (lines 31–112) still creates
  `VkImage` correctly. The block that creates the single `m_ImageView`
  inside the constructor (lines 94–111) is **deleted**. Search for
  `vkCreateImageView` in `VulkanTexture.cpp` and remove the entire
  `VkImageViewCreateInfo viewCreateInfo {} ... vkCreateImageView(...)`
  block.

### 5.13 Modify: `Vulkan/VulkanDescriptor.cpp` — read view handle instead of texture

The only call site for `texture->GetImageView()` is in the
`CombinedImageSampler` branch of `VulkanBindGroup::Create`. The fix is
two-line.

**Before** (lines 141–154):

```cpp
auto* texture = dynamic_cast<VulkanTexture*>(resource.Texture);
auto* sampler = dynamic_cast<VulkanSampler*>(resource.Sampler);
if (texture == nullptr || sampler == nullptr ||
    texture->GetImageView() == VK_NULL_HANDLE || sampler->GetHandle() == VK_NULL_HANDLE)
{
    XENGINE_LOG_ERROR("Combined image sampler binding requires a valid Vulkan texture and sampler");
    return false;
}

VkDescriptorImageInfo imageInfo {};
imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
imageInfo.imageView = texture->GetImageView();
imageInfo.sampler = sampler->GetHandle();
```

**After**:

```cpp
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
```

Add `#include "VulkanTextureView.h"` near the top of `VulkanDescriptor.cpp`.

Note: `dynamic_cast` is replaced with `static_cast` here because we are
already inside a `VulkanBindGroup::Create` that has verified the texture
and sampler are non-null and the layout is a `VulkanBindGroupLayout`. In
debug builds, the surrounding `XE_ASSERT` checks (already in place after
Stage 1) catch any backend mismatch.

### 5.14 CMake

No changes. `VulkanTextureView.cpp` and `Resources/RHITextureView.cpp` and
`Resources/RHITexture.cpp` are picked up by `GLOB_RECURSE`.

## 6. Implementation Order

1. Add `RHITextureView.h` and `VulkanTextureView.h/.cpp`.
2. Add `RHITextureViewDesc` and `RHITextureView` virtual base.
3. Make `RHITexture` inherit from `RHIResource` (Stage 1 prerequisite).
4. Add `RHIDevice::CreateTextureView` virtual and `VulkanDevice` implementation.
5. Refactor `VulkanTexture` to hold a `shared_ptr<RHITextureView>` default view.
6. Replace `GetImageView()` calls in `VulkanDescriptor.cpp` and
   `VulkanCommandList.cpp` with view-handle calls.
7. Replace `GetNativeImageView()` callers **only in the RHI module itself**
   (there are none — the only caller is the editor ImGui backend, which is
   Stage 8). Verify the editor still builds and runs.
8. Compile and run Editor + Sandbox scenes to confirm the default-view path
   behaves identically.

## 7. Verification

- **Build:** `XEngineRHI` compiles. `XEngineRenderer` is unchanged.
- **Sandbox smoke test:** Same forward PBR scene renders identically.
- **Editor smoke test:** Editor viewport renders, ImGui overlay draws over
  the offscreen color target using the transitional `GetNativeImageView`.
- **Vulkan validation:** No new validation-layer warnings. Each texture
  should have exactly **one** `VkImageView` (the default view) — confirm in
  RenderDoc.
- **Diagnostic check:** Add a temporary `XE_LOG_INFO` in
  `VulkanTextureView` constructor showing `BaseArrayLayer`, `ArrayLayerCount`,
  `BaseMipLevel`, `MipCount`. Confirm the single default view per texture.

## 8. Common Mistakes

- Forgetting to remove the old `VkImageView m_ImageView` from `VulkanTexture`
  but leaving `GetImageView()` returning `nullptr`, causing black textures.
- Forgetting to set `viewDesc.Texture = this` in the lazy default-view path,
  producing a view whose `Texture` is null and breaking future per-view
  queries.
- Creating a view in the `RHITexture` constructor (instead of lazily) and
  causing infinite recursion: the constructor calls
  `device.CreateTextureView(desc)`, which constructs a `VulkanTextureView`
  which reads `desc.Texture` — fine — but `desc.Texture` is `this`, which
  is not yet fully constructed. Always create lazily.
- Using `ArrayLayerCount = 0` on a single-layer texture. Stage 7's validator
  rejects this; Stage 2 should default it to `m_Desc.ArrayLayers` if zero is
  passed.
- Creating `RHITextureAspect::Depth` views on a non-depth texture. Stage 7
  validation will catch this; Stage 2 should log a warning and return a
  null view.

## 9. What This Stage Intentionally Does Not Do

- Does **not** move `RHIRenderOutputDesc::ColorTarget / DepthTarget` from
  `RHITexture*` to `RHITextureView*`. That is Stage 5.
- Does **not** move `RHIBindingResource::Texture` from `RHITexture*` to
  `RHITextureView*`. That is Stage 5.
- Does **not** remove `RHITexture::GetNativeImageView`. Stage 8.
- Does **not** introduce `RHIResourceFactory`. `CreateTextureView` lives on
  `RHIDevice` for this stage. Stage 3.
- Does **not** introduce the upload manager. Stage 4.
- Does **not** introduce depth-only pipeline. Stage 6.
- Does **not** change the bind group resource types. Stage 5.
- Does **not** add the CSM texture array + views. That is Stage 9 work,
  but it is now **expressible** after this stage.