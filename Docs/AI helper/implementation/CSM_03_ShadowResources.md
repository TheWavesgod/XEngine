# CSM_03 — Shadow Resources (ShadowResourceCache)

## Goal

`ShadowResourceCache` owns the GPU resources for shadow rendering. It does not know about Scene, does not compute matrices, does not draw, and does not know Vulkan. It only creates, holds, and recreates RHI resources based on a `DirectionalShadowResourceDesc`.

This is the only class in Stage 9 that is allowed to call `RHIResourceFactory::CreateTexture` / `CreateTextureView` / `CreateSampler` for shadow-related work.

## File to Modify

- `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h` (header already correct; only `.cpp` needs filling in)
- `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp` (currently empty)

## Files to NOT Modify

- `Engine/Source/Runtime/RHI/...` — RHI is finalized.
- `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h` — already has the right `DirectionalShadowResources` shape.
- `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.*` — owns math only.

## Current Header Shape (do not change)

```cpp
struct DirectionalShadowResourceDesc
{
    u32 Resolution   = 2048;
    u32 CascadeCount = 4;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    ShadowMapStorageMode StorageMode = ShadowMapStorageMode::Texture2DArray;
};

struct DirectionalShadowResources
{
    std::shared_ptr<RHITexture> Texture;
    std::shared_ptr<RHITextureView> SampledView;
    std::array<std::shared_ptr<RHITextureView>, MaxShadowCascades> LayerDepthViews {};
    std::shared_ptr<RHISampler> Sampler;

    u32 Resolution   = 0;
    u32 CascadeCount = 0;
    RHIFormat Format = RHIFormat::Undefined;
};

class ShadowResourceCache
{
public:
    void Initialize(RHIDevice& device);
    void Shutdown(RHIDevice& device);

    DirectionalShadowResources& GetOrCreateDirectionalShadowResources(
        RHIDevice& device,
        const DirectionalShadowResourceDesc& desc);

private:
    DirectionalShadowResources m_Directional;
};
```

The cache is **single-slot** in V0. If the requested `desc` matches the current slot, it returns the existing resources. If anything differs, it destroys the existing resources and rebuilds. The header's `GetOrCreate*` name is intentional and matches the cleanup spec's allowed pattern (the cache is the resource owner, not `RHITexture`).

## Detailed Implementation Plan

### `ShadowResourceCache::Initialize(RHIDevice& device)`

```cpp
void ShadowResourceCache::Initialize(RHIDevice&)
{
    // Nothing to do up front. Resources are created lazily on first
    // GetOrCreateDirectionalShadowResources call.
}
```

### `ShadowResourceCache::Shutdown(RHIDevice& device)`

```cpp
void ShadowResourceCache::Shutdown(RHIDevice&)
{
    m_Directional = {};
}
```

The `shared_ptr` reset releases the texture, views, and sampler. The RHI handles GPU-side destruction in its own destructor (see `VulkanTexture::~VulkanTexture` and `VulkanTextureView::~VulkanTextureView`).

### `ShadowResourceCache::GetOrCreateDirectionalShadowResources(...)`

Pseudocode:

```cpp
DirectionalShadowResources& ShadowResourceCache::GetOrCreateDirectionalShadowResources(
    RHIDevice& device,
    const DirectionalShadowResourceDesc& desc)
{
    XENGINE_ASSERT(device.IsValid(), "ShadowResourceCache requires a valid RHIDevice");

    // Reject anything other than Texture2DArray in V0.
    if (desc.StorageMode != ShadowMapStorageMode::Texture2DArray)
    {
        XENGINE_LOG_ERROR("Stage 9 V0 only supports ShadowMapStorageMode::Texture2DArray");
        return m_Directional; // empty; caller should treat as failure
    }

    // Validate count.
    if (desc.CascadeCount == 0 || desc.CascadeCount > MaxShadowCascades)
    {
        XENGINE_LOG_ERROR("Cascade count out of range");
        return m_Directional;
    }
    if (desc.Resolution == 0)
    {
        XENGINE_LOG_ERROR("Shadow resolution must be > 0");
        return m_Directional;
    }
    if (desc.DepthFormat != RHIFormat::D32Float)
    {
        XENGINE_LOG_ERROR("Stage 9 V0 only supports D32Float shadow formats");
        return m_Directional;
    }

    // Match against current slot. Same shape? return existing.
    if (m_Directional.Texture != nullptr
        && m_Directional.CascadeCount == desc.CascadeCount
        && m_Directional.Resolution   == desc.Resolution
        && m_Directional.Format       == desc.DepthFormat)
    {
        return m_Directional;
    }

    // Recreate. Release existing first.
    m_Directional = {};
    m_Directional.CascadeCount = desc.CascadeCount;
    m_Directional.Resolution   = desc.Resolution;
    m_Directional.Format       = desc.DepthFormat;

    RHIResourceFactory& factory = device.GetResourceFactory();

    // 1. Whole shadow texture (2D array depth).
    RHITextureDesc texDesc {};
    texDesc.Width        = desc.Resolution;
    texDesc.Height       = desc.Resolution;
    texDesc.MipLevels    = 1;
    texDesc.ArrayLayers  = desc.CascadeCount;
    texDesc.Format       = desc.DepthFormat;
    texDesc.Dimension    = RHITextureDimension::Texture2DArray;
    texDesc.Usage        = RHITextureUsageFlags::DepthStencilAttachment
                         | RHITextureUsageFlags::Sampled;
    texDesc.DebugName    = "DirectionalShadowArray";
    m_Directional.Texture = factory.CreateTexture(texDesc);
    if (!m_Directional.Texture)
    {
        XENGINE_LOG_ERROR("Failed to create directional shadow texture array");
        m_Directional = {};
        return m_Directional;
    }

    // 2. Whole-array sampled view (used by ForwardPBR.slang).
    RHITextureViewDesc sampledDesc {};
    sampledDesc.Texture        = m_Directional.Texture.get();
    sampledDesc.Usage          = RHITextureViewUsageFlags::Sampled;
    sampledDesc.ViewDimension  = RHITextureViewDimension::Texture2DArray;
    sampledDesc.Aspect         = RHITextureAspectFlags::Depth;
    sampledDesc.Format         = desc.DepthFormat;
    sampledDesc.BaseMipLevel   = 0;
    sampledDesc.MipCount       = 1;
    sampledDesc.BaseArrayLayer = 0;
    sampledDesc.ArrayLayerCount = 0; // all layers
    sampledDesc.DebugName      = "DirectionalShadowArraySampled";
    m_Directional.SampledView  = factory.CreateTextureView(sampledDesc);
    if (!m_Directional.SampledView)
    {
        XENGINE_LOG_ERROR("Failed to create directional shadow sampled view");
        m_Directional = {};
        return m_Directional;
    }

    // 3. Per-layer depth attachment views.
    for (u32 layer = 0; layer < desc.CascadeCount; ++layer)
    {
        RHITextureViewDesc layerDesc {};
        layerDesc.Texture         = m_Directional.Texture.get();
        layerDesc.Usage           = RHITextureViewUsageFlags::DepthAttachment;
        layerDesc.ViewDimension   = RHITextureViewDimension::Texture2DArray;
        layerDesc.Aspect          = RHITextureAspectFlags::Depth;
        layerDesc.Format          = desc.DepthFormat;
        layerDesc.BaseMipLevel    = 0;
        layerDesc.MipCount        = 1;
        layerDesc.BaseArrayLayer  = layer;
        layerDesc.ArrayLayerCount = 1; // single layer
        layerDesc.DebugName       = "DirectionalShadowLayerView";

        m_Directional.LayerDepthViews[layer] = factory.CreateTextureView(layerDesc);
        if (!m_Directional.LayerDepthViews[layer])
        {
            XENGINE_LOG_ERROR("Failed to create directional shadow per-layer depth view");
            m_Directional = {};
            return m_Directional;
        }
    }

    // 4. Comparison sampler.
    RHISamplerDesc samplerDesc {};
    samplerDesc.MinFilter    = RHIFilter::Linear;
    samplerDesc.MagFilter    = RHIFilter::Linear;
    samplerDesc.AddressU     = RHIAddressMode::ClampToBorder;
    samplerDesc.AddressV     = RHIAddressMode::ClampToBorder;
    samplerDesc.AddressW     = RHIAddressMode::ClampToBorder;
    samplerDesc.MaxAnisotropy = 1.0f;
    samplerDesc.DebugName    = "DirectionalShadowSampler";
    m_Directional.Sampler    = factory.CreateSampler(samplerDesc);
    if (!m_Directional.Sampler)
    {
        XENGINE_LOG_ERROR("Failed to create directional shadow comparison sampler");
        m_Directional = {};
        return m_Directional;
    }

    return m_Directional;
}
```

### Notes on RHI validation

- The base `RHIResourceFactory::CreateTextureView` will already reject mismatches. In particular:
  - `desc.Texture` must be non-null.
  - `desc.Texture->GetOwnerDevice() == factory.GetDevice()`.
  - `NormalizeTextureViewDesc` will clamp `MipCount` to texture's mip count and `ArrayLayerCount = 0` to the texture's array layer count.
  - `ValidateTextureViewDesc` will reject the per-layer view if `ArrayLayerCount = 1` while the texture is `Texture2D` (it isn't here — we use `Texture2DArray`).
- `RHIUtils::IsDepthFormat(D32Float)` returns `true`, so the `RHITextureAspectFlags::Depth` is valid.
- `RHITextureUsageFlags::DepthStencilAttachment` is required because we want the texture usable as a depth attachment, and `RHITextureUsageFlags::Sampled` is required for the shader sample.

### Notes on layout transitions

`ShadowResourceCache` does **not** insert any Vulkan barriers. Layout transitions are performed by `ShadowDepthPass` (writing) and by the implicit `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` set in `VulkanBindGroup` (sampling). If the engine grows a real RenderGraph resource tracker, this is the place to attach the resource to a graph handle.

### Notes on the sampler

The current `RHISamplerDesc` does not have a `CompareOp` or `CompareEnable` field. The current Vulkan backend's `VulkanSampler` hardcodes `compareEnable = VK_FALSE` and `compareOp = VK_COMPARE_OP_ALWAYS`. For V0 this means we cannot use `SampleCmp` with hardware PCF.

**Two options for V0:**

1. **Recommended — keep current sampler, do PCF in the shader.** The `ForwardPBR.slang` shader reads the shadow map with a `Sample` (no comparison) using `Linear` filter, then performs a 3x3 PCF kernel in the shader: for each of 9 offsets, read the depth and compare manually. This works with the existing `RHISamplerDesc` and avoids touching the RHI.
2. **Future — add `CompareOp` to `RHISamplerDesc`.** This requires RHI changes and a new RHI cleanup pass. Defer to Stage 10+.

This document specifies **option 1** for V0. The shader will use `Sample` and do the comparison in HLSL/Slang. The "comparison sampler" name in the `DirectionalShadowResources` is still accurate in intent — it is the appropriate sampler for shadow sampling — but the implementation in V0 is a regular bilinear sampler with `ClampToBorder` and border color opaque white.

### `ClampToBorder` with a known border color

The current `VulkanSampler` hardcodes `borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK`. For shadow sampling with PCF in the shader, you want the border to read as "lit" (1.0 in the comparison), which means the border depth should be 1.0. With `INT_OPAQUE_BLACK` and `Linear` filter, the border value is `0.0`, which is interpreted by the shader as "depth = 0" → comparison fails → "in shadow". This is wrong for the border case.

**Workaround in the shader:** clamp UV to `[0, 1]` before sampling, then the border is never reached. Add the following in the shadow sampling helper:

```hlsl
float2 ClampUV(float2 uv)
{
    return saturate(uv);
}
```

Apply before every `g_ShadowMap.Sample(...)` call. See [CSM_08](CSM_08_Shaders.md) for the integration.

## Initialization Order

`RenderShadowManager::Initialize` calls `m_ResourceCache.Initialize(device)`. No resources are created at that point.

On the first frame where shadow rendering is needed, `RenderShadowManager::PrepareDirectionalShadow` calls `m_ResourceCache.GetOrCreateDirectionalShadowResources(device, desc)`. The first call creates the resources; subsequent calls return them as long as the desc does not change.

## What This Document Does Not Do

- It does not describe how matrices are filled — see [CSM_04](CSM_04_DirectionalShadowPlanner_Coordinates.md).
- It does not describe how the resources are bound in a pass — see [CSM_06](CSM_06_ShadowDepthPass_And_Pipeline.md).
- It does not describe how the resources are wired into `RenderShadowFrameData` — see [CSM_05](CSM_05_RenderShadowManager.md).

## Common Mistakes

1. **Forgetting to set `ArrayLayerCount = 0` for the whole-array sampled view.** This makes the sampled view only cover layer 0, and `ForwardPBR.slang` will read garbage. The `RHIResourceFactory::CreateTextureView` validator catches this only if you pass a number that exceeds the texture's layer count.

2. **Setting `ArrayLayerCount = desc.CascadeCount` for the sampled view.** This works, but it ties the view to a specific cascade count, requiring recreation if the count changes. Using `0` (meaning "all layers") is robust.

3. **Setting `RHITextureUsageFlags::ColorAttachment` on the shadow texture.** Shadow textures are depth-only; adding color is wasteful and the validator will reject the per-layer view.

4. **Reusing the same `RHITextureView` for both sampling and depth attachment.** They are different view types (`Sampled` vs `DepthAttachment`) and need separate `RHITextureView` objects. The cache creates both kinds.

5. **Forgetting to reset the slot when creation partially fails.** The pseudocode above resets `m_Directional = {}` on any failure. Without this, the next call would return a half-initialized cache.
