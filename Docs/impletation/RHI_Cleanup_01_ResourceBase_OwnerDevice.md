# Stage 1 — RHIResource Base and Owner Device

## 1. Goal

Introduce a common `RHIResource` base class that:

- Stores an `RHIDevice*` owner pointer.
- Exposes `GetOwnerDevice()` and `GetBackend()` uniformly.
- Provides a non-virtual backend helper hook point that derived classes use
  to assert backend compatibility.

Then introduce a free-function helper `CheckedVulkanCast<T>()` that
debug-build-asserts owner / backend match and `static_cast`s in release, so
the existing `dynamic_cast` sites in `VulkanCommandList`,
`VulkanPipeline`, and `VulkanDescriptor` can be replaced one-for-one without
changing observable behaviour.

After Stage 1, **no public header changes are user-visible**. All resource
classes still expose the same API. Only the internal type hierarchy changes.

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBuffer.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIShader.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHISampler.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBindGroup.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanShader.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.h
```

What already exists:

- All resource base classes (`RHIBuffer`, `RHITexture`, `RHIShader`,
  `RHIPipeline`, `RHISampler`, `RHIBindGroup`, `RHIBindGroupLayout`) are
  independent virtual classes. None of them shares an `RHIResource` base.
- `RHIDevice::GetBackend()` already exists.
- `VulkanTexture` and `VulkanSampler` already store the `VkDevice` they
  were created on. Other Vulkan classes (`VulkanPipeline`, `VulkanBuffer`,
  `VulkanShader`, `VulkanBindGroup`, `VulkanBindGroupLayout`) also store
  `VkDevice` or `VkDescriptorPool`. None of them stores an `RHIDevice*`.

What is missing:

- A common owner-device pointer shared across all RHI resources.
- A common backend-query method on every RHI resource.
- A `CheckedVulkanCast<T>` helper that asserts at debug time and
  `static_cast`s in release.

What should **not** be changed yet:

- Do not move `CreateXXX` off `RHIDevice` yet (Stage 3).
- Do not introduce `RHITextureView` yet (Stage 2).
- Do not introduce `RHIResourceFactory` yet (Stage 3).
- Do not change `RHIDevice::GetVulkanNativeContext` / `RenderVulkanOverlay`
  yet (used by the editor ImGui backend).

## 3. Files to Add

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIResource.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUtils.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCheckedCast.h
```

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBuffer.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIShader.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHISampler.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBindGroup.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHI.h

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanShader.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.h

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.cpp

Engine/Source/Runtime/RHI/Private/RHISystem.cpp   (drop device into default resources)
Engine/Source/Runtime/RHI/Private/RHIDevice.cpp   (delete fallthrough device handle holder if any)
```

## 5. Detailed Code Plan

All changes below are anchored to the **current** files in the repo. The
"Before" column shows what is in the file today. The "After" column shows
exactly what the file should look like after Stage 1 lands. Delete the
"Before" lines; insert the "After" lines at the same location.

### 5.1 New file: `RHIResource.h`

Create a brand new file. There is no "Before" because the file does not
exist yet.

```cpp
// Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIResource.h
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIDevice;

    // Common base for every long-lived RHI object that is owned by an RHIDevice.
    // Stage 1: identity + backend query only. Stage 3 reuses this in the factory.
    class RHIResource
    {
    public:
        virtual ~RHIResource() = default;

        RHIDevice& GetOwnerDevice() const;
        RHIBackend GetBackend() const;

    protected:
        explicit RHIResource(RHIDevice& ownerDevice);

        // Optional debug-time verification that backend matches expectation.
        // Compiled to no-op in release if XE_ASSERT is empty there.
        void XE_AssertBackendMatches(RHIBackend expected) const;

    private:
        RHIDevice* m_OwnerDevice = nullptr;
    };
}
```

### 5.2 New file: `RHIResource.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/RHIResource.cpp
#include "XEngine/RHI/RHIResource.h"

#include "XEngine/RHI/RHIDevice.h"

namespace XEngine
{
    RHIResource::RHIResource(RHIDevice& ownerDevice)
        : m_OwnerDevice(&ownerDevice)
    {
    }

    RHIDevice& RHIResource::GetOwnerDevice() const
    {
        XE_ASSERT(m_OwnerDevice != nullptr);
        return *m_OwnerDevice;
    }

    RHIBackend RHIResource::GetBackend() const
    {
        return m_OwnerDevice->GetBackend();
    }

    void RHIResource::XE_AssertBackendMatches(RHIBackend expected) const
    {
        XE_ASSERT(GetBackend() == expected);
    }
}
```

### 5.3 New file: `RHIUtils.h`

```cpp
// Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUtils.h
#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend);
}
```

### 5.4 New file: `RHIUtils.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/RHIUtils.cpp
#include "XEngine/RHI/RHIUtils.h"

namespace XEngine
{
    const char* RHIBackendToString(RHIBackend backend)
    {
        switch (backend)
        {
        case RHIBackend::None:   return "None";
        case RHIBackend::Vulkan: return "Vulkan";
        case RHIBackend::D3D12:  return "D3D12";
        case RHIBackend::Metal:  return "Metal";
        }
        return "Unknown";
    }
}
```

### 5.5 New file: `VulkanCheckedCast.h`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCheckedCast.h
#pragma once

#include "XEngine/Core/Assert.h"
#include "XEngine/RHI/RHIResource.h"
#include "XEngine/RHI/RHITypes.h"

namespace XEngine
{
    class VulkanDevice;

    // Replaces dynamic_cast<VulkanX*>(rhiX*) in backend code.
    // Debug builds verify owner device matches and backend is Vulkan.
    // Release builds use static_cast.
    template <typename VulkanType, typename RHIType>
    VulkanType* CheckedVulkanCast(RHIType* resource, const VulkanDevice& expectedDevice)
    {
        XE_ASSERT(resource != nullptr);
        XE_ASSERT(&resource->GetOwnerDevice() == &expectedDevice);
        XE_ASSERT(resource->GetBackend() == RHIBackend::Vulkan);
        return static_cast<VulkanType*>(resource);
    }

    template <typename VulkanType, typename RHIType>
    const VulkanType* CheckedVulkanCast(const RHIType* resource, const VulkanDevice& expectedDevice)
    {
        XE_ASSERT(resource != nullptr);
        XE_ASSERT(&resource->GetOwnerDevice() == &expectedDevice);
        XE_ASSERT(resource->GetBackend() == RHIBackend::Vulkan);
        return static_cast<const VulkanType*>(resource);
    }
}
```

No `.cpp` needed — fully header-template.

### 5.6 Modify: `RHI.h` (the public header aggregate)

**Before** (lines 1–16 of `Public/XEngine/RHI/RHI.h`):

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIClipSpace.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <cstddef>
#include <functional>
#include <memory>
```

**After**:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>      // NEW
#include <XEngine/RHI/RHIUtils.h>          // NEW
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/RHIClipSpace.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <cstddef>
#include <functional>
#include <memory>
```

### 5.7 Modify: `Resources/RHIBuffer.h`

**Before** (full file is short):

```cpp
#pragma once

#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    // ... RHIBufferUsage enum, RHIBufferDesc struct ...

    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;

        virtual std::size_t GetSize() const = 0;
        virtual bool Update(const void* data, std::size_t size, std::size_t offset = 0) = 0;
    };
}
```

**After**:

```cpp
#pragma once

#include <XEngine/RHI/RHIResource.h>      // NEW
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    // ... RHIBufferUsage enum, RHIBufferDesc struct unchanged ...

    class RHIBuffer : public RHIResource   // CHANGED
    {
    public:
        ~RHIBuffer() override = default;   // CHANGED

        virtual std::size_t GetSize() const = 0;
        virtual bool Update(const void* data, std::size_t size, std::size_t offset = 0) = 0;

    protected:                              // NEW BLOCK
        explicit RHIBuffer(RHIDevice& ownerDevice);
    };
}
```

### 5.8 Modify: `Resources/RHITexture.h`

**Before**:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    struct RHITextureDesc
    {
        // ... fields unchanged ...
    };

    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;

        virtual const RHITextureDesc& GetDesc() const = 0;
        virtual void* GetNativeImageView(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }
    };
}
```

**After**:

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHIResource.h>     // NEW
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    struct RHITextureDesc
    {
        // ... fields unchanged ...
    };

    class RHITexture : public RHIResource  // CHANGED
    {
    public:
        ~RHITexture() override = default;  // CHANGED

        virtual const RHITextureDesc& GetDesc() const = 0;
        virtual void* GetNativeImageView(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }

    protected:                             // NEW BLOCK
        explicit RHITexture(RHIDevice& ownerDevice);
    };
}
```

### 5.9 Modify: `Resources/RHIShader.h`

**Before**:

```cpp
class RHIShader
{
public:
    virtual ~RHIShader() = default;

    virtual ShaderStage GetStage() const = 0;
    virtual ShaderTarget GetTarget() const = 0;
};
```

**After**:

```cpp
class RHIShader : public RHIResource     // CHANGED
{
public:
    ~RHIShader() override = default;      // CHANGED

    virtual ShaderStage GetStage() const = 0;
    virtual ShaderTarget GetTarget() const = 0;

protected:                                 // NEW BLOCK
    explicit RHIShader(RHIDevice& ownerDevice);
};
```

Add `#include <XEngine/RHI/RHIResource.h>` near the top.

### 5.10 Modify: `Resources/RHIPipeline.h`

**Before**:

```cpp
class RHIPipeline
{
public:
    virtual ~RHIPipeline() = default;
};
```

**After**:

```cpp
class RHIPipeline : public RHIResource   // CHANGED
{
public:
    ~RHIPipeline() override = default;    // CHANGED

protected:                                 // NEW BLOCK
    explicit RHIPipeline(RHIDevice& ownerDevice);
};
```

Add `#include <XEngine/RHI/RHIResource.h>` near the top.

### 5.11 Modify: `Resources/RHISampler.h`

**Before**:

```cpp
class RHISampler
{
public:
    virtual ~RHISampler() = default;

    virtual const RHISamplerDesc& GetDesc() const = 0;
    virtual void* GetNativeSampler(RHIBackend backend) const
    {
        (void)backend;
        return nullptr;
    }
};
```

**After**:

```cpp
class RHISampler : public RHIResource   // CHANGED
{
public:
    ~RHISampler() override = default;    // CHANGED

    virtual const RHISamplerDesc& GetDesc() const = 0;
    virtual void* GetNativeSampler(RHIBackend backend) const
    {
        (void)backend;
        return nullptr;
    }

protected:                                 // NEW BLOCK
    explicit RHISampler(RHIDevice& ownerDevice);
};
```

Add `#include <XEngine/RHI/RHIResource.h>` near the top.

### 5.12 Modify: `Resources/RHIBindGroup.h`

**Before**:

```cpp
class RHIBindGroupLayout
{
public:
    virtual ~RHIBindGroupLayout() = default;

    virtual const RHIBindGroupLayoutDesc& GetDesc() const = 0;
};

class RHIBindGroup
{
public:
    virtual ~RHIBindGroup() = default;

    virtual const RHIBindGroupDesc& GetDesc() const = 0;
};
```

**After**:

```cpp
class RHIBindGroupLayout : public RHIResource   // CHANGED
{
public:
    ~RHIBindGroupLayout() override = default;   // CHANGED

    virtual const RHIBindGroupLayoutDesc& GetDesc() const = 0;

protected:                                        // NEW BLOCK
    explicit RHIBindGroupLayout(RHIDevice& ownerDevice);
};

class RHIBindGroup : public RHIResource         // CHANGED
{
public:
    ~RHIBindGroup() override = default;          // CHANGED

    virtual const RHIBindGroupDesc& GetDesc() const = 0;

protected:                                        // NEW BLOCK
    explicit RHIBindGroup(RHIDevice& ownerDevice);
};
```

Add `#include <XEngine/RHI/RHIResource.h>` near the top.

### 5.13 Modify: `Vulkan/VulkanBuffer.h`

**Before** (constructor signature on line 14):

```cpp
class VulkanBuffer final : public RHIBuffer
{
public:
    VulkanBuffer() = default;
    VulkanBuffer(VmaAllocator allocator, const RHIBufferDesc& desc, const void* initialData, std::size_t initialDataSize);
    ~VulkanBuffer() override;

    // ...
};
```

**After**:

```cpp
class VulkanBuffer final : public RHIBuffer
{
public:
    VulkanBuffer() = default;
    // Pass the owning RHIDevice so RHIBuffer's base can store it.
    VulkanBuffer(class VulkanDevice& device, VmaAllocator allocator, const RHIBufferDesc& desc, const void* initialData, std::size_t initialDataSize);
    ~VulkanBuffer() override;

    // ...
};
```

Add at the top of the file a forward declaration (the existing
`<XEngine/RHI/RHIResource.h>` already pulls in `RHIDevice`, so this is
just for clarity):

```cpp
namespace XEngine { class VulkanDevice; }
```

### 5.14 Modify: `Vulkan/VulkanBuffer.cpp`

**Before** (lines 41–47 of `VulkanBuffer.cpp`):

```cpp
VulkanBuffer::VulkanBuffer(
    VmaAllocator allocator,
    const RHIBufferDesc& desc,
    const void* initialData,
    std::size_t initialDataSize)
    : m_Allocator(allocator)
    , m_Size(desc.Size)
{
```

**After**:

```cpp
VulkanBuffer::VulkanBuffer(
    VulkanDevice& device,
    VmaAllocator allocator,
    const RHIBufferDesc& desc,
    const void* initialData,
    std::size_t initialDataSize)
    : RHIBuffer(device)               // NEW: pass owner to base
    , m_Device(device.GetHandle())    // NEW: cache VkDevice from RHIDevice
    , m_Allocator(allocator)
    , m_Size(desc.Size)
{
```

Add the following to the private member section of `VulkanBuffer.h`
(currently has `VkDevice m_Device = VK_NULL_HANDLE;` already on line 26 —
verify it is `VK_NULL_HANDLE` initialised).

### 5.15 Modify: `Vulkan/VulkanTexture.h`

**Before** (constructor signature on line 14):

```cpp
VulkanTexture(VkDevice device, VmaAllocator allocator, const RHITextureDesc& desc);
```

**After**:

```cpp
VulkanTexture(class VulkanDevice& device, VmaAllocator allocator, const RHITextureDesc& desc);
```

### 5.16 Modify: `Vulkan/VulkanTexture.cpp`

**Before** (lines 31–34):

```cpp
VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, const RHITextureDesc& desc)
    : m_Device(device)
    , m_Allocator(allocator)
    , m_Desc(desc)
{
```

**After**:

```cpp
VulkanTexture::VulkanTexture(VulkanDevice& device, VmaAllocator allocator, const RHITextureDesc& desc)
    : RHITexture(device)               // NEW: pass owner to base
    , m_Device(device.GetHandle())     // NEW: cache VkDevice
    , m_Allocator(allocator)
    , m_Desc(desc)
{
```

### 5.17 Modify: `Vulkan/VulkanShader.h`

**Before**:

```cpp
VulkanShader(VkDevice device, const RHIShaderDesc& desc);
```

**After**:

```cpp
VulkanShader(class VulkanDevice& device, const RHIShaderDesc& desc);
```

### 5.18 Modify: `Vulkan/VulkanShader.cpp`

**Before** (lines 13–16 area):

```cpp
VulkanShader::VulkanShader(VkDevice device, const RHIShaderDesc& desc)
    : m_Device(device)
    , m_Stage(desc.Stage)
    // ...
{
```

**After**:

```cpp
VulkanShader::VulkanShader(VulkanDevice& device, const RHIShaderDesc& desc)
    : RHIShader(device)               // NEW
    , m_Device(device.GetHandle())     // NEW
    , m_Stage(desc.Stage)
    // ...
{
```

### 5.19 Modify: `Vulkan/VulkanPipeline.h`

**Before**:

```cpp
VulkanPipeline(VkDevice device, const RHIGraphicsPipelineDesc& desc);
```

**After**:

```cpp
VulkanPipeline(class VulkanDevice& device, const RHIGraphicsPipelineDesc& desc);
```

### 5.20 Modify: `Vulkan/VulkanPipeline.cpp`

**Before** (lines 15–18):

```cpp
VulkanPipeline::VulkanPipeline(VkDevice device, const RHIGraphicsPipelineDesc& desc)
    : m_Device(device)
    , m_PushConstantStages(ToVulkanShaderStageFlags(desc.PushConstantStages))
{
```

**After**:

```cpp
VulkanPipeline::VulkanPipeline(VulkanDevice& device, const RHIGraphicsPipelineDesc& desc)
    : RHIPipeline(device)               // NEW
    , m_Device(device.GetHandle())      // NEW
    , m_PushConstantStages(ToVulkanShaderStageFlags(desc.PushConstantStages))
{
```

### 5.21 Modify: `Vulkan/VulkanSampler.h`

**Before**:

```cpp
VulkanSampler(VkDevice device, const RHISamplerDesc& desc);
```

**After**:

```cpp
VulkanSampler(class VulkanDevice& device, const RHISamplerDesc& desc);
```

### 5.22 Modify: `Vulkan/VulkanSampler.cpp`

**Before** (lines 11–14):

```cpp
VulkanSampler::VulkanSampler(VkDevice device, const RHISamplerDesc& desc)
    : m_Device(device)
    , m_Desc(desc)
{
```

**After**:

```cpp
VulkanSampler::VulkanSampler(VulkanDevice& device, const RHISamplerDesc& desc)
    : RHISampler(device)              // NEW
    , m_Device(device.GetHandle())    // NEW
    , m_Desc(desc)
{
```

### 5.23 Modify: `Vulkan/VulkanDescriptor.h`

**Before**:

```cpp
bool Create(VkDevice device, const RHIBindGroupLayoutDesc& desc);

bool Create(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    const RHIBindGroupDesc& desc);
```

**After**:

```cpp
bool Create(class VulkanDevice& device, const RHIBindGroupLayoutDesc& desc);

bool Create(
    class VulkanDevice& device,
    VkDescriptorPool descriptorPool,
    const RHIBindGroupDesc& desc);
```

Add `VkDevice m_Device` private member if not already present; cache
`device.GetHandle()` in the constructor (see 5.24).

### 5.24 Modify: `Vulkan/VulkanDescriptor.cpp`

**Before** (lines 20–28):

```cpp
bool VulkanBindGroupLayout::Create(VkDevice device, const RHIBindGroupLayoutDesc& desc)
{
    if (device == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Cannot create Vulkan bind group layout without a valid device");
        return false;
    }

    m_Device = device;
```

**After**:

```cpp
bool VulkanBindGroupLayout::Create(VulkanDevice& device, const RHIBindGroupLayoutDesc& desc)
{
    m_Device = device.GetHandle();      // NEW: cache VkDevice from RHIDevice
    if (m_Device == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Cannot create Vulkan bind group layout without a valid device");
        return false;
    }
```

**Before** (lines 93–99 of `VulkanBindGroup::Create`):

```cpp
bool VulkanBindGroup::Create(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    const RHIBindGroupDesc& desc)
{
    if (device == VK_NULL_HANDLE || descriptorPool == VK_NULL_HANDLE || desc.Layout == nullptr)
    {
        // ...
    }

    m_Device = device;
    m_DescriptorPool = descriptorPool;
```

**After**:

```cpp
bool VulkanBindGroup::Create(
    VulkanDevice& device,
    VkDescriptorPool descriptorPool,
    const RHIBindGroupDesc& desc)
{
    m_Device = device.GetHandle();      // NEW: cache VkDevice from RHIDevice
    if (m_Device == VK_NULL_HANDLE || descriptorPool == VK_NULL_HANDLE || desc.Layout == nullptr)
    {
        // ...
    }

    m_DescriptorPool = descriptorPool;
```

### 5.25 Modify: `Vulkan/VulkanCommandList.h`

**Before**:

```cpp
class VulkanCommandList final : public RHICommandList
{
public:
    VulkanCommandList() = default;
    ~VulkanCommandList() override = default;
    // ...
};
```

**After** (add a private owner-device pointer populated by `BeginFrame`):

```cpp
class VulkanCommandList final : public RHICommandList
{
public:
    VulkanCommandList() = default;
    ~VulkanCommandList() override = default;
    // ...

private:
    // NEW: cached reference to the VulkanDevice that owns this command list.
    // Populated by BeginFrame, cleared by Reset.
    class VulkanDevice* m_Device = nullptr;

    // ... existing members unchanged ...
};
```

Add `class VulkanDevice;` forward declaration at the top of the header
(after `class VulkanBindGroup; class VulkanPipeline; class VulkanTexture;`).

### 5.26 Modify: `Vulkan/VulkanCommandList.cpp` — `BeginFrame`

**Before** (lines 43–63):

```cpp
void VulkanCommandList::BeginFrame(
    VkCommandBuffer commandBuffer,
    VkImage swapchainImage,
    VkImageView swapchainImageView,
    VkExtent2D swapchainExtent,
    VkImageLayout* swapchainImageLayout,
    VulkanTexture* depthTexture)
{
    m_CommandBuffer = commandBuffer;
    m_SwapchainImage = swapchainImage;
    m_SwapchainImageView = swapchainImageView;
    m_SwapchainExtent = swapchainExtent;
    m_RenderViewport = RHIRect2D { 0, 0, swapchainExtent.width, swapchainExtent.height };
    m_RenderOutput = RHIRenderOutputDesc {};
    m_RenderOutput.Viewport = m_RenderViewport;
    m_RenderOutput.RenderToSwapchain = true;
    m_SwapchainImageLayout = swapchainImageLayout;
    m_DepthTexture = depthTexture;
    m_BoundGraphicsPipeline = nullptr;
    m_RenderingActive = false;
}
```

**After**:

```cpp
void VulkanCommandList::BeginFrame(
    class VulkanDevice& device,
    VkCommandBuffer commandBuffer,
    VkImage swapchainImage,
    VkImageView swapchainImageView,
    VkExtent2D swapchainExtent,
    VkImageLayout* swapchainImageLayout,
    VulkanTexture* depthTexture)
{
    m_Device = &device;                              // NEW
    m_CommandBuffer = commandBuffer;
    m_SwapchainImage = swapchainImage;
    m_SwapchainImageView = swapchainImageView;
    m_SwapchainExtent = swapchainExtent;
    m_RenderViewport = RHIRect2D { 0, 0, swapchainExtent.width, swapchainExtent.height };
    m_RenderOutput = RHIRenderOutputDesc {};
    m_RenderOutput.Viewport = m_RenderViewport;
    m_RenderOutput.RenderToSwapchain = true;
    m_SwapchainImageLayout = swapchainImageLayout;
    m_DepthTexture = depthTexture;
    m_BoundGraphicsPipeline = nullptr;
    m_RenderingActive = false;
}
```

Update the matching declaration in `VulkanCommandList.h` to take the new
leading `VulkanDevice& device` parameter.

### 5.27 Modify: `Vulkan/VulkanCommandList.cpp` — `Reset`

**Before** (lines 65–77):

```cpp
void VulkanCommandList::Reset()
{
    m_CommandBuffer = VK_NULL_HANDLE;
    m_SwapchainImage = VK_NULL_HANDLE;
    m_SwapchainImageView = VK_NULL_HANDLE;
    m_SwapchainExtent = {};
    m_RenderViewport = {};
    m_RenderOutput = {};
    m_SwapchainImageLayout = nullptr;
    m_DepthTexture = nullptr;
    m_BoundGraphicsPipeline = nullptr;
    m_RenderingActive = false;
}
```

**After**:

```cpp
void VulkanCommandList::Reset()
{
    m_Device = nullptr;                              // NEW
    m_CommandBuffer = VK_NULL_HANDLE;
    m_SwapchainImage = VK_NULL_HANDLE;
    m_SwapchainImageView = VK_NULL_HANDLE;
    m_SwapchainExtent = {};
    m_RenderViewport = {};
    m_RenderOutput = {};
    m_SwapchainImageLayout = nullptr;
    m_DepthTexture = nullptr;
    m_BoundGraphicsPipeline = nullptr;
    m_RenderingActive = false;
}
```

### 5.28 Modify: `Vulkan/VulkanCommandList.cpp` — replace every `dynamic_cast`

The pattern is identical in each spot. Below are the six exact call sites
and their replacements.

**5.28.a `SetGraphicsPipeline`** (lines 102–119):

Before:
```cpp
auto* vulkanPipeline = dynamic_cast<VulkanPipeline*>(pipeline);
if (vulkanPipeline == nullptr || !vulkanPipeline->IsValid())
```

After:
```cpp
XE_ASSERT(m_Device != nullptr);
auto* vulkanPipeline = CheckedVulkanCast<VulkanPipeline>(pipeline, *m_Device);
if (!vulkanPipeline->IsValid())
```

Before (lines 273–283 inside `BeginRenderingIfNeeded`):
```cpp
colorTexture = dynamic_cast<VulkanTexture*>(m_RenderOutput.ColorTarget);
depthTexture = dynamic_cast<VulkanTexture*>(m_RenderOutput.DepthTarget);
if (colorTexture == nullptr || !colorTexture->IsValid())
```

After:
```cpp
XE_ASSERT(m_Device != nullptr);
colorTexture = CheckedVulkanCast<VulkanTexture>(m_RenderOutput.ColorTarget, *m_Device);
depthTexture = CheckedVulkanTexture>(m_RenderOutput.DepthTarget, *m_Device);
if (colorTexture == nullptr || !colorTexture->IsValid())
```

Add at the top of `VulkanCommandList.cpp`:

```cpp
#include "VulkanCheckedCast.h"
#include "VulkanDevice.h"
```

**5.28.b `TransitionTextureToShaderRead`** (lines 133–145):

Before:
```cpp
auto* vulkanTexture = dynamic_cast<VulkanTexture*>(texture);
if (vulkanTexture == nullptr)
{
    return;
}
```

After:
```cpp
XE_ASSERT(m_Device != nullptr);
auto* vulkanTexture = CheckedVulkanCast<VulkanTexture>(texture, *m_Device);
```

**5.28.c `SetBindGroup`** (lines 147–158):

Before:
```cpp
auto* vulkanBindGroup = dynamic_cast<VulkanBindGroup*>(bindGroup);
if (vulkanBindGroup == nullptr || vulkanBindGroup->GetHandle() == VK_NULL_HANDLE)
```

After:
```cpp
XE_ASSERT(m_Device != nullptr);
auto* vulkanBindGroup = CheckedVulkanCast<VulkanBindGroup>(bindGroup, *m_Device);
if (vulkanBindGroup->GetHandle() == VK_NULL_HANDLE)
```

**5.28.d `SetVertexBuffer`** (lines 173–185):

Before:
```cpp
auto* vulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer);
if (vulkanBuffer == nullptr || !vulkanBuffer->IsValid())
```

After:
```cpp
XE_ASSERT(m_Device != nullptr);
auto* vulkanBuffer = CheckedVulkanCast<VulkanBuffer>(buffer, *m_Device);
if (!vulkanBuffer->IsValid())
```

**5.28.e `SetIndexBuffer`** (lines 192–204):

Before:
```cpp
auto* vulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer);
if (vulkanBuffer == nullptr || !vulkanBuffer->IsValid())
```

After:
```cpp
XE_ASSERT(m_Device != nullptr);
auto* vulkanBuffer = CheckedVulkanCast<VulkanBuffer>(buffer, *m_Device);
if (!vulkanBuffer->IsValid())
```

### 5.29 Modify: `Vulkan/VulkanPipeline.cpp` — replace `dynamic_cast`

**Before** (lines 27–34):

```cpp
auto* vertexShader = dynamic_cast<VulkanShader*>(desc.VertexShader);
auto* fragmentShader = dynamic_cast<VulkanShader*>(desc.FragmentShader);
if (vertexShader == nullptr || fragmentShader == nullptr ||
    !vertexShader->IsValid() || !fragmentShader->IsValid())
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline requires valid Vulkan vertex and fragment shaders");
    return;
}
```

**After** — needs the device handle for the cast. Add a local reference
extracted from one of the shaders:

```cpp
XE_ASSERT(desc.VertexShader != nullptr && desc.FragmentShader != nullptr);
VulkanDevice& device = static_cast<VulkanDevice&>(desc.VertexShader->GetOwnerDevice());
auto* vertexShader = CheckedVulkanCast<VulkanShader>(desc.VertexShader, device);
auto* fragmentShader = CheckedVulkanCast<VulkanShader>(desc.FragmentShader, device);
if (!vertexShader->IsValid() || !fragmentShader->IsValid())
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline requires valid Vulkan vertex and fragment shaders");
    return;
}
```

**Before** (lines 41–49):

```cpp
for (RHIBindGroupLayout* layout : desc.BindGroupLayouts)
{
    auto* vulkanLayout = dynamic_cast<VulkanBindGroupLayout*>(layout);
    if (vulkanLayout == nullptr || vulkanLayout->GetHandle() == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Vulkan pipeline received an invalid bind group layout");
        return;
    }

    descriptorSetLayouts.push_back(vulkanLayout->GetHandle());
}
```

**After**:

```cpp
for (RHIBindGroupLayout* layout : desc.BindGroupLayouts)
{
    auto* vulkanLayout = CheckedVulkanCast<VulkanBindGroupLayout>(layout, device);
    if (vulkanLayout->GetHandle() == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Vulkan pipeline received an invalid bind group layout");
        return;
    }

    descriptorSetLayouts.push_back(vulkanLayout->GetHandle());
}
```

Add at the top of `VulkanPipeline.cpp`:

```cpp
#include "VulkanCheckedCast.h"
#include "VulkanDevice.h"
```

### 5.30 Modify: `Vulkan/VulkanDescriptor.cpp` — replace `dynamic_cast`

**Before** (lines 104–110):

```cpp
auto* layout = dynamic_cast<VulkanBindGroupLayout*>(desc.Layout);
if (layout == nullptr || layout->GetHandle() == VK_NULL_HANDLE)
{
    XENGINE_LOG_ERROR("Vulkan bind group requires a valid Vulkan bind group layout");
    return false;
}
```

**After**:

```cpp
VulkanDevice& device = static_cast<VulkanDevice&>(desc.Layout->GetOwnerDevice());
auto* layout = CheckedVulkanCast<VulkanBindGroupLayout>(desc.Layout, device);
if (layout->GetHandle() == VK_NULL_HANDLE)
{
    XENGINE_LOG_ERROR("Vulkan bind group requires a valid Vulkan bind group layout");
    return false;
}
```

**Before** (lines 141–148 inside `CombinedImageSampler` branch):

```cpp
auto* texture = dynamic_cast<VulkanTexture*>(resource.Texture);
auto* sampler = dynamic_cast<VulkanSampler*>(resource.Sampler);
if (texture == nullptr || sampler == nullptr ||
    texture->GetImageView() == VK_NULL_HANDLE || sampler->GetHandle() == VK_NULL_HANDLE)
```

**After**:

```cpp
XE_ASSERT(resource.Texture != nullptr && resource.Sampler != nullptr);
VulkanDevice& device = static_cast<VulkanDevice&>(resource.Texture->GetOwnerDevice());
auto* texture = CheckedVulkanCast<VulkanTexture>(resource.Texture, device);
auto* sampler = CheckedVulkanCast<VulkanSampler>(resource.Sampler, device);
if (texture->GetImageView() == VK_NULL_HANDLE || sampler->GetHandle() == VK_NULL_HANDLE)
```

**Before** (lines 169–173 inside `UniformBuffer / StorageBuffer` branch):

```cpp
auto* buffer = dynamic_cast<VulkanBuffer*>(resource.Buffer);
if (buffer == nullptr || buffer->GetHandle() == VK_NULL_HANDLE)
```

**After**:

```cpp
XE_ASSERT(resource.Buffer != nullptr);
VulkanDevice& device = static_cast<VulkanDevice&>(resource.Buffer->GetOwnerDevice());
auto* buffer = CheckedVulkanCast<VulkanBuffer>(resource.Buffer, device);
if (buffer->GetHandle() == VK_NULL_HANDLE)
```

Add at the top of `VulkanDescriptor.cpp`:

```cpp
#include "VulkanCheckedCast.h"
#include "VulkanDevice.h"
```

### 5.31 Modify: `Vulkan/VulkanDevice.h` — add `GetHandle()`

Stage 1 needs `VulkanDevice::GetHandle()` to return its `VkDevice`. This
unblocks the `m_Device(device.GetHandle())` patterns above.

**Before** (the public surface area, around lines 33–69):

```cpp
class VulkanDevice final : public RHIDevice
{
public:
    VulkanDevice();
    ~VulkanDevice() override;

    bool Initialize(const VulkanDeviceCreateInfo& createInfo);
    void Shutdown();
    // ...
};
```

**After** — add `GetHandle()` to the public surface:

```cpp
class VulkanDevice final : public RHIDevice
{
public:
    VulkanDevice();
    ~VulkanDevice() override;

    bool Initialize(const VulkanDeviceCreateInfo& createInfo);
    void Shutdown();

    VkDevice GetHandle() const { return m_Device; }    // NEW

    // ...
};
```

### 5.32 Modify: `Vulkan/VulkanDevice.cpp` — pass `*this` into each Create call

**Before** (lines 504–681, every `CreateX` method):

```cpp
auto vulkanShader = std::make_shared<VulkanShader>(m_Device, desc);

auto buffer = std::make_shared<VulkanBuffer>(m_Allocator.GetHandle(), desc, initialData, initialDataSize);

auto texture = std::make_shared<VulkanTexture>(m_Device, m_Allocator.GetHandle(), desc);

auto sampler = std::make_shared<VulkanSampler>(m_Device, desc);

auto layout = std::make_shared<VulkanBindGroupLayout>();
if (!layout->Create(m_Device, desc)) { ... }

auto bindGroup = std::make_shared<VulkanBindGroup>();
if (!bindGroup->Create(m_Device, m_DescriptorPool, desc)) { ... }

auto pipeline = std::make_shared<VulkanPipeline>(m_Device, desc);
```

**After**:

```cpp
auto vulkanShader = std::make_shared<VulkanShader>(*this, desc);

auto buffer = std::make_shared<VulkanBuffer>(*this, m_Allocator.GetHandle(), desc, initialData, initialDataSize);

auto texture = std::make_shared<VulkanTexture>(*this, m_Allocator.GetHandle(), desc);

auto sampler = std::make_shared<VulkanSampler>(*this, desc);

auto layout = std::make_shared<VulkanBindGroupLayout>();
if (!layout->Create(*this, desc)) { ... }

auto bindGroup = std::make_shared<VulkanBindGroup>();
if (!bindGroup->Create(*this, m_DescriptorPool, desc)) { ... }

auto pipeline = std::make_shared<VulkanPipeline>(*this, desc);
```

That is the only change needed inside `CreateX` methods — passing `*this`
where they used to pass `m_Device`.

### 5.33 Modify: `Vulkan/VulkanDevice.cpp` — `BeginFrame` passes `*this` to `m_CommandList.BeginFrame`

**Before** (lines 328–334):

```cpp
m_CommandList.BeginFrame(
    m_FrameResources.GetCommandBuffer(),
    m_Swapchain.GetImage(m_CurrentImageIndex),
    m_Swapchain.GetImageView(m_CurrentImageIndex),
    m_Swapchain.GetExtent(),
    &m_CurrentSwapchainImageLayout,
    m_DepthTexture.get());
```

**After**:

```cpp
m_CommandList.BeginFrame(
    *this,                                          // NEW
    m_FrameResources.GetCommandBuffer(),
    m_Swapchain.GetImage(m_CurrentImageIndex),
    m_Swapchain.GetImageView(m_CurrentImageIndex),
    m_Swapchain.GetExtent(),
    &m_CurrentSwapchainImageLayout,
    m_DepthTexture.get());
```

### 5.34 Modify: `Vulkan/VulkanDevice.cpp` — depth texture construction

**Before** (line 803 inside `CreateDepthTexture`):

```cpp
auto depthTexture = std::make_unique<VulkanTexture>(m_Device, m_Allocator.GetHandle(), desc);
```

**After**:

```cpp
auto depthTexture = std::make_unique<VulkanTexture>(*this, m_Allocator.GetHandle(), desc);
```

### 5.35 CMake

`Engine/Source/Runtime/RHI/CMakeLists.txt` uses `GLOB_RECURSE` on
`Public/*.h`, `Private/*.h`, `Private/*.cpp`. New files `RHIResource.h`,
`RHIResource.cpp`, `RHIUtils.h`, `RHIUtils.cpp`, `VulkanCheckedCast.h`
will be picked up automatically. No edits required.

If you ever disable `XENGINE_ENABLE_VULKAN`, `VulkanCheckedCast.h` must
still be excluded by the existing `list(FILTER ... EXCLUDE REGEX "/Vulkan/")`
filter — verify by inspecting the `if(NOT XENGINE_ENABLE_VULKAN)` block in
the CMakeLists.

## 6. Implementation Order

Small, sequential steps inside Stage 1:

1. Add `RHIResource.h` and `RHIUtils.h` to `Public/XEngine/RHI/`.
2. Add `RHIResource` base implementation to a new `Private/RHIResource.cpp`
   (just `GetOwnerDevice`, `GetBackend`, the ctor, `XE_AssertBackendMatches`).
3. Make every resource base class inherit from `RHIResource` and add the
   protected `RHIResource(RHIDevice&)` constructor.
4. Update every `VulkanX` header to take `VulkanDevice&` instead of `VkDevice`
   in its public constructor (kept internal: still constructs the same native
   handles).
5. Update every `VulkanX.cpp` to pass `*this` to the base ctor.
6. Add `VulkanCheckedCast.h` and replace one `dynamic_cast` per file as a
   smoke test (e.g. only `VulkanCommandList::SetVertexBuffer`).
7. Replace remaining `dynamic_cast` sites.
8. Compile and run Editor + Sandbox to confirm behaviour is identical.

## 7. Verification

- **Build:** `XEngineRHI` library compiles. No public ABI break.
- **Editor smoke test:** Editor viewport still renders the spinner / default
  cube. ImGui overlay still draws over the offscreen color target
  (`ImGuiVulkanBackend` still uses
  `RHITexture::GetNativeImageView`).
- **Sandbox smoke test:** Sandbox shows the same forward PBR scene at the
  same quality and frame rate.
- **Vulkan validation:** No new validation-layer warnings. The
  `XENGINE_VK_CHECK` macro path is unchanged.
- **Runtime check:** On debug builds, deliberately pass an `RHITexture`
  belonging to a different (hypothetical) second `RHIDevice` into
  `SetVertexBuffer`. Confirm `XE_ASSERT` fires.
- **RenderDoc / shader debugger:** Capture the same frame as before. The
  draw call list and descriptor set contents should be byte-identical.

## 8. Common Mistakes

- Forgetting to include `RHIResource.h` from one of the resource headers
  and getting a confusing `GetOwnerDevice is not a member` error in a
  `VulkanX` .cpp.
- Calling `GetOwnerDevice()` on a temporary in the helper:
  ```cpp
  CheckedVulkanCast<VulkanTexture>(tex, tex->GetOwnerDevice());  // OK
  CheckedVulkanCast<VulkanTexture>(tex, *dev);                    // OK
  CheckedVulkanCast<VulkanTexture>(tex, SomeGlobalVulkanDevice{}); // WRONG — assertion will fire
  ```
- Removing `VkDevice m_Device` from a Vulkan class because `RHIDevice` is
  stored in the base. Keep it — Vulkan destroys natives via the
  `VkDevice`, not via `RHIDevice`. The two are intentionally separate
  references.
- Adding `XE_ASSERT` that is **not** compiled out in release. Verify the
  existing `XE_ASSERT` definition behaviour before relying on it.
- Forgetting `friend class RHIDevice;` (or future `friend class
  RHIResourceFactory;`) and getting a "protected constructor not
  accessible" error.

## 9. What This Stage Intentionally Does Not Do

- No `RHITextureView`. Every existing texture still exposes one default
  view via the existing `GetNativeImageView` path.
- No resource factory. `RHIDevice::CreateX` still does the work; the
  factory comes in Stage 3.
- No upload manager. `VulkanDevice::CreateTexture` still does the
  immediate-submit upload.
- No view-based render pass. `RHIRenderOutputDesc` still uses
  `RHITexture*`.
- No depth-only pipeline. `RHIGraphicsPipelineDesc::ColorFormat` is still
  required.
- No migration of the editor's `GetNativeImageView` call. That is Stage 8.
- No changes to the public Vulkan handle structures
  (`VulkanNativeContext`). Stage 7 will add `RHICapabilities`.