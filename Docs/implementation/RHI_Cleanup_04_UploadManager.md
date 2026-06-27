# Stage 4 — RHIUploadManager

## 1. Goal

Centralise every CPU→GPU upload behind a single `RHIUploadManager`
abstraction. After Stage 4:

- `RHIUploadManager` exposes:
  ```text
  UploadBuffer(RHIBuffer& destination, const void* data, std::size_t size, std::size_t offset = 0)
  UploadTexture(RHITexture& destination, const void* data, std::size_t size,
                const RHITextureSubresourceRange& subresource = defaultAllSubresources)
  FlushUploads()      // explicit flush; V0 is a no-op since uploads are inline-blocking
  ```
- `RHIUploadManager` does **not** know about `TextureAsset`, `MeshAsset`,
  `MaterialAsset`, `stb_image`, glTF, or any Renderer concept.
- `VulkanUploadManager` is a V0 implementation that uses an internal
  one-shot command buffer + `vkQueueWaitIdle` (the existing
  `ImmediateSubmit` mechanism) and a small per-frame staging buffer pool
  to avoid recreating staging buffers on every call.
- `RHIDevice::GetUploadManager()` returns the manager.
- `VulkanResourceFactory::CreateBufferImpl` / `CreateTextureImpl` stop
  doing inline upload and instead call `m_UploadManager->UploadX(...)`.
- The factory still accepts the `initialData / initialDataSize` parameters
  for convenience — it converts them into a single `UploadBuffer /
  UploadTexture` call right after construction.

This stage does **not** implement async upload, a transfer queue, or a
staging ring buffer. Those belong to a future stage.

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp
  - VulkanDevice::CreateBuffer (map / memcpy / unmap for initial data)
  - VulkanDevice::CreateTexture (creates VulkanBuffer staging, ImmediateSubmit, barriers, copy)
  - VulkanDevice::ImmediateSubmit (private helper)

Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBuffer.h
  - RHIBuffer::Update(data, size, offset) — public map/memcpy for CPU-mapped buffers
```

What already exists:

- `RHIBuffer::Update(...)` already exists for in-place buffer updates; the
  CPU-mapped path is correctly used today (no GPU command buffer involved).
- `VulkanDevice::CreateBuffer` does `vmaMapMemory → memcpy → vmaUnmapMemory`
  for the initial buffer data (no staging buffer).
- `VulkanDevice::CreateTexture` does:
  1. Create a staging `VulkanBuffer`.
  2. Call `ImmediateSubmit` with a lambda that issues
     `vkCmdPipelineBarrier` → `vkCmdCopyBufferToImage` →
     `vkCmdPipelineBarrier`.
- `VulkanDevice::ImmediateSubmit` creates a transient command pool +
  buffer, runs the user's function, submits, waits, and frees everything.

What is missing:

- No `RHIUploadManager` class.
- No common upload API.
- The texture upload path is baked into `CreateTexture` and not reusable
  for "upload later" use cases.

What should **not** be changed yet:

- `RHIRenderOutputDesc` still uses `RHITexture*` (Stage 5).
- `RHIBindingResource` still uses `RHITexture*` (Stage 5).
- `RHIGraphicsPipelineDesc::ColorFormat` still required (Stage 6).
- `RHITexture::GetNativeImageView` still transitional (Stage 8).
- `RHIDevice::CreateX` wrappers still call factory (Stage 8 removal).
- Async upload, transfer queue, and staging ring buffer are **out of
  scope** for this stage.

## 3. Files to Add

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUploadManager.h
Engine/Source/Runtime/RHI/Private/RHIUploadManager.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUploadManager.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUploadManager.cpp
```

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHI.h
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h
                                            (add RHITextureSubresourceRange)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanResourceFactory.cpp
                                            (delegate initial data to UploadManager)
Engine/Source/Runtime/RHI/Private/Vulkan/VulkanBuffer.cpp
                                            (RHIBuffer::Update stays; CPU-mapped path)
Engine/Source/Runtime/RHI/CMakeLists.txt
```

## 5. Detailed Code Plan

### 5.1 Modify: `Resources/RHITexture.h` — add `RHITextureSubresourceRange`

**Before** (the bottom of `RHITextureDesc` block, around lines 8–22):

```cpp
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
```

**After** — insert `RHITextureSubresourceRange` immediately after
`RHITextureDesc`:

```cpp
struct RHITextureDesc
{
    // ... unchanged ...
};

// NEW.
struct RHITextureSubresourceRange
{
    u32 BaseMipLevel = 0;
    u32 MipCount = 0;            // 0 = all remaining
    u32 BaseArrayLayer = 0;
    u32 ArrayLayerCount = 0;     // 0 = all remaining
};

inline RHITextureSubresourceRange AllSubresources()
{
    return RHITextureSubresourceRange { 0, 0, 0, 0 };
}
```

### 5.2 New file: `RHIUploadManager.h`

```cpp
// Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUploadManager.h
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <cstddef>

namespace XEngine
{
    class RHIBuffer;

    // Stage 4: V0 is blocking, single-threaded. No async / transfer queue.
    // The manager must not know about TextureAsset / MeshAsset / stb / glTF.
    class RHIUploadManager
    {
    public:
        virtual ~RHIUploadManager() = default;

        virtual void UploadBuffer(
            RHIBuffer& destination,
            const void* data,
            std::size_t size,
            std::size_t offset = 0) = 0;

        virtual void UploadTexture(
            RHITexture& destination,
            const void* data,
            std::size_t size,
            const RHITextureSubresourceRange& subresource = AllSubresources()) = 0;

        virtual void FlushUploads() = 0;

    protected:
        RHIUploadManager() = default;
    };
}
```

### 5.3 Modify: `Vulkan/VulkanBuffer.cpp` — keep `RHIBuffer::Update` unchanged

The existing `VulkanBuffer::Update` already does
`vmaMapMemory → memcpy → vmaFlushAllocation → vmaUnmapMemory` and returns
`true` on success. **Stage 4 leaves this function alone.** It is what
`RHIUploadManager::UploadBuffer` forwards to for CPU-mappable buffers.

### 5.4 New file: `Vulkan/VulkanUploadManager.h`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUploadManager.h
#pragma once

#include <XEngine/RHI/RHIUploadManager.h>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cstddef>
#include <vector>

namespace XEngine
{
    class VulkanDevice;

    class VulkanUploadManager final : public RHIUploadManager
    {
    public:
        explicit VulkanUploadManager(VulkanDevice& ownerDevice);
        ~VulkanUploadManager() override;

        void UploadBuffer(
            RHIBuffer& destination,
            const void* data,
            std::size_t size,
            std::size_t offset = 0) override;

        void UploadTexture(
            RHITexture& destination,
            const void* data,
            std::size_t size,
            const RHITextureSubresourceRange& subresource = AllSubresources()) override;

        void FlushUploads() override;

    private:
        // Acquire a staging buffer of at least `minSize` bytes.
        VkBuffer AcquireStagingBuffer(VmaAllocation& outAllocation, std::size_t minSize);
        void ReleaseStagingBuffer(VkBuffer buffer, VmaAllocation allocation);

        VulkanDevice& m_Device;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        struct StagingEntry
        {
            VmaAllocation allocation = VK_NULL_HANDLE;
            VkBuffer buffer = VK_NULL_HANDLE;
            std::size_t size = 0;
        };
        std::vector<StagingEntry> m_StagingPool;
    };
}
```

### 5.5 New file: `Vulkan/VulkanUploadManager.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUploadManager.cpp
#include "VulkanUploadManager.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <XEngine/Core/Assert.h>
#include <XEngine/Logging/Log.h>

#include <cstring>
#include <string>

namespace XEngine
{
    namespace
    {
        VkBufferUsageFlags ToVulkanBufferUsage(RHIBufferUsage usage)
        {
            VkBufferUsageFlags flags = 0;
            if (HasFlag(usage, RHIBufferUsage::Vertex)) { flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::Index)) { flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::Uniform)) { flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::Storage)) { flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
            if (HasFlag(usage, RHIBufferUsage::TransferSrc)) { flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; }
            if (HasFlag(usage, RHIBufferUsage::TransferDst)) { flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; }
            return flags;
        }
    }

    VulkanUploadManager::VulkanUploadManager(VulkanDevice& ownerDevice)
        : m_Device(ownerDevice)
        , m_Allocator(ownerDevice.GetVmaAllocator())
    {
    }

    VulkanUploadManager::~VulkanUploadManager()
    {
        for (StagingEntry& entry : m_StagingPool)
        {
            if (entry.buffer != VK_NULL_HANDLE && entry.allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(m_Allocator, entry.buffer, entry.allocation);
            }
        }
        m_StagingPool.clear();
    }

    void VulkanUploadManager::UploadBuffer(
        RHIBuffer& destination,
        const void* data,
        std::size_t size,
        std::size_t offset)
    {
        // CPU-mapped buffer path — forward to RHIBuffer::Update.
        destination.Update(data, size, offset);
    }

    void VulkanUploadManager::UploadTexture(
        RHITexture& destination,
        const void* data,
        std::size_t size,
        const RHITextureSubresourceRange& subresource)
    {
        XE_ASSERT(data != nullptr && size > 0);

        auto* vkTexture = static_cast<VulkanTexture*>(&destination);
        const RHITextureDesc& desc = destination.GetDesc();

        // Stage 4 only handles the base-mip full-extent case.
        const u32 mipCount = (subresource.MipCount == 0)
            ? (desc.MipLevels - subresource.BaseMipLevel)
            : subresource.MipCount;
        const u32 layerCount = (subresource.ArrayLayerCount == 0)
            ? (desc.ArrayLayers - subresource.BaseArrayLayer)
            : subresource.ArrayLayerCount;
        XE_ASSERT(subresource.BaseMipLevel == 0 && mipCount == 1);
        XE_ASSERT(subresource.BaseArrayLayer == 0 && layerCount == desc.ArrayLayers);

        VmaAllocation stagingAlloc = VK_NULL_HANDLE;
        VkBuffer stagingBuffer = AcquireStagingBuffer(stagingAlloc, size);
        if (stagingBuffer == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("Failed to acquire upload staging buffer");
            return;
        }

        void* mapped = nullptr;
        const VkResult mapResult = vmaMapMemory(m_Allocator, stagingAlloc, &mapped);
        if (mapResult != VK_SUCCESS)
        {
            XENGINE_LOG_ERROR("Failed to map upload staging buffer");
            ReleaseStagingBuffer(stagingBuffer, stagingAlloc);
            return;
        }
        std::memcpy(mapped, data, size);
        vmaUnmapMemory(m_Allocator, stagingAlloc);

        m_Device.ImmediateSubmit([&](VkCommandBuffer commandBuffer)
        {
            const VkImageAspectFlags aspect =
                (desc.Format == RHIFormat::D32Float)
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;

            VkImageSubresourceRange range {};
            range.aspectMask = aspect;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = desc.ArrayLayers;

            // Transition UNDEFINED/SHADER_READ -> TRANSFER_DST.
            VkImageMemoryBarrier toTransfer {};
            toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toTransfer.srcAccessMask = 0;
            toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toTransfer.oldLayout = *vkTexture->GetLayoutPtr();
            toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = vkTexture->GetImage();
            toTransfer.subresourceRange = range;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &toTransfer);

            VkBufferImageCopy copyRegion {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = aspect;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = desc.ArrayLayers;
            copyRegion.imageOffset = { 0, 0, 0 };
            copyRegion.imageExtent = { desc.Width, desc.Height, 1 };

            vkCmdCopyBufferToImage(
                commandBuffer,
                stagingBuffer,
                vkTexture->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &copyRegion);

            // Transition TRANSFER_DST -> SHADER_READ_ONLY_OPTIMAL.
            VkImageMemoryBarrier toShaderRead {};
            toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShaderRead.image = vkTexture->GetImage();
            toShaderRead.subresourceRange = range;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &toShaderRead);
        });

        *vkTexture->GetLayoutPtr() = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // ImmediateSubmit waits idle — staging is safe to keep alive, but
        // for Stage 4 we release back to the pool for simplicity.
        ReleaseStagingBuffer(stagingBuffer, stagingAlloc);
    }

    void VulkanUploadManager::FlushUploads()
    {
        // Stage 4: uploads are inline-blocking. No-op.
    }

    VkBuffer VulkanUploadManager::AcquireStagingBuffer(
        VmaAllocation& outAllocation, std::size_t minSize)
    {
        for (StagingEntry& entry : m_StagingPool)
        {
            if (entry.size >= minSize)
            {
                outAllocation = entry.allocation;
                return entry.buffer;
            }
        }

        VkBufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = minSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        const VkResult result = vmaCreateBuffer(
            m_Allocator, &bufferCreateInfo, &allocationCreateInfo,
            &buffer, &allocation, nullptr);
        if (result != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }

        StagingEntry entry;
        entry.buffer = buffer;
        entry.allocation = allocation;
        entry.size = minSize;
        m_StagingPool.push_back(entry);

        outAllocation = allocation;
        return buffer;
    }

    void VulkanUploadManager::ReleaseStagingBuffer(VkBuffer, VmaAllocation)
    {
        // Stage 4 keeps entries in the pool. Real release happens in
        // destructor. Replace with ring buffer / fence tracking in future
        // stages.
    }
}
```

### 5.6 Modify: `Vulkan/VulkanDevice.h` — add upload manager member + accessor

**Before** (private member section after `m_ResourceFactory`):

```cpp
private:
    // ...
    std::unique_ptr<RHIResourceFactory> m_ResourceFactory;
    // ...
```

**After**:

```cpp
class RHIUploadManager;     // NEW forward decl

class VulkanDevice final : public RHIDevice
{
public:
    // ... existing accessors ...

    // NEW
    RHIUploadManager& GetUploadManager();
    const RHIUploadManager& GetUploadManager() const;

private:
    // ...
    std::unique_ptr<RHIResourceFactory> m_ResourceFactory;
    std::unique_ptr<RHIUploadManager>   m_UploadManager;     // NEW
    // ...
};
```

### 5.7 Modify: `Vulkan/VulkanDevice.cpp` — manage upload manager lifetime

**Before** (the factory creation block in `Initialize`, after `CreateDescriptorPool`):

```cpp
    if (!CreateDescriptorPool())
    {
        return false;
    }

    // NEW: factory depends on device + allocator + descriptor pool.
    m_ResourceFactory = std::make_unique<VulkanResourceFactory>(*this);
```

**After**:

```cpp
    if (!CreateDescriptorPool())
    {
        return false;
    }

    m_ResourceFactory = std::make_unique<VulkanResourceFactory>(*this);
    m_UploadManager   = std::make_unique<VulkanUploadManager>(*this);     // NEW
```

**Before** (`Shutdown`):

```cpp
    WaitIdle();

    m_ResourceFactory.reset();   // NEW: destroy before pool/allocator
```

**After**:

```cpp
    WaitIdle();

    m_UploadManager.reset();     // NEW: before factory, allocator, pool
    m_ResourceFactory.reset();
```

### 5.8 Modify: `Vulkan/VulkanDevice.cpp` — add `GetUploadManager` impl

**After** the `GetResourceFactory` implementation block:

```cpp
RHIUploadManager& VulkanDevice::GetUploadManager()
{
    XE_ASSERT(m_UploadManager != nullptr);
    return *m_UploadManager;
}

const RHIUploadManager& VulkanDevice::GetUploadManager() const
{
    XE_ASSERT(m_UploadManager != nullptr);
    return *m_UploadManager;
}
```

### 5.9 Modify: `RHIDevice.h` — add `GetUploadManager` accessor

**Before** (after `GetResourceFactory` virtual):

```cpp
    virtual RHIResourceFactory& GetResourceFactory() = 0;
    virtual const RHIResourceFactory& GetResourceFactory() const = 0;
```

**After**:

```cpp
    virtual RHIResourceFactory& GetResourceFactory() = 0;
    virtual const RHIResourceFactory& GetResourceFactory() const = 0;

    virtual RHIUploadManager& GetUploadManager() = 0;             // NEW
    virtual const RHIUploadManager& GetUploadManager() const = 0; // NEW
```

Add `#include <XEngine/RHI/RHIUploadManager.h>` to `RHIDevice.h`.

### 5.10 Modify: `Vulkan/VulkanResourceFactory.cpp` — delegate initial data to upload manager

**Before** (the `CreateTextureImpl` body — lines after the texture is
created):

```cpp
    if (initialData != nullptr && initialDataSize > 0)
    {
        // ... 90 lines of inline upload code copied verbatim ...
    }

    return texture;
```

**After** — replace the entire upload block with a single call:

```cpp
    if (initialData != nullptr && initialDataSize > 0)
    {
        dev.GetUploadManager().UploadTexture(*texture, initialData, initialDataSize);
    }

    return texture;
```

**Before** (`CreateBufferImpl` body):

```cpp
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
```

The buffer's initial data upload happens inside the `VulkanBuffer`
constructor itself (`vmaMapMemory / memcpy / vmaUnmapMemory` — already
present in Stage 1). `RHIUploadManager::UploadBuffer` is the explicit
"upload later" path used after Stage 4 if a caller wants to defer upload.
**No change to `CreateBufferImpl` body** beyond what Stage 1 already
established — the existing constructor takes `initialData / initialDataSize`
directly.

### 5.11 CMake

No edits. New files `RHIUploadManager.h`,
`Vulkan/VulkanUploadManager.h/.cpp` are picked up by `GLOB_RECURSE`.

## 6. Implementation Order

1. Add `RHITextureSubresourceRange` to `RHITexture.h`.
2. Add `RHIUploadManager.h/.cpp` skeleton with `AllSubresources()` helper.
3. Add `VulkanUploadManager.h/.cpp`. Initial `UploadBuffer` simply calls
   `RHIBuffer::Update`; initial `UploadTexture` does the inline barrier +
   copy path.
4. Wire `m_UploadManager` into `VulkanDevice` (allocate / destroy /
   accessor).
5. Move the inline upload path out of `VulkanDevice::CreateBuffer` and
   `VulkanDevice::CreateTexture` into a single `UploadBuffer` / `UploadTexture`
   call.
6. Move the inline upload path out of `VulkanResourceFactory::CreateXImpl`
   if it is still duplicated there (it should not be after Step 5).
7. Compile. Renderer callers continue to pass `initialData` to
   `RHIDevice::CreateX` wrappers; nothing changes.
8. Run Editor + Sandbox to confirm texture loading and buffer upload paths
   still work.

## 7. Verification

- **Build:** Compiles without changes to Renderer source.
- **Editor smoke test:** Texture asset loading still populates the same
  pixel data; default white / black / normal textures still appear correct.
- **Sandbox smoke test:** Identical forward PBR rendering. Mesh vertex /
  index buffers still upload correctly.
- **Vulkan validation:** No new validation-layer warnings. Staging buffer
  pool should not leak — check `RenderDoc` for `VkBuffer` count parity.
- **Stage 9 readiness:** Stage 9 can now call
  `device.GetUploadManager().UploadTexture(...)` to upload cascade CPU data
  without needing a private helper on `VulkanDevice`.
- **Counting:** Temporary `XE_LOG_INFO` in `UploadBuffer / UploadTexture`
  should match the same call count as the previous inline path.

## 8. Common Mistakes

- Pulling `TextureAsset` into `RHIUploadManager` to "save a parameter".
  The whole point of the manager is to know nothing about asset types.
- Forgetting to wait for `vkQueueWaitIdle` (in `ImmediateSubmit`) before
  returning the staging buffer to the pool, causing the next upload to
  corrupt data.
- Calling `UploadBuffer` on a `GPUOnly` memory buffer. Stage 4 silently
  logs an error and returns. Future stages must add the GPU-only branch.
- Calling `UploadTexture` with a subresource range that does not match the
  data size. Stage 4 only supports "all subresources at base mip";
  validation of partial-subresource uploads is deferred.
- Re-using a staging buffer across two `UploadTexture` calls without
  ensuring the previous `vkQueueWaitIdle` has completed.

## 9. What This Stage Intentionally Does Not Do

- Does **not** move uploads to a dedicated transfer queue.
- Does **not** introduce a staging ring buffer or async fence tracking.
- Does **not** implement GPU-only memory upload paths.
- Does **not** compress uploads across frames.
- Does **not** move `RHIRenderOutputDesc::ColorTarget / DepthTarget` to
  views. Stage 5.
- Does **not** introduce depth-only pipeline. Stage 6.
- Does **not** migrate Renderer callers to `GetUploadManager()` directly.
  Stage 8.
- Does **not** remove the `initialData` parameters from
  `RHIDevice::CreateX`. Stage 8 may remove them once Renderer callers no
  longer need them.