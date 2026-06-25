# Stage 3 — RHIResourceFactory

## 1. Goal

Move every `RHIDevice::CreateXXX(...)` resource-creation call to a new
`RHIResourceFactory` abstraction. After Stage 3:

- `RHIDevice` becomes a backend root context: capabilities, queues, frame
  lifecycle, swapchain, **and factory + upload manager accessors**.
- `RHIResourceFactory` exposes:
  ```text
  CreateBuffer          (desc, optional initial data, optional initialDataSize)
  CreateTexture         (desc)
  CreateTextureView     (desc)
  CreateSampler         (desc)
  CreateShader          (desc)
  CreateBindGroupLayout (desc)
  CreateBindGroup       (desc)
  CreateGraphicsPipeline(desc)
  ```
- `RHIResourceFactory` public methods run common `validate + normalize +
  capability check` and then call a protected virtual `CreateXImpl` on the
  backend subclass.
- `VulkanResourceFactory` overrides only the `CreateXImpl` methods and does
  the native Vulkan / VMA work.
- `RHIDevice::CreateXXX` remain as **inline wrappers** that delegate to
  `GetResourceFactory().CreateXXX(...)`. They are removed in Stage 8 once
  all Renderer call sites have migrated.

This stage is the largest mechanical refactor but does not introduce any new
behavioural capability — that comes in Stage 4 (upload), Stage 5 (views),
and Stage 6 (depth-only).

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Private/RHIDevice.cpp        (likely thin forwarder to VulkanDevice)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanShader.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp
```

What already exists:

- `RHIDevice::CreateShader`, `CreateBuffer`, `CreateTexture`, `CreateSampler`,
  `CreateBindGroupLayout`, `CreateBindGroup`, `CreateGraphicsPipeline` virtual
  methods.
- `VulkanDevice` implements all of them by directly constructing a
  `std::shared_ptr<VulkanX>`.
- `CreateBuffer` accepts `(desc, initialData, initialDataSize)`.
- `CreateTexture` accepts `(desc, initialData, initialDataSize)`.
- `CreateSampler` / `CreateShader` / `CreateBindGroupLayout` /
  `CreateBindGroup` / `CreateGraphicsPipeline` accept just the descriptor.

Renderer callers (today all use `m_Device->CreateX(...)`):

```text
Engine/Source/Runtime/Renderer/Private/Resources/RenderTextureManager.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderMeshManager.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderLibrary.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderPipelineStateCache.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderMaterialSystem.cpp
Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp
Engine/Source/Runtime/Renderer/Private/Mesh/PrimitiveMeshes.cpp
```

These call sites **do not change** in Stage 3 — they continue to call
`RHIDevice::CreateX`, which forwards to the factory.

What is missing:

- No `RHIResourceFactory` class.
- No central validation / normalisation layer.
- No way to instantiate a `VulkanResourceFactory` separately from
  `VulkanDevice`.

What should **not** be changed yet:

- `RHIDevice::CreateTexture` still does the inline upload (Stage 4).
- `RHIRenderOutputDesc` / `RHIBindingResource` still use `RHITexture*`
  (Stage 5).
- `RHIGraphicsPipelineDesc::ColorFormat` is still required (Stage 6).
- `RHITexture::GetNativeImageView` is still transitional (Stage 8).
- The `Renderer/Private/*` callers do not need to be migrated in Stage 3 —
  `RHIDevice::CreateX` wrappers keep working.

## 3. Files to Add

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIResourceFactory.h
Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp        (non-virtual common logic)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanResourceFactory.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanResourceFactory.cpp
```

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHI.h

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanShader.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp

Engine/Source/Runtime/RHI/Private/RHISystem.cpp          (factory lifetime)

Engine/Source/Runtime/RHI/CMakeLists.txt                (if needed)
```

## 5. Detailed Code Plan

### 5.1 New file: `RHIResourceFactory.h`

```cpp
// Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIResourceFactory.h
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/Resources/RHIBindGroup.h>
#include <XEngine/RHI/Resources/RHIBuffer.h>
#include <XEngine/RHI/Resources/RHIPipeline.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHIShader.h>
#include <XEngine/RHI/Resources/RHITexture.h>
#include <XEngine/RHI/Resources/RHITextureView.h>

#include <cstddef>
#include <memory>

namespace XEngine
{
    // NVI-style base. Public methods do common validation / normalisation,
    // then call the protected virtual CreateXImpl that the backend implements.
    class RHIResourceFactory
    {
    public:
        virtual ~RHIResourceFactory() = default;

        std::shared_ptr<RHIBuffer> CreateBuffer(
            const RHIBufferDesc& desc,
            const void* initialData = nullptr,
            std::size_t initialDataSize = 0);

        std::shared_ptr<RHITexture> CreateTexture(
            const RHITextureDesc& desc,
            const void* initialData = nullptr,
            std::size_t initialDataSize = 0);

        std::shared_ptr<RHITextureView> CreateTextureView(
            const RHITextureViewDesc& desc);

        std::shared_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc);
        std::shared_ptr<RHIShader>  CreateShader(const RHIShaderDesc& desc);

        std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayout(
            const RHIBindGroupLayoutDesc& desc);

        std::shared_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc);

        std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(
            const RHIGraphicsPipelineDesc& desc);

        RHIDevice& GetDevice() const;

    protected:
        explicit RHIResourceFactory(RHIDevice& ownerDevice);

        virtual std::shared_ptr<RHIBuffer> CreateBufferImpl(
            const RHIBufferDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        virtual std::shared_ptr<RHITexture> CreateTextureImpl(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) = 0;

        virtual std::shared_ptr<RHITextureView> CreateTextureViewImpl(
            const RHITextureViewDesc& desc) = 0;

        virtual std::shared_ptr<RHISampler> CreateSamplerImpl(
            const RHISamplerDesc& desc) = 0;

        virtual std::shared_ptr<RHIShader> CreateShaderImpl(
            const RHIShaderDesc& desc) = 0;

        virtual std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayoutImpl(
            const RHIBindGroupLayoutDesc& desc) = 0;

        virtual std::shared_ptr<RHIBindGroup> CreateBindGroupImpl(
            const RHIBindGroupDesc& desc) = 0;

        virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipelineImpl(
            const RHIGraphicsPipelineDesc& desc) = 0;

    private:
        RHIDevice* m_Device = nullptr;
    };
}
```

### 5.2 New file: `RHIResourceFactory.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp
#include "XEngine/RHI/RHIResourceFactory.h"

#include "XEngine/RHI/RHIDevice.h"
#include "XEngine/Core/Assert.h"
#include "XEngine/Logging/Log.h"

namespace XEngine
{
    RHIResourceFactory::RHIResourceFactory(RHIDevice& ownerDevice)
        : m_Device(&ownerDevice)
    {
    }

    RHIDevice& RHIResourceFactory::GetDevice() const
    {
        XE_ASSERT(m_Device != nullptr);
        return *m_Device;
    }

    // ---- Buffer ----

    std::shared_ptr<RHIBuffer> RHIResourceFactory::CreateBuffer(
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        if (desc.Size == 0)
        {
            XENGINE_LOG_ERROR("Cannot create RHI buffer with zero size");
            return nullptr;
        }
        RHIBufferDesc normalised = desc;
        if (normalised.DebugName == nullptr)
        {
            normalised.DebugName = "RHIBuffer";
        }
        return CreateBufferImpl(normalised, initialData, initialDataSize);
    }

    // ---- Texture ----

    std::shared_ptr<RHITexture> RHIResourceFactory::CreateTexture(
        const RHITextureDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        if (desc.Width == 0 || desc.Height == 0)
        {
            XENGINE_LOG_ERROR("Cannot create RHI texture with zero extent");
            return nullptr;
        }
        if (desc.Dimension == RHITextureDimension::TextureCube && desc.ArrayLayers != 6)
        {
            XENGINE_LOG_ERROR("TextureCube requires exactly 6 array layers");
            return nullptr;
        }
        RHITextureDesc normalised = desc;
        if (normalised.GenerateMips)
        {
            XENGINE_LOG_WARN("Texture mip generation not implemented; forcing MipLevels = 1");
            normalised.MipLevels = 1;
            normalised.GenerateMips = false;
        }
        if (normalised.DebugName == nullptr)
        {
            normalised.DebugName = "RHITexture";
        }
        return CreateTextureImpl(normalised, initialData, initialDataSize);
    }

    // ---- TextureView ----

    std::shared_ptr<RHITextureView> RHIResourceFactory::CreateTextureView(
        const RHITextureViewDesc& desc)
    {
        if (desc.Texture == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot create RHI texture view with null texture");
            return nullptr;
        }
        const RHITextureDesc& texDesc = desc.Texture->GetDesc();

        RHITextureViewDesc normalised = desc;
        if (normalised.MipCount == 0)
        {
            normalised.MipCount = texDesc.MipLevels - normalised.BaseMipLevel;
        }
        if (normalised.ArrayLayerCount == 0)
        {
            normalised.ArrayLayerCount = texDesc.ArrayLayers - normalised.BaseArrayLayer;
        }
        if (normalised.BaseMipLevel + normalised.MipCount > texDesc.MipLevels)
        {
            XENGINE_LOG_ERROR("RHI texture view mip range exceeds source texture");
            return nullptr;
        }
        if (normalised.BaseArrayLayer + normalised.ArrayLayerCount > texDesc.ArrayLayers)
        {
            XENGINE_LOG_ERROR("RHI texture view array layer range exceeds source texture");
            return nullptr;
        }
        return CreateTextureViewImpl(normalised);
    }

    // ---- Sampler ----

    std::shared_ptr<RHISampler> RHIResourceFactory::CreateSampler(const RHISamplerDesc& desc)
    {
        return CreateSamplerImpl(desc);
    }

    // ---- Shader ----

    std::shared_ptr<RHIShader> RHIResourceFactory::CreateShader(const RHIShaderDesc& desc)
    {
        if (desc.Code == nullptr || desc.CodeSize == 0 || desc.EntryPoint.empty())
        {
            XENGINE_LOG_ERROR("RHI shader desc requires non-null code and non-empty entry point");
            return nullptr;
        }
        return CreateShaderImpl(desc);
    }

    // ---- BindGroupLayout ----

    std::shared_ptr<RHIBindGroupLayout> RHIResourceFactory::CreateBindGroupLayout(
        const RHIBindGroupLayoutDesc& desc)
    {
        if (desc.Entries.empty())
        {
            XENGINE_LOG_ERROR("RHI bind group layout desc has no entries");
            return nullptr;
        }
        return CreateBindGroupLayoutImpl(desc);
    }

    // ---- BindGroup ----

    std::shared_ptr<RHIBindGroup> RHIResourceFactory::CreateBindGroup(
        const RHIBindGroupDesc& desc)
    {
        if (desc.Layout == nullptr)
        {
            XENGINE_LOG_ERROR("RHI bind group desc requires a non-null layout");
            return nullptr;
        }
        return CreateBindGroupImpl(desc);
    }

    // ---- GraphicsPipeline ----

    std::shared_ptr<RHIPipeline> RHIResourceFactory::CreateGraphicsPipeline(
        const RHIGraphicsPipelineDesc& desc)
    {
        if (desc.VertexShader == nullptr || desc.FragmentShader == nullptr)
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline requires both vertex and fragment shaders");
            return nullptr;
        }
        if (desc.ColorFormat == RHIFormat::Undefined)
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline requires a color format (Stage 6 will lift this)");
            return nullptr;
        }
        return CreateGraphicsPipelineImpl(desc);
    }
}
```

### 5.3 New file: `Vulkan/VulkanResourceFactory.h`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanResourceFactory.h
#pragma once

#include <XEngine/RHI/RHIResourceFactory.h>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cstddef>
#include <memory>

namespace XEngine
{
    class VulkanDevice;

    class VulkanResourceFactory final : public RHIResourceFactory
    {
    public:
        explicit VulkanResourceFactory(VulkanDevice& ownerDevice);
        ~VulkanResourceFactory() override = default;

    protected:
        std::shared_ptr<RHIBuffer> CreateBufferImpl(
            const RHIBufferDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) override;

        std::shared_ptr<RHITexture> CreateTextureImpl(
            const RHITextureDesc& desc,
            const void* initialData,
            std::size_t initialDataSize) override;

        std::shared_ptr<RHITextureView> CreateTextureViewImpl(
            const RHITextureViewDesc& desc) override;

        std::shared_ptr<RHISampler> CreateSamplerImpl(
            const RHISamplerDesc& desc) override;

        std::shared_ptr<RHIShader> CreateShaderImpl(
            const RHIShaderDesc& desc) override;

        std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayoutImpl(
            const RHIBindGroupLayoutDesc& desc) override;

        std::shared_ptr<RHIBindGroup> CreateBindGroupImpl(
            const RHIBindGroupDesc& desc) override;

        std::shared_ptr<RHIPipeline> CreateGraphicsPipelineImpl(
            const RHIGraphicsPipelineDesc& desc) override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}
```

### 5.4 New file: `Vulkan/VulkanResourceFactory.cpp`

This file is **purely** the verbatim contents of the existing
`VulkanDevice::CreateX` methods moved here. Each implementation now calls
the Vulkan constructor that takes `VulkanDevice&` (already wired in
Stage 1). The only changes versus the current `VulkanDevice.cpp` body are:

- Replace `m_Device` references with the cached `m_Device` member of the
  factory.
- Replace `m_Allocator.GetHandle()` with the cached `m_Allocator` member.
- Replace `m_DescriptorPool` with the cached member.
- The `CreateTexture` inline upload path stays here for Stage 3; Stage 4
  extracts it.

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanResourceFactory.cpp
#include "VulkanResourceFactory.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptor.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanSampler.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureView.h"
#include "VulkanUtils.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

#include <cstring>
#include <functional>
#include <string>

namespace XEngine
{
    VulkanResourceFactory::VulkanResourceFactory(VulkanDevice& ownerDevice)
        : RHIResourceFactory(ownerDevice)
        , m_Device(ownerDevice.GetHandle())
        , m_Allocator(ownerDevice.GetVmaAllocator())        // see 5.5 for accessor
        , m_DescriptorPool(ownerDevice.GetDescriptorPool())  // see 5.5 for accessor
    {
    }

    // CreateBufferImpl — moved from VulkanDevice.cpp lines 516–528.
    std::shared_ptr<RHIBuffer> VulkanResourceFactory::CreateBufferImpl(
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto buffer = std::make_shared<VulkanBuffer>(dev, m_Allocator, desc, initialData, initialDataSize);
        if (!buffer->IsValid())
        {
            return nullptr;
        }
        return buffer;
    }

    // CreateTextureImpl — moved from VulkanDevice.cpp lines 530–637.
    // For Stage 3 the inline upload path stays inside this function.
    // Stage 4 extracts it into RHIUploadManager.
    std::shared_ptr<RHITexture> VulkanResourceFactory::CreateTextureImpl(
        const RHITextureDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto texture = std::make_shared<VulkanTexture>(dev, m_Allocator, desc);
        if (!texture->IsValid())
        {
            return nullptr;
        }

        if (initialData != nullptr && initialDataSize > 0)
        {
            // Verbatim copy of the inline upload code from
            // VulkanDevice::CreateTexture (lines 541–633).
            RHIBufferDesc stagingDesc;
            stagingDesc.Size = initialDataSize;
            stagingDesc.Usage = RHIBufferUsage::TransferSrc;
            stagingDesc.MemoryUsage = RHIMemoryUsage::CPUToGPU;
            stagingDesc.DebugName = "Texture upload staging buffer";

            VulkanBuffer stagingBuffer(dev, m_Allocator, stagingDesc, initialData, initialDataSize);
            if (!stagingBuffer.IsValid())
            {
                XENGINE_LOG_ERROR("Failed to create texture upload staging buffer");
                return nullptr;
            }

            dev.ImmediateSubmit([&](VkCommandBuffer commandBuffer)
            {
                // ... unchanged barriers + vkCmdCopyBufferToImage ...
            });

            *texture->GetLayoutPtr() = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        return texture;
    }

    // CreateTextureViewImpl — moved from VulkanDevice::CreateTextureView.
    std::shared_ptr<RHITextureView> VulkanResourceFactory::CreateTextureViewImpl(
        const RHITextureViewDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto view = std::make_shared<VulkanTextureView>(dev, desc);
        if (!view->IsValid())
        {
            return nullptr;
        }
        return view;
    }

    // CreateSamplerImpl — moved from VulkanDevice.cpp lines 639–648.
    std::shared_ptr<RHISampler> VulkanResourceFactory::CreateSamplerImpl(
        const RHISamplerDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto sampler = std::make_shared<VulkanSampler>(dev, desc);
        if (!sampler->IsValid())
        {
            return nullptr;
        }
        return sampler;
    }

    // CreateShaderImpl — moved from VulkanDevice.cpp lines 504–514.
    std::shared_ptr<RHIShader> VulkanResourceFactory::CreateShaderImpl(
        const RHIShaderDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto shader = std::make_shared<VulkanShader>(dev, desc);
        if (!shader->IsValid())
        {
            return nullptr;
        }
        return shader;
    }

    // CreateBindGroupLayoutImpl — moved from VulkanDevice.cpp lines 650–659.
    std::shared_ptr<RHIBindGroupLayout> VulkanResourceFactory::CreateBindGroupLayoutImpl(
        const RHIBindGroupLayoutDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto layout = std::make_shared<VulkanBindGroupLayout>();
        if (!layout->Create(dev, desc))
        {
            return nullptr;
        }
        return layout;
    }

    // CreateBindGroupImpl — moved from VulkanDevice.cpp lines 661–670.
    std::shared_ptr<RHIBindGroup> VulkanResourceFactory::CreateBindGroupImpl(
        const RHIBindGroupDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto bindGroup = std::make_shared<VulkanBindGroup>();
        if (!bindGroup->Create(dev, m_DescriptorPool, desc))
        {
            return nullptr;
        }
        return bindGroup;
    }

    // CreateGraphicsPipelineImpl — moved from VulkanDevice.cpp lines 672–681.
    std::shared_ptr<RHIPipeline> VulkanResourceFactory::CreateGraphicsPipelineImpl(
        const RHIGraphicsPipelineDesc& desc)
    {
        VulkanDevice& dev = static_cast<VulkanDevice&>(GetDevice());
        auto pipeline = std::make_shared<VulkanPipeline>(dev, desc);
        if (!pipeline->IsValid())
        {
            return nullptr;
        }
        return pipeline;
    }
}
```

### 5.5 Modify: `Vulkan/VulkanDevice.h` — add factory owner / accessors

**Before** (private member section of `VulkanDevice.h`, lines 80–103):

```cpp
private:
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateDescriptorPool();
    void DestroyDescriptorPool();
    bool CreateDepthTexture();
    void DestroyDepthTexture();
    void RecreateSwapchain(u32 width, u32 height);
    void ImmediateSubmit(const std::function<void(VkCommandBuffer)>& function);

    VulkanInstance m_Instance;
    VulkanSurface m_Surface;
    VulkanAllocator m_Allocator;
    VulkanSwapchain m_Swapchain;
    VulkanFrameResources m_FrameResources;
    VulkanCommandList m_CommandList;
    std::unique_ptr<VulkanTexture> m_DepthTexture;

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VulkanQueue m_GraphicsQueue;
    VulkanQueue m_PresentQueue;

    u32 m_GraphicsFamilyIndex = 0;
    u32 m_PresentFamilyIndex = 0;
    u32 m_CurrentImageIndex = 0;
    u32 m_PendingResizeWidth = 0;
    u32 m_PendingResizeHeight = 0;
    VkImageLayout m_CurrentSwapchainImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool m_EnableVSync = true;
    bool m_FrameActive = false;
    bool m_ResizeRequested = false;
    bool m_Initialized = false;
};
```

**After** — add a factory member, a forward decl, and three public
accessors used by `VulkanResourceFactory`:

```cpp
class RHIResourceFactory;            // NEW forward decl

class VulkanDevice final : public RHIDevice
{
public:
    // ... existing public API ...

    // NEW: accessors used by VulkanResourceFactory and VulkanUploadManager.
    VmaAllocator GetVmaAllocator() const { return m_Allocator.GetHandle(); }
    VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

    // Immediate submit is now used by both VulkanResourceFactory (Stage 3
    // texture upload) and VulkanUploadManager (Stage 4).
    void ImmediateSubmit(const std::function<void(VkCommandBuffer)>& function);

    RHIResourceFactory& GetResourceFactory() override;
    const RHIResourceFactory& GetResourceFactory() const override;

private:
    // ... existing helpers ...

    std::unique_ptr<RHIResourceFactory> m_ResourceFactory;     // NEW

    // ... existing members ...
};
```

Remove all `CreateX(...) override;` lines from the public surface — they
are no longer overridden here; `RHIDevice` provides inline forwarders.

### 5.6 Modify: `Vulkan/VulkanDevice.cpp` — remove CreateX bodies, add factory management

**Before** (the entire `CreateX` block, lines 504–681):

```cpp
std::shared_ptr<RHIShader> VulkanDevice::CreateShader(const RHIShaderDesc& desc)
{
    auto vulkanShader = std::make_shared<VulkanShader>(*this, desc);
    // ... body ...
}

std::shared_ptr<RHIBuffer> VulkanDevice::CreateBuffer(...)
{
    // ... body ...
}

std::shared_ptr<RHITexture> VulkanDevice::CreateTexture(...)
{
    // ... body including inline upload ...
}

// ... CreateSampler / CreateBindGroupLayout / CreateBindGroup / CreateGraphicsPipeline ...
```

**After** — delete every `CreateX` method body. Add the factory accessor
implementations:

```cpp
RHIResourceFactory& VulkanDevice::GetResourceFactory()
{
    XE_ASSERT(m_ResourceFactory != nullptr);
    return *m_ResourceFactory;
}

const RHIResourceFactory& VulkanDevice::GetResourceFactory() const
{
    XE_ASSERT(m_ResourceFactory != nullptr);
    return *m_ResourceFactory;
}
```

### 5.7 Modify: `Vulkan/VulkanDevice.cpp` — `Initialize` creates the factory

**Before** (lines 122–202):

```cpp
bool VulkanDevice::Initialize(const VulkanDeviceCreateInfo& createInfo)
{
    // ... volkInitialize, instance, surface, PickPhysicalDevice, ...
    if (!m_Allocator.Create(m_Instance.GetHandle(), m_PhysicalDevice, m_Device))
    {
        return false;
    }

    if (!CreateDescriptorPool())
    {
        return false;
    }
    // ... swapchain, depth, frame resources ...
}
```

**After** — insert factory creation **after** `CreateDescriptorPool`:

```cpp
bool VulkanDevice::Initialize(const VulkanDeviceCreateInfo& createInfo)
{
    // ... volkInitialize, instance, surface, PickPhysicalDevice, ...

    if (!m_Allocator.Create(m_Instance.GetHandle(), m_PhysicalDevice, m_Device))
    {
        return false;
    }

    if (!CreateDescriptorPool())
    {
        return false;
    }

    // NEW: factory depends on device + allocator + descriptor pool.
    m_ResourceFactory = std::make_unique<VulkanResourceFactory>(*this);

    // ... swapchain, depth, frame resources ...
}
```

### 5.8 Modify: `Vulkan/VulkanDevice.cpp` — `Shutdown` destroys the factory

**Before** (lines 204–239):

```cpp
void VulkanDevice::Shutdown()
{
    if (!m_Initialized && m_Device == VK_NULL_HANDLE && m_PhysicalDevice == VK_NULL_HANDLE)
    {
        return;
    }

    WaitIdle();

    DestroyDepthTexture();
    m_FrameResources.Destroy();
    m_Swapchain.Destroy();
    DestroyDescriptorPool();
    m_Allocator.Destroy();
    // ...
}
```

**After** — destroy the factory before the pool and allocator:

```cpp
void VulkanDevice::Shutdown()
{
    if (!m_Initialized && m_Device == VK_NULL_HANDLE && m_PhysicalDevice == VK_NULL_HANDLE)
    {
        return;
    }

    WaitIdle();

    m_ResourceFactory.reset();   // NEW: destroy before pool/allocator

    DestroyDepthTexture();
    m_FrameResources.Destroy();
    m_Swapchain.Destroy();
    DestroyDescriptorPool();
    m_Allocator.Destroy();
    // ...
}
```

### 5.9 Modify: `RHIDevice.h` — add `GetResourceFactory` virtual

**Before** (after the `GetVulkanNativeContext` block):

```cpp
    virtual void RenderVulkanOverlay(const std::function<void(RHINativeCommandBuffer)>& callback)
    {
        (void)callback;
    }

    virtual void WaitIdle() = 0;
};
```

**After**:

```cpp
    virtual void RenderVulkanOverlay(const std::function<void(RHINativeCommandBuffer)>& callback)
    {
        (void)callback;
    }

    virtual void WaitIdle() = 0;

    // Stage 3: factory accessor. Stage 8 callers go through this directly
    // instead of the transitional CreateX wrappers.
    virtual RHIResourceFactory& GetResourceFactory() = 0;
    virtual const RHIResourceFactory& GetResourceFactory() const = 0;
};
```

Add `#include <XEngine/RHI/RHIResourceFactory.h>` to `RHIDevice.h`.

### 5.10 Modify: `RHIDevice.h` — replace `CreateX` virtuals with inline forwarders

**Before** (lines 39–63):

```cpp
    virtual std::shared_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) = 0;
    virtual std::shared_ptr<RHIBuffer> CreateBuffer(
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize) = 0;

    // TODO Stage 8/10:
    // Split RHIDevice resource creation into RHIResourceFactory and texture uploads into RHIUploadManager.
    virtual std::shared_ptr<RHITexture> CreateTexture(
        const RHITextureDesc& desc,
        const void* initialData,
        std::size_t initialDataSize) = 0;

    virtual std::shared_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) = 0;
    virtual std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc) = 0;
    virtual std::shared_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc) = 0;
    virtual std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) = 0;
```

**After**:

```cpp
    // Transitional inline forwarders — Stage 8 removes these.
    // Renderer callers continue to work unchanged during Stages 3–7.
    std::shared_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc)
    {
        return GetResourceFactory().CreateShader(desc);
    }

    std::shared_ptr<RHIBuffer> CreateBuffer(
        const RHIBufferDesc& desc,
        const void* initialData = nullptr,
        std::size_t initialDataSize = 0)
    {
        return GetResourceFactory().CreateBuffer(desc, initialData, initialDataSize);
    }

    std::shared_ptr<RHITexture> CreateTexture(
        const RHITextureDesc& desc,
        const void* initialData = nullptr,
        std::size_t initialDataSize = 0)
    {
        return GetResourceFactory().CreateTexture(desc, initialData, initialDataSize);
    }

    std::shared_ptr<RHITextureView> CreateTextureView(const RHITextureViewDesc& desc)
    {
        return GetResourceFactory().CreateTextureView(desc);
    }

    std::shared_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc)
    {
        return GetResourceFactory().CreateSampler(desc);
    }

    std::shared_ptr<RHIBindGroupLayout> CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc)
    {
        return GetResourceFactory().CreateBindGroupLayout(desc);
    }

    std::shared_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc)
    {
        return GetResourceFactory().CreateBindGroup(desc);
    }

    std::shared_ptr<RHIPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
    {
        return GetResourceFactory().CreateGraphicsPipeline(desc);
    }
```

The `CreateTextureView` transitional wrapper is added here in Stage 3 to
keep the Stage 2 transitional method working until Stage 8.

### 5.11 Modify: `RHISystem.cpp` — drop device pointer into default textures

The default white / black / normal textures in `RHISystem` need a debug
name. The current call site uses `device->CreateTexture(desc, pixels, size)`
with `desc.DebugName = nullptr` for white/normal (they get hardcoded
default names inside `VulkanTexture::VulkanTexture` already? — verify).
Stage 3 changes are limited to giving the factory a name to use:

**Before** (the factory will assign `"RHITexture"` if `DebugName` is null,
matching current behaviour — **no change needed in `RHISystem.cpp`**).

### 5.12 CMake

No edits. New files `RHIResourceFactory.h/.cpp` and
`Vulkan/VulkanResourceFactory.h/.cpp` are picked up by `GLOB_RECURSE`.

## 6. Implementation Order

The order minimises compile breakage:

1. Add `RHIResourceFactory.h` with the abstract public interface and
   protected virtuals (no .cpp yet).
2. Add `RHIResourceFactory.cpp` with the public wrappers calling
   `CreateXImpl` (no validation yet beyond `XE_ASSERT` non-null inputs).
3. Add `VulkanResourceFactory.h/.cpp`. Each `CreateXImpl` body is a
   copy-paste of today's `VulkanDevice::CreateX` body.
4. Add `RHIDevice::GetResourceFactory()` as a pure virtual on the base.
   Implement it on `VulkanDevice` returning the new factory. Keep the
   old `CreateX` virtuals but redirect each body to a single line that
   calls `GetResourceFactory().CreateX(...)`.
5. Compile. All existing Renderer callers continue to work through the
   transitional wrappers.
6. Add validation to `RHIResourceFactory.cpp` (size zero, layer count,
   null layout, etc.) one descriptor at a time.
7. Add `XE_ASSERT` checks inside `VulkanX` constructors for owner-device
   match (Stage 1 prerequisite).
8. Run Editor + Sandbox to confirm identical rendering.

## 7. Verification

- **Build:** `XEngineRHI` and `XEngineRenderer` compile without changes to
  Renderer source.
- **Editor smoke test:** Identical viewport rendering.
- **Sandbox smoke test:** Identical forward PBR scene.
- **Validation spot-check:** Call `GetResourceFactory().CreateTexture(...)`
  with `Width = 0`. Confirm `XE_ASSERT` or `XENGINE_LOG_ERROR` fires.
- **Descriptor pool:** Confirm the pool is created in `VulkanDevice::CreateDescriptorPool`
  and destroyed in `VulkanDevice::DestroyDescriptorPool` exactly once each.
- **RenderDoc:** A frame should produce the same descriptor set contents
  as before. No duplicate VkImageView creation.
- **Counting:** Use a temporary `XE_LOG_INFO` in each `CreateXImpl` to
  print `m_ResourceFactory->CreateX(...)` calls. Verify the same call
  count as before the refactor.

## 8. Common Mistakes

- Making `RHIResourceFactory` own the `VkDescriptorPool` and creating it
  before `VulkanDevice` has a valid `VkDevice`. Keep pool ownership on
  `VulkanDevice`; factory stores a non-owning handle.
- Forgetting to default-construct `m_ResourceFactory` to `nullptr` and
  calling `GetResourceFactory()` before `Initialize()` — undefined
  behaviour. `RHIDevice::GetResourceFactory()` should `XE_ASSERT` that the
  factory is non-null.
- Calling `CreateTexture(desc, initialData, initialDataSize)` and forgetting
  that the factory normalises the descriptor in place. Either copy the
  descriptor at the public wrapper or document that the descriptor may be
  mutated. Recommend copy at wrapper (use `RHITextureDesc normalised = desc;`).
- Putting validation in the backend subclass (`VulkanResourceFactory`)
  instead of the public wrapper. The point of the factory is shared
  validation across backends.
- Calling `CreateTextureView` with a texture that does not belong to the
  factory's owner device. `XE_ASSERT(&texture->GetOwnerDevice() == &GetDevice())`
  in the wrapper.

## 9. What This Stage Intentionally Does Not Do

- Does **not** change the `RHIDevice::CreateTexture` upload path.
- Does **not** introduce `RHIUploadManager`. Stage 4.
- Does **not** move `RHIRenderOutputDesc` / `RHIBindingResource` to
  `RHITextureView*`. Stage 5.
- Does **not** introduce depth-only pipeline support. Stage 6.
- Does **not** migrate Renderer callers to the factory accessor. Stage 8.
- Does **not** remove `RHIDevice::CreateX` transitional wrappers. Stage 8.
- Does **not** add capabilities / format utilities. Stage 7.
- Does **not** add debug-name support beyond what's already in each
  descriptor. Stage 7 adds a uniform path.