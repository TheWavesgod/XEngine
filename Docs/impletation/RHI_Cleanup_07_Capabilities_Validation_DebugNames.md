# Stage 7 — Capabilities, Format Utils, Validation, Debug Names

## 1. Goal

Introduce the diagnostic / safety infrastructure that makes every preceding
stage robust:

- `RHICapabilities` exposes per-backend limits (max sampled-image layers,
  max array layers, supported depth formats, anisotropy range, etc.).
- `RHIFormat` helpers in `RHIUtils` (bytes-per-pixel, depth/stencil-ness,
  aspect resolution, `IsCompressed`, `IsDepth`, `IsStencil`).
- `RHIResourceFactory::CreateTexture(Impl)` validates the descriptor
  against `RHICapabilities`.
- `RHIResourceFactory::CreateTextureView(Impl)` validates subresource
  range against the source texture's description and the capabilities.
- `RHIResourceFactory::CreateBindGroupLayout(Impl)` validates shader
  visibility flags.
- Debug names flow uniformly from descriptor `DebugName` field to
  `vkSetDebugUtilsObjectNameEXT` (via a small helper that looks up the
  function pointer from `VulkanDevice` once at startup).
- A `RHIDeferredDeleter` placeholder is added (a static queue of
  `std::function<void()>` that is drained once per frame at
  `RHIDevice::BeginFrame`). Stage 9 / future stages will use it to safely
  destroy resources that may still be referenced by an in-flight
  command buffer. This stage does **not** wire any resource into it yet —
  it only sets up the API.

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h
  - RHIFormat enum (Undefined, RGBA8Unorm/Srgb, BGRA8Unorm/Srgb,
    RGBA16Float, RGBA32Float, D32Float, R32G32Float, R32G32B32Float,
    R32G32B32A32Float)
  - No aspect helpers, no bytes-per-pixel.

Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUtils.h          (Stage 1 stub)
Engine/Source/Runtime/RHI/Private/RHIUtils.cpp                    (Stage 1 stub)

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUtils.h/.cpp
  - VulkanFormatToRHIFormat, RHIFormatToVulkanFormat,
    ToVulkanImageUsageFlags, ToVulkanFilter, ToVulkanAddressMode,
    ToVulkanDescriptorType, ToVulkanShaderStageFlags,
    XENGINE_VK_CHECK, VulkanResultToString.

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp
  - VulkanSampler.cpp logs "anisotropy feature query is not wired in
    Stage 6A. Creating sampler without anisotropy."

Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp   (Stage 3)
  - currently only minimal validation; no capability reads.
```

What already exists:

- `RHITypes.h` enums.
- `VulkanUtils` translation helpers.
- `XENGINE_VK_CHECK` macro.
- Every descriptor carries a `const char* DebugName` field but only
  `VulkanPipeline` actually logs it (in `vkCreateGraphicsPipelines` call
  site). No Vulkan object is named via `vkSetDebugUtilsObjectNameEXT`.

What is missing:

- `RHICapabilities` struct.
- `RHIDevice::GetCapabilities()`.
- `RHIFormat` helper functions (`IsDepth`, `IsStencil`, `GetAspectMask`,
  `GetBytesPerPixel`).
- A uniform debug-name path.
- A deferred-deleter API.

What should **not** be changed yet:

- `RHITexture::GetNativeImageView` (transitional, Stage 8).
- `RHIDevice::CreateX` wrappers (transitional, Stage 8).
- View-based render pass and bind group (Stage 5).
- Depth-only pipeline (Stage 6).

## 3. Files to Add

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICapabilities.h
Engine/Source/Runtime/RHI/Private/RHIDeferredDeleter.h
Engine/Source/Runtime/RHI/Private/RHIDeferredDeleter.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCapabilities.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCapabilities.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDebugName.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDebugName.cpp
```

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHI.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUtils.h
Engine/Source/Runtime/RHI/Private/RHIUtils.cpp
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp

Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp    (use capabilities + format helpers)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanResourceFactory.cpp  (call debug-name helper)

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanShader.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp
                                                    (each: apply debug name)

Engine/Source/Runtime/RHI/CMakeLists.txt
```

## 5. Detailed Code Plan

### 5.1 New file: `Public/XEngine/RHI/RHICapabilities.h` — full content

```cpp
// Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICapabilities.h
#pragma once

#include <XEngine/Core/Types.h>

#include <vector>

namespace XEngine
{
    // Read-only device capabilities surfaced by RHIDevice::GetCapabilities().
    // Filled once at device init. Capabilities that depend on per-object
    // state (e.g. swapchain) are refreshed only on swapchain recreate.
    struct RHICapabilities
    {
        // Textures.
        u32 MaxTextureDimension2D = 0;
        u32 MaxTextureDimensionCube = 0;
        u32 MaxTextureArrayLayers = 0;
        u32 MaxSamplerAnisotropy = 0;

        // Buffers.
        u64 MaxBufferSize = 0;

        // Push constants.
        u32 MaxPushConstantSize = 0;

        // Descriptor limits.
        u32 MaxBindGroups = 0;
        u32 MaxBindingsPerBindGroup = 0;

        // Reserved for future MSAA / multi-view paths. Stage 7 only stores.
        u32 MaxSampleCount = 1;

        // Feature flags.
        bool SupportsDepthOnlyPipeline = true;
        bool SupportsConservativeRaster = false;
        bool SupportsMultiView = false;
        bool SupportsBindless = false;     // always false in Stage 7
    };
}
```

### 5.2 Modify: `Public/XEngine/RHI/RHI.h` — pull in the header

**Before** (the include block at the top of `RHI.h`):

```cpp
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIUtils.h>
```

**After**:

```cpp
#include <XEngine/RHI/RHICapabilities.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIUtils.h>
```

### 5.3 New file: `Private/Vulkan/VulkanCapabilities.h` — full content

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCapabilities.h
#pragma once

#include "XEngine/RHI/RHICapabilities.h"

#include <vulkan/vulkan.h>

namespace XEngine
{
    class VulkanPhysicalDevice;

    RHICapabilities BuildVulkanCapabilities(
        VkPhysicalDevice physicalDevice,
        const VulkanPhysicalDevice& physicalDeviceContext);
}
```

### 5.4 New file: `Private/Vulkan/VulkanCapabilities.cpp` — full content

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCapabilities.cpp
#include "VulkanCapabilities.h"

#include "VulkanPhysicalDevice.h"

#include <algorithm>

namespace XEngine
{
    RHICapabilities BuildVulkanCapabilities(
        VkPhysicalDevice physicalDevice,
        const VulkanPhysicalDevice& physicalDeviceContext)
    {
        RHICapabilities caps;

        VkPhysicalDeviceProperties properties {};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        caps.MaxTextureDimension2D    = properties.limits.maxImageDimension2D;
        caps.MaxTextureDimensionCube   = properties.limits.maxImageDimensionCube;
        caps.MaxTextureArrayLayers    = properties.limits.maxImageArrayLayers;
        caps.MaxSamplerAnisotropy      = static_cast<u32>(properties.limits.maxSamplerAnisotropy);
        caps.MaxPushConstantSize      = properties.limits.maxPushConstantsSize;
        caps.MaxSampleCount            = static_cast<u32>(properties.limits.framebufferColorSampleCounts
                                                       & properties.limits.framebufferDepthSampleCounts);
        caps.MaxSampleCount            = std::max<u32>(1, caps.MaxSampleCount);
        caps.MaxBufferSize             = static_cast<u64>(properties.limits.maxStorageBufferRange);

        // Per-stage descriptor limits aggregate to "max bindings per set".
        caps.MaxBindingsPerBindGroup =
            properties.limits.maxPerStageDescriptorSampledImages
          + properties.limits.maxPerStageDescriptorUniformBuffers
          + properties.limits.maxPerStageDescriptorStorageBuffers
          + properties.limits.maxPerStageDescriptorSampledImages
          + properties.limits.maxPerStageDescriptorStorageImages;

        // Vulkan 1.0+ always supports depth-only pipelines via dynamic
        // rendering (VK_KHR_dynamic_rendering). Stage 7 just asserts.
        caps.SupportsDepthOnlyPipeline = true;
        caps.SupportsBindless = false;

        return caps;
    }
}
```

(Vulkan 1.2 / 1.3 paths can be added later to read
`VkPhysicalDeviceVulkan12Properties` for `maxBindGroups` and friends. The
above covers what Stage 7 needs.)

### 5.5 Modify: `Private/Vulkan/VulkanDevice.h` — cache + accessors

**Before** (the private section of `VulkanDevice`):

```cpp
private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    // ... many more members ...
    std::unique_ptr<RHIResourceFactory> m_ResourceFactory;
    std::unique_ptr<RHIUploadManager>   m_UploadManager;
};
```

**After**:

```cpp
private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    // ... many more members ...
    std::unique_ptr<RHIResourceFactory> m_ResourceFactory;
    std::unique_ptr<RHIUploadManager>   m_UploadManager;

    RHICapabilities m_Capabilities {};
    RHIDeferredDeleter m_DeferredDeleter;

    PFN_vkSetDebugUtilsObjectNameEXT m_PfnSetDebugName = nullptr;
};
```

Add at the top of the header:

```cpp
#include "XEngine/RHI/RHICapabilities.h"
#include "RHIDeferredDeleter.h"
```

In the public section:

```cpp
    const RHICapabilities& GetCapabilities() const override { return m_Capabilities; }
    RHIDeferredDeleter&    GetDeferredDeleter() override     { return m_DeferredDeleter; }

    PFN_vkSetDebugUtilsObjectNameEXT GetDebugNameFn() const { return m_PfnSetDebugName; }
```

### 5.6 Modify: `Private/Vulkan/VulkanDevice.cpp::Initialize` — populate caps and fn

**Before** (the bottom of `Initialize`, just before creating the
resource factory):

```cpp
    m_ResourceFactory = std::make_unique<VulkanResourceFactory>(*this, m_Allocator, m_DescriptorPool);
    m_UploadManager   = std::make_unique<VulkanUploadManager>(*this, m_CommandPool);
```

**After**:

```cpp
    m_Capabilities = BuildVulkanCapabilities(m_PhysicalDevice, m_PhysicalDeviceCtx);
    XE_LOG_INFO("RHI capabilities: maxImageDimension2D=%u maxImageArrayLayers=%u "
                "maxSamplerAnisotropy=%u maxPushConstantsSize=%u maxBindGroups=%u",
                m_Capabilities.MaxTextureDimension2D,
                m_Capabilities.MaxTextureArrayLayers,
                m_Capabilities.MaxSamplerAnisotropy,
                m_Capabilities.MaxPushConstantSize,
                m_Capabilities.MaxBindGroups);

    m_PfnSetDebugName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT"));
    if (m_PfnSetDebugName == nullptr)
    {
        XE_LOG_WARN("vkSetDebugUtilsObjectNameEXT not available; Vulkan debug names disabled");
    }

    m_ResourceFactory = std::make_unique<VulkanResourceFactory>(*this, m_Allocator, m_DescriptorPool);
    m_UploadManager   = std::make_unique<VulkanUploadManager>(*this, m_CommandPool);
```

`m_PhysicalDeviceCtx` is the Stage 1 type alias / wrapper the device
already uses to hold feature/extension state. If that wrapper does not
exist in the current codebase, replace the call with:

```cpp
    m_Capabilities = BuildVulkanCapabilities(m_PhysicalDevice, *this);
```

and adjust the function signature accordingly.

### 5.7 Modify: `Public/XEngine/RHI/RHIDevice.h` — surface capabilities + deleter

**Before** (the public section of `RHIDevice`):

```cpp
    class RHI_API RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;
        // ... factory, upload, etc. ...
    };
```

**After**:

```cpp
    class RHIResourceFactory;
    class RHIUploadManager;
    class RHIDeferredDeleter;     // fwd decl

    class RHI_API RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        // ... existing factory / upload / BeginFrame accessors ...

        virtual const RHICapabilities& GetCapabilities() const = 0;
        virtual RHIDeferredDeleter&    GetDeferredDeleter() = 0;
    };
```

Add the include at the top:

```cpp
#include "XEngine/RHI/RHICapabilities.h"
```

### 5.8 New file: `Private/RHIDeferredDeleter.h` — full content

```cpp
// Engine/Source/Runtime/RHI/Private/RHIDeferredDeleter.h
#pragma once

#include <functional>
#include <vector>

namespace XEngine
{
    // Stage 7 placeholder. Drains a list of destruction callbacks once per
    // frame at BeginFrame. Future stages will route resource destruction
    // through it.
    class RHIDeferredDeleter
    {
    public:
        void Enqueue(std::function<void()> destroy);
        void Flush();
        bool IsEmpty() const;

    private:
        std::vector<std::function<void()>> m_Pending;
    };
}
```

### 5.9 New file: `Private/RHIDeferredDeleter.cpp` — full content

```cpp
// Engine/Source/Runtime/RHI/Private/RHIDeferredDeleter.cpp
#include "RHIDeferredDeleter.h"

#include <XEngine/Logging/Log.h>

namespace XEngine
{
    void RHIDeferredDeleter::Enqueue(std::function<void()> destroy)
    {
        m_Pending.push_back(std::move(destroy));
    }

    void RHIDeferredDeleter::Flush()
    {
        if (m_Pending.empty()) return;

        for (auto& fn : m_Pending)
        {
            if (fn) fn();
        }
        m_Pending.clear();
    }

    bool RHIDeferredDeleter::IsEmpty() const
    {
        return m_Pending.empty();
    }
}
```

### 5.10 Modify: `Private/Vulkan/VulkanDevice.cpp::BeginFrame` — drain

**Before** (the top of `BeginFrame`):

```cpp
void VulkanDevice::BeginFrame()
{
    // (existing acquire / fence handling)
}
```

**After**:

```cpp
void VulkanDevice::BeginFrame()
{
    // Stage 7: drain any deferred-deletion work scheduled by previous frames.
    // This must run BEFORE the renderer submits new commands.
    m_DeferredDeleter.Flush();

    // (existing acquire / fence handling)
}
```

### 5.11 Modify: `Public/XEngine/RHI/RHIUtils.h` — format helpers

**Before** (the Stage 1 stub):

```cpp
namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend);
    const char* RHIFormatToString(RHIFormat format);
}
```

**After**:

```cpp
namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend);
    const char* RHIFormatToString(RHIFormat format);

    bool        IsDepthFormat(RHIFormat format);
    bool        IsStencilFormat(RHIFormat format);
    bool        IsSrgbFormat(RHIFormat format);
    u32         GetBytesPerPixel(RHIFormat format);
    u32         GetMaxMipLevels(u32 width, u32 height);
    RHITextureAspect GetDefaultAspect(RHIFormat format);
}
```

Add include for the aspect enum:

```cpp
#include "XEngine/RHI/Resources/RHITextureView.h"
```

### 5.12 New file (or extension to): `Private/RHIUtils.cpp` — implementations

```cpp
// Engine/Source/Runtime/RHI/Private/RHIUtils.cpp
#include "XEngine/RHI/RHIUtils.h"

#include <XEngine/Core/Assert.h>

namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend)
    {
        switch (backend)
        {
            case RHIBackend::Vulkan:  return "Vulkan";
            case RHIBackend::D3D12:   return "D3D12";
            case RHIBackend::Metal:   return "Metal";
            default:                  return "Unknown";
        }
    }

    const char* RHIFormatToString(RHIFormat format)
    {
        switch (format)
        {
            case RHIFormat::RGBA8Unorm:    return "RGBA8Unorm";
            case RHIFormat::RGBA8Srgb:     return "RGBA8Srgb";
            case RHIFormat::BGRA8Unorm:    return "BGRA8Unorm";
            case RHIFormat::BGRA8Srgb:     return "BGRA8Srgb";
            case RHIFormat::RGBA16Float:   return "RGBA16Float";
            case RHIFormat::RGBA32Float:   return "RGBA32Float";
            case RHIFormat::R32G32Float:   return "R32G32Float";
            case RHIFormat::R32G32B32Float:    return "R32G32B32Float";
            case RHIFormat::R32G32B32A32Float: return "R32G32B32A32Float";
            case RHIFormat::D32Float:      return "D32Float";
            case RHIFormat::D24UnormS8Uint:return "D24UnormS8Uint";
            case RHIFormat::D32FloatS8Uint:return "D32FloatS8Uint";
            default:                       return "Undefined";
        }
    }

    bool IsDepthFormat(RHIFormat format)
    {
        return format == RHIFormat::D32Float
            || format == RHIFormat::D24UnormS8Uint
            || format == RHIFormat::D32FloatS8Uint;
    }

    bool IsStencilFormat(RHIFormat format)
    {
        return format == RHIFormat::D24UnormS8Uint
            || format == RHIFormat::D32FloatS8Uint;
    }

    bool IsSrgbFormat(RHIFormat format)
    {
        return format == RHIFormat::RGBA8Srgb
            || format == RHIFormat::BGRA8Srgb;
    }

    u32 GetBytesPerPixel(RHIFormat format)
    {
        switch (format)
        {
            case RHIFormat::RGBA8Unorm:
            case RHIFormat::RGBA8Srgb:
            case RHIFormat::BGRA8Unorm:
            case RHIFormat::BGRA8Srgb:
                return 4;
            case RHIFormat::RGBA16Float:
                return 8;
            case RHIFormat::RGBA32Float:
                return 16;
            case RHIFormat::R32G32Float:
                return 8;
            case RHIFormat::R32G32B32Float:
                return 12;
            case RHIFormat::R32G32B32A32Float:
                return 16;
            case RHIFormat::D32Float:
                return 4;
            case RHIFormat::D24UnormS8Uint:
                return 4;
            case RHIFormat::D32FloatS8Uint:
                return 8;
            default:
                XE_ASSERT(false);
                return 0;
        }
    }

    u32 GetMaxMipLevels(u32 width, u32 height)
    {
        u32 max = (width > height) ? width : height;
        u32 levels = 1;
        while (max > 1) { max >>= 1; ++levels; }
        return levels;
    }

    RHITextureAspect GetDefaultAspect(RHIFormat format)
    {
        if (IsDepthFormat(format))
        {
            return IsStencilFormat(format)
                ? (RHITextureAspect::Depth | RHITextureAspect::Stencil)
                : RHITextureAspect::Depth;
        }
        return RHITextureAspect::Color;
    }
}
```

### 5.13 Modify: `Private/RHIResourceFactory.cpp` — capability-aware validation

**Before** (the `CreateTexture` wrapper):

```cpp
std::unique_ptr<RHITexture> RHIResourceFactory::CreateTexture(const RHITextureDesc& desc)
{
    // (Stage 3 basic null / zero checks)
    if (desc.DebugName == nullptr) desc.DebugName = "UnnamedTexture";
    return CreateTextureImpl(desc);
}
```

**After**:

```cpp
std::unique_ptr<RHITexture> RHIResourceFactory::CreateTexture(const RHITextureDesc& desc)
{
    const RHICapabilities& caps = m_Device.GetCapabilities();

    if (desc.Width == 0 || desc.Height == 0)
    {
        XENGINE_LOG_ERROR("CreateTexture rejected: zero width/height (%u, %u)", desc.Width, desc.Height);
        return nullptr;
    }
    if (desc.Width > caps.MaxTextureDimension2D || desc.Height > caps.MaxTextureDimension2D)
    {
        XENGINE_LOG_ERROR("CreateTexture rejected: %ux%u exceeds maxImageDimension2D=%u",
            desc.Width, desc.Height, caps.MaxTextureDimension2D);
        return nullptr;
    }
    if (desc.ArrayLayers > caps.MaxTextureArrayLayers)
    {
        XENGINE_LOG_ERROR("CreateTexture rejected: ArrayLayers=%u exceeds maxImageArrayLayers=%u",
            desc.ArrayLayers, caps.MaxTextureArrayLayers);
        return nullptr;
    }
    const u32 maxMips = GetMaxMipLevels(desc.Width, desc.Height);
    if (desc.MipLevels == 0 || desc.MipLevels > maxMips)
    {
        XENGINE_LOG_ERROR("CreateTexture rejected: MipLevels=%u out of range (1..%u)",
            desc.MipLevels, maxMips);
        return nullptr;
    }
    if (desc.Usage == RHITextureUsageFlags::None)
    {
        XENGINE_LOG_ERROR("CreateTexture rejected: Usage is None (texture has no purpose)");
        return nullptr;
    }

    const RHITextureAspect aspect = GetDefaultAspect(desc.Format);
    const bool formatIsDepth = IsDepthFormat(desc.Format);
    if (formatIsDepth && !HasAllFlags(desc.Usage, RHITextureUsageFlags::DepthAttachment))
    {
        XENGINE_LOG_WARN("CreateTexture: depth format %s without DepthAttachment usage",
            RHIFormatToString(desc.Format));
    }
    if (!formatIsDepth && HasAllFlags(desc.Usage, RHITextureUsageFlags::DepthAttachment))
    {
        XENGINE_LOG_ERROR("CreateTexture rejected: DepthAttachment usage on non-depth format %s",
            RHIFormatToString(desc.Format));
        return nullptr;
    }

    return CreateTextureImpl(desc);
}
```

Add the missing helpers at the top of `RHIResourceFactory.cpp`:

```cpp
#include "XEngine/RHI/RHIUtils.h"
```

`m_Device` is the cached `RHIDevice&` Stage 3's factory holds.

### 5.14 Modify: `Private/RHIResourceFactory.cpp::CreateTextureView` — validate range

**Before**:

```cpp
std::unique_ptr<RHITextureView> RHIResourceFactory::CreateTextureView(
    const RHITextureViewDesc& desc)
{
    return CreateTextureViewImpl(desc);
}
```

**After**:

```cpp
std::unique_ptr<RHITextureView> RHIResourceFactory::CreateTextureView(
    const RHITextureViewDesc& desc)
{
    if (desc.Texture == nullptr)
    {
        XENGINE_LOG_ERROR("CreateTextureView rejected: source texture is null");
        return nullptr;
    }

    const RHITextureDesc& src = desc.Texture->GetDesc();
    if (desc.BaseMipLevel >= src.MipLevels)
    {
        XENGINE_LOG_ERROR("CreateTextureView rejected: BaseMipLevel=%u out of range (MipLevels=%u)",
            desc.BaseMipLevel, src.MipLevels);
        return nullptr;
    }
    if (desc.MipCount == 0 || desc.BaseMipLevel + desc.MipCount > src.MipLevels)
    {
        XENGINE_LOG_ERROR("CreateTextureView rejected: MipCount=%u exceeds MipLevels=%u",
            desc.MipCount, src.MipLevels);
        return nullptr;
    }
    if (desc.BaseArrayLayer >= src.ArrayLayers)
    {
        XENGINE_LOG_ERROR("CreateTextureView rejected: BaseArrayLayer=%u out of range (ArrayLayers=%u)",
            desc.BaseArrayLayer, src.ArrayLayers);
        return nullptr;
    }
    if (desc.ArrayLayerCount == 0 || desc.BaseArrayLayer + desc.ArrayLayerCount > src.ArrayLayers)
    {
        XENGINE_LOG_ERROR("CreateTextureView rejected: ArrayLayerCount=%u exceeds ArrayLayers=%u",
            desc.ArrayLayerCount, src.ArrayLayers);
        return nullptr;
    }
    if (desc.ArrayLayerCount > m_Device.GetCapabilities().MaxTextureArrayLayers)
    {
        XENGINE_LOG_ERROR("CreateTextureView rejected: ArrayLayerCount=%u exceeds device limit=%u",
            desc.ArrayLayerCount, m_Device.GetCapabilities().MaxTextureArrayLayers);
        return nullptr;
    }

    return CreateTextureViewImpl(desc);
}
```

### 5.15 Modify: `Private/RHIResourceFactory.cpp::CreateBindGroupLayout` — flag gate

**Before**:

```cpp
std::unique_ptr<RHIBindGroupLayout> RHIResourceFactory::CreateBindGroupLayout(
    const RHIBindGroupLayoutDesc& desc)
{
    if (desc.Bindings.empty()) { XENGINE_LOG_ERROR("..."); return nullptr; }
    return CreateBindGroupLayoutImpl(desc);
}
```

**After**:

```cpp
std::unique_ptr<RHIBindGroupLayout> RHIResourceFactory::CreateBindGroupLayout(
    const RHIBindGroupLayoutDesc& desc)
{
    if (desc.Bindings.empty())
    {
        XENGINE_LOG_ERROR("CreateBindGroupLayout rejected: Bindings is empty");
        return nullptr;
    }
    for (const auto& b : desc.Bindings)
    {
        if (b.Count > 1 && !m_Device.GetCapabilities().SupportsBindless)
        {
            XENGINE_LOG_ERROR("CreateBindGroupLayout rejected: array binding Count=%u but bindless is not supported",
                b.Count);
            return nullptr;
        }
    }
    return CreateBindGroupLayoutImpl(desc);
}
```

### 5.16 Modify: `Private/RHIResourceFactory.cpp::CreateGraphicsPipeline` — push constant + bind group limits

**Before** (the existing wrapper after the Stage 6 changes):

```cpp
std::unique_ptr<RHIGraphicsPipeline> RHIResourceFactory::CreateGraphicsPipeline(
    const RHIGraphicsPipelineDesc& desc)
{
    // ... Stage 6 validation ...
    return CreateGraphicsPipelineImpl(desc);
}
```

**After** — append two more checks:

```cpp
    const RHICapabilities& caps = m_Device.GetCapabilities();
    if (desc.PushConstantSize > caps.MaxPushConstantSize)
    {
        XENGINE_LOG_ERROR("CreateGraphicsPipeline rejected: PushConstantSize=%u exceeds MaxPushConstantSize=%u",
            desc.PushConstantSize, caps.MaxPushConstantSize);
        return nullptr;
    }
    if (desc.BindGroupLayouts.size() > caps.MaxBindGroups)
    {
        XENGINE_LOG_ERROR("CreateGraphicsPipeline rejected: %zu bind groups exceeds MaxBindGroups=%u",
            desc.BindGroupLayouts.size(), caps.MaxBindGroups);
        return nullptr;
    }
    return CreateGraphicsPipelineImpl(desc);
```

### 5.17 New file: `Private/Vulkan/VulkanDebugName.h`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDebugName.h
#pragma once

#include <vulkan/vulkan.h>

#include <XEngine/Core/Types.h>

namespace XEngine
{
    class VulkanDevice;

    void VulkanSetDebugName(
        VulkanDevice& device,
        VkObjectType objectType,
        u64 objectHandle,
        const char* debugName);
}
```

### 5.18 New file: `Private/Vulkan/VulkanDebugName.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDebugName.cpp
#include "VulkanDebugName.h"

#include "VulkanDevice.h"

namespace XEngine
{
    void VulkanSetDebugName(
        VulkanDevice& device,
        VkObjectType objectType,
        u64 objectHandle,
        const char* debugName)
    {
        if (debugName == nullptr) return;

        PFN_vkSetDebugUtilsObjectNameEXT fn = device.GetDebugNameFn();
        if (fn == nullptr) return;

        VkDebugUtilsObjectNameInfoEXT info {};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType = objectType;
        info.objectHandle = objectHandle;
        info.pObjectName = debugName;
        fn(device.GetHandle(), &info);
    }
}
```

`VulkanDevice::GetHandle()` is the Stage 1 accessor returning
`VkDevice`.

### 5.19 Modify: every `VulkanX` constructor — apply debug name

Apply this same shape to each `VulkanX` constructor after the handle has
been successfully created.

**`VulkanBuffer.cpp` — after `vkCreateBuffer` succeeds**:

**Before**:
```cpp
VulkanBuffer::VulkanBuffer(
    VulkanDevice& device,
    VkBuffer buffer,
    VmaAllocation allocation,
    const RHIBufferDesc& desc)
    : RHIBuffer(device)
    , m_Device(device.GetHandle())
    , m_Buffer(buffer)
    , m_Allocation(allocation)
{
    (void)desc;
}
```

**After**:
```cpp
VulkanBuffer::VulkanBuffer(
    VulkanDevice& device,
    VkBuffer buffer,
    VmaAllocation allocation,
    const RHIBufferDesc& desc)
    : RHIBuffer(device)
    , m_Device(device.GetHandle())
    , m_Buffer(buffer)
    , m_Allocation(allocation)
{
    VulkanSetDebugName(device, VK_OBJECT_TYPE_BUFFER, (u64)m_Buffer, desc.DebugName);
}
```

**`VulkanTexture.cpp`** — after `vkCreateImage` succeeds:

**Before**:
```cpp
VulkanTexture::VulkanTexture(
    VulkanDevice& device,
    VkImage image,
    VmaAllocation allocation,
    const RHITextureDesc& desc)
    : RHITexture(device)
    , m_Device(device.GetHandle())
    , m_Image(image)
    , m_Allocation(allocation)
{
    // ... create m_DefaultView (Stage 2) ...
}
```

**After**:
```cpp
    // ... create m_DefaultView (Stage 2) ...
    VulkanSetDebugName(device, VK_OBJECT_TYPE_IMAGE, (u64)m_Image, desc.DebugName);
}
```

**`VulkanShader.cpp`** — after `vkCreateShaderModule`:

```cpp
VulkanSetDebugName(device, VK_OBJECT_TYPE_SHADER_MODULE, (u64)m_ShaderModule, desc.DebugName);
```

**`VulkanPipeline.cpp`** — after `vkCreateGraphicsPipelines` succeeds.
Replace the existing `XENGINE_LOG_INFO("Pipeline created: %s", desc.DebugName);` line:

**Before**:
```cpp
XENGINE_LOG_INFO("Pipeline created: %s", desc.DebugName);
```

**After**:
```cpp
VulkanSetDebugName(device, VK_OBJECT_TYPE_PIPELINE, (u64)m_Pipeline, desc.DebugName);
if (desc.DebugName != nullptr) { /* keep the LOG_INFO at INFO level if desired */ }
```

For layout, do similarly in `VulkanBindGroupLayout::Create`:

**Before**:
```cpp
XENGINE_LOG_INFO("Bind group layout created: %s", desc.DebugName);
```

**After**:
```cpp
VulkanSetDebugName(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)m_PipelineLayout, desc.DebugName);
```

**`VulkanSampler.cpp`** — after `vkCreateSampler`:

```cpp
VulkanSetDebugName(device, VK_OBJECT_TYPE_SAMPLER, (u64)m_Sampler, desc.DebugName);
```

**`VulkanTextureView.cpp`** — after `vkCreateImageView`:

```cpp
VulkanSetDebugName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (u64)m_ImageView, desc.DebugName);
```

**`VulkanDescriptor.cpp`** — after `vkAllocateDescriptorSets` succeeds:

```cpp
VulkanSetDebugName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (u64)m_Set, desc.DebugName);
```

### 5.20 Modify: `Private/Vulkan/VulkanSampler.cpp` — wire anisotropy

**Before** (the `vkCreateSampler` call):

```cpp
samplerInfo.anisotropyEnable = VK_FALSE;     // "anisotropy not wired in Stage 6A"
// ...
samplerInfo.maxAnisotropy = 1.0f;
```

**After**:

```cpp
const RHICapabilities& caps = device.GetCapabilities();
const bool useAniso = (desc.MaxAnisotropy > 1.0f) && (caps.MaxSamplerAnisotropy > 1);
samplerInfo.anisotropyEnable = useAniso ? VK_TRUE : VK_FALSE;
samplerInfo.maxAnisotropy = useAniso
    ? std::min(desc.MaxAnisotropy, static_cast<float>(caps.MaxSamplerAnisotropy))
    : 1.0f;
```

Add at the top of `VulkanSampler.cpp`:

```cpp
#include <algorithm>
#include "XEngine/RHI/RHICapabilities.h"
```

### 5.21 CMake

No edits. All new files are picked up by `GLOB_RECURSE`:

```text
Public/XEngine/RHI/RHICapabilities.h
Private/RHIDeferredDeleter.h
Private/RHIDeferredDeleter.cpp
Private/Vulkan/VulkanCapabilities.h
Private/Vulkan/VulkanCapabilities.cpp
Private/Vulkan/VulkanDebugName.h
Private/Vulkan/VulkanDebugName.cpp
```

## 6. Implementation Order

1. Add `RHICapabilities.h` and `RHIUtils` format helpers.
2. Add `VulkanCapabilities` and populate it in `VulkanDevice::Initialize`.
3. Add `RHIDevice::GetCapabilities()` and `GetDeferredDeleter()`.
4. Add `RHIDeferredDeleter` and `VulkanDevice::BeginFrame` drains it.
5. Add `VulkanDebugName` helper and wire every Vulkan constructor to call
   it. Remove redundant `XENGINE_LOG_INFO(desc.DebugName)` lines.
6. Extend `RHIResourceFactory` validation one descriptor at a time
   (`CreateTexture`, `CreateTextureView`, `CreateBindGroupLayout`,
   `CreateGraphicsPipeline`).
7. Compile and run Editor + Sandbox. RenderDoc should show meaningful
   names on every Vulkan object.

## 7. Verification

- **Build:** Compiles. No public ABI break.
- **Editor / Sandbox smoke test:** Identical rendering.
- **Vulkan validation:** No new validation-layer warnings.
- **RenderDoc:** Every Vulkan object (image, buffer, sampler, pipeline,
  layout, descriptor set) should have a debug name from the descriptor.
- **Capabilities:** Add a temporary `XE_LOG_INFO` of every field at
  startup. Confirm values are non-zero for a real GPU.
- **Anisotropy sanity:** Set `RHISamplerDesc::MaxAnisotropy > 1.0f` and
  confirm the sampler is created with `anisotropyEnable = VK_TRUE` and
  `maxAnisotropy` clamped to `caps.MaxSamplerAnisotropy`. Replace the
  existing "anisotropy not wired in Stage 6A" warning.
- **Deferred deleter:** Add a temporary `Enqueue([] { XE_LOG_INFO("flush"); });`
  test in `BeginFrame`. Confirm the log fires once per frame.

## 8. Common Mistakes

- Reading `VkPhysicalDeviceProperties` after `volkInitialize` but before
  `vkCreateDevice`. Capabilities must be captured **before** the device
  is created or after both device and physical device are alive.
- Forgetting to refresh capabilities after a swapchain recreate (the
  swapchain can affect `MaxBindGroups` indirectly — in practice it does
  not, but if any capability ever depends on a created object, document
  the refresh rule).
- Passing `debugName == nullptr` to `vkSetDebugUtilsObjectNameEXT` —
  Vulkan spec allows null but the helper should no-op.
- Using `RHICapabilities` to silently shrink user request sizes (e.g.
  `ArrayLayers = std::min(desc.ArrayLayers, caps.MaxTextureArrayLayers)`).
  The factory must reject instead.
- Putting the deferred deleter drain **after** `vkAcquireNextImageKHR`
  in `BeginFrame`. It must run before the user submits new commands.

## 9. What This Stage Intentionally Does Not Do

- Does **not** route resource destruction through `RHIDeferredDeleter`.
  Future stage.
- Does **not** implement bindless. Capabilities reserve the flag, but the
  factory still rejects `Count > 1` bindings.
- Does **not** implement MSAA / multi-view / conservative raster paths.
  Capabilities reserve the flags.
- Does **not** implement a full resource-state tracker. Capabilities are
  read-only.
- Does **not** remove `RHITexture::GetNativeImageView` or the
  `RHIDevice::CreateX` wrappers. Stage 8.
- Does **not** migrate the editor ImGui backend. Stage 8.
- Does **not** add an `RHIDevice::WaitIdle` semantic change. Existing
  behaviour stays.