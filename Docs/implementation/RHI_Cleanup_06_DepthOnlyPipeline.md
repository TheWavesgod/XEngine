# Stage 6 — Depth-Only Pipeline

## 1. Goal

Allow `RHIGraphicsPipelineDesc` to describe a depth-only pipeline:

- `ColorAttachmentCount = 0` means "no color attachments" (depth-only /
  shadow depth pass).
- `ColorFormat` becomes optional.
- `DepthFormat` is still required.
- Pipeline key `RenderPassKind::ShadowDepth` becomes constructible.

This is required for Stage 9 ShadowDepthPass but it is intentionally a
self-contained RHI change so Stage 9 is purely a Renderer-side concern.

> Current-source correction (2026-06-25): the checked-in
> `RHIGraphicsPipelineDesc` already has `bool HasColorAttachment`. Prefer
> extending that path for Stage 6:
>
> - `HasColorAttachment == false` maps to Vulkan dynamic rendering
>   `colorAttachmentCount = 0`.
> - `HasColorAttachment == true` maps to the existing single-colour path.
> - Add depth-bias fields if needed by shadow pipelines.
>
> If `ColorAttachmentCount` is still desired for future MRT, make Stage 6 an
> explicit rename from `HasColorAttachment` to `ColorAttachmentCount`; do not
> leave both fields in the descriptor.

## 2. Current Code Audit

Relevant existing files:

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h
  - RHIGraphicsPipelineDesc
      RHIShader* VertexShader
      RHIShader* FragmentShader
      RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;     // required
      RHIFormat DepthFormat = RHIFormat::D32Float;
      bool EnableDepthTest, EnableDepthWrite
      RHIVertexBufferLayoutDesc VertexLayout
      std::vector<RHIBindGroupLayout*> BindGroupLayouts
      u32 PushConstantSize
      RHIShaderStageFlags PushConstantStages
      const char* DebugName

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp
  - hardcoded: renderingCreateInfo.colorAttachmentCount = 1
  - hardcoded: pColorAttachmentFormats points to a single format
  - hardcoded: colorBlend.attachmentCount = 1
  - colorBlendAttachment colorWriteMask unconditionally all bits

Engine/Source/Runtime/Renderer/Private/Resources/GraphicsPipelineStateKey.h
  - RenderPassKind::ShadowDepth already exists
  - key has ColorFormat and DepthFormat; pipeline cache does not yet
    construct ShadowDepth pipelines.
```

What already exists:

- `RenderPassKind::ShadowDepth` is referenced in the pipeline key but no
  pipeline is constructed for it yet.
- `VulkanPipeline` supports `VK_FORMAT_UNDEFINED` for `DepthFormat` only —
  ColorFormat is currently required to be valid.

What is missing:

- `RHIGraphicsPipelineDesc::ColorAttachmentCount`.
- Backend support for `colorAttachmentCount = 0`.
- Backend support for `pColorAttachmentFormats = nullptr`.

What should **not** be changed yet:

- `RHITexture::GetNativeImageView` (transitional, Stage 8).
- `RHIDevice::CreateX` wrappers (transitional, Stage 8).
- View-based render pass / bind group are already done (Stage 5).
- Renderer ShadowDepthPass implementation (Stage 9).

## 3. Files to Add

None. This is a header change plus backend wiring.

## 4. Files to Modify

```text
Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h

Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp

Engine/Source/Runtime/Renderer/Private/Resources/GraphicsPipelineStateKey.h
  (no change to key; RenderPassKind::ShadowDepth already exists)

Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp  (Stage 3 file;
  validation: depth-only pipeline requires non-null fragment shader)
```

## 5. Detailed Code Plan

### 5.1 Modify: `Resources/RHIPipeline.h` — add `ColorAttachmentCount` and helper

For the current source baseline, read this section as one of two valid
alternatives:

1. Minimal Stage 6: keep `HasColorAttachment` and add only the depth-bias
   fields plus Vulkan handling for `HasColorAttachment == false`.
2. MRT-ready Stage 6: replace `HasColorAttachment` with
   `ColorAttachmentCount`. Update all existing call sites in the same commit.

**Before** (lines 56–75 of `RHIPipeline.h`, the body of `RHIGraphicsPipelineDesc`):

```cpp
struct RHIGraphicsPipelineDesc
{
    RHIShader* VertexShader = nullptr;
    RHIShader* FragmentShader = nullptr;

    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;

    bool EnableDepthTest = true;
    bool EnableDepthWrite = true;

    RHIVertexBufferLayoutDesc VertexLayout;
    std::vector<RHIBindGroupLayout*> BindGroupLayouts;

    u32 PushConstantSize = 0;
    RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;

    const char* DebugName = nullptr;
};
```

**After**:

```cpp
struct RHIGraphicsPipelineDesc
{
    RHIShader* VertexShader = nullptr;
    RHIShader* FragmentShader = nullptr;

    // 0 = depth-only pipeline (Stage 6 / Stage 9 ShadowDepth).
    // 1 = single color attachment (default; matches Stage 5 behaviour).
    // >1 = rejected in Stage 6 (multi-rendertarget comes later).
    u32 ColorAttachmentCount = 1;

    // Ignored when ColorAttachmentCount == 0.
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;

    RHIFormat DepthFormat = RHIFormat::D32Float;

    bool EnableDepthTest = true;
    bool EnableDepthWrite = true;

    // Shadow depth pipelines typically want fixed bias. Defaults still
    // safe; Renderer / Stage 9 sets them when the key says ShadowDepth.
    bool DepthBiasEnable = false;
    float DepthBiasConstantFactor = 0.0f;
    float DepthBiasSlopeFactor = 0.0f;

    RHIVertexBufferLayoutDesc VertexLayout;
    std::vector<RHIBindGroupLayout*> BindGroupLayouts;

    u32 PushConstantSize = 0;
    RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;

    const char* DebugName = nullptr;

    // Convenience for Stage 9 ShadowDepth construction.
    static RHIGraphicsPipelineDesc MakeDepthOnly(
        RHIShader* vertexShader,
        RHIShader* fragmentShader,
        RHIFormat depthFormat,
        const RHIVertexBufferLayoutDesc& vertexLayout,
        std::vector<RHIBindGroupLayout*> bindGroupLayouts,
        u32 pushConstantSize = 0,
        RHIShaderStageFlags pushConstantStages = RHIShaderStageFlags::Vertex,
        bool depthBiasEnable = true,
        float depthBiasConstantFactor = 1.25f,
        float depthBiasSlopeFactor = 1.75f,
        const char* debugName = nullptr);
};
```

### 5.2 Modify: `Resources/RHIPipeline.cpp` — implement `MakeDepthOnly`

`RHIPipeline.cpp` may be a header-only translation unit today. If it is
not, add a new `.cpp`:

**New file**: `Private/Resources/RHIPipeline.cpp`

```cpp
// Engine/Source/Runtime/RHI/Private/Resources/RHIPipeline.cpp
#include "XEngine/RHI/Resources/RHIPipeline.h"

namespace XEngine
{
    RHIGraphicsPipelineDesc RHIGraphicsPipelineDesc::MakeDepthOnly(
        RHIShader* vertexShader,
        RHIShader* fragmentShader,
        RHIFormat depthFormat,
        const RHIVertexBufferLayoutDesc& vertexLayout,
        std::vector<RHIBindGroupLayout*> bindGroupLayouts,
        u32 pushConstantSize,
        RHIShaderStageFlags pushConstantStages,
        bool depthBiasEnable,
        float depthBiasConstantFactor,
        float depthBiasSlopeFactor,
        const char* debugName)
    {
        RHIGraphicsPipelineDesc desc;
        desc.VertexShader = vertexShader;
        desc.FragmentShader = fragmentShader;
        desc.ColorAttachmentCount = 0;
        desc.ColorFormat = RHIFormat::Undefined;
        desc.DepthFormat = depthFormat;
        desc.EnableDepthTest = true;
        desc.EnableDepthWrite = true;
        desc.DepthBiasEnable = depthBiasEnable;
        desc.DepthBiasConstantFactor = depthBiasConstantFactor;
        desc.DepthBiasSlopeFactor = depthBiasSlopeFactor;
        desc.VertexLayout = vertexLayout;
        desc.BindGroupLayouts = std::move(bindGroupLayouts);
        desc.PushConstantSize = pushConstantSize;
        desc.PushConstantStages = pushConstantStages;
        desc.DebugName = debugName;
        return desc;
    }
}
```

`CMake` picks it up via `GLOB_RECURSE`.

### 5.3 Modify: `Vulkan/VulkanPipeline.cpp` — honour `ColorAttachmentCount`

**Before** (the block that builds `VkPipelineRenderingCreateInfo` and the
`VkPipelineColorBlendStateCreateInfo` — approximately lines 274–295):

```cpp
const VkFormat colorFormat = RHIFormatToVulkanFormat(desc.ColorFormat);
const VkFormat depthFormat = RHIFormatToVulkanFormat(desc.DepthFormat);
if (colorFormat == VK_FORMAT_UNDEFINED)
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported color format");
    return;
}

VkPipelineRenderingCreateInfo renderingCreateInfo {};
renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingCreateInfo.colorAttachmentCount = 1;
renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
renderingCreateInfo.depthAttachmentFormat = depthFormat;

// ...

VkPipelineColorBlendAttachmentState colorBlendAttachment {};
colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

VkPipelineColorBlendStateCreateInfo colorBlend {};
colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlend.attachmentCount = 1;
colorBlend.pAttachments = &colorBlendAttachment;
```

**After**:

```cpp
const bool depthOnly = desc.ColorAttachmentCount == 0;
const VkFormat colorFormat = depthOnly
                                ? VK_FORMAT_UNDEFINED
                                : RHIFormatToVulkanFormat(desc.ColorFormat);
const VkFormat depthFormat = RHIFormatToVulkanFormat(desc.DepthFormat);
if (!depthOnly && colorFormat == VK_FORMAT_UNDEFINED)
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported color format");
    return;
}
if (depthFormat == VK_FORMAT_UNDEFINED)
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported depth format");
    return;
}

VkPipelineRenderingCreateInfo renderingCreateInfo {};
renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingCreateInfo.colorAttachmentCount = depthOnly ? 0u : 1u;
renderingCreateInfo.pColorAttachmentFormats = depthOnly ? nullptr : &colorFormat;
renderingCreateInfo.depthAttachmentFormat = depthFormat;
renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

// ...

VkPipelineColorBlendAttachmentState colorBlendAttachment {};
if (!depthOnly)
{
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

VkPipelineColorBlendStateCreateInfo colorBlend {};
colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlend.attachmentCount = depthOnly ? 0u : 1u;
colorBlend.pAttachments = depthOnly ? nullptr : &colorBlendAttachment;
```

### 5.4 Modify: `Vulkan/VulkanPipeline.cpp` — depth bias wiring

**Before** (the depth-stencil state block — approximately lines 256–268):

```cpp
VkPipelineDepthStencilStateCreateInfo depthStencil {};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = desc.EnableDepthTest;
depthStencil.depthWriteEnable = desc.EnableDepthWrite;
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.stencilTestEnable = VK_FALSE;
```

**After**:

```cpp
VkPipelineDepthStencilStateCreateInfo depthStencil {};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = desc.EnableDepthTest;
depthStencil.depthWriteEnable = desc.EnableDepthWrite;
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.stencilTestEnable = VK_FALSE;
depthStencil.depthBiasEnable = desc.DepthBiasEnable ? VK_TRUE : VK_FALSE;
depthStencil.depthBiasConstantFactor = desc.DepthBiasConstantFactor;
depthStencil.depthBiasSlopeFactor = desc.DepthBiasSlopeFactor;
depthStencil.depthBiasClamp = 0.0f;
```

(`LESS_OR_EQUAL` matches the existing SkyboxPass depth-test behaviour —
this is not a Stage 6 change, just kept explicit so the bias is
reviewable.)

### 5.5 Modify: `RHIResourceFactory.cpp` — validate depth-only input

**Before** (the `CreateGraphicsPipeline` wrapper in
`RHIResourceFactory.cpp`):

```cpp
std::unique_ptr<RHIGraphicsPipeline> RHIResourceFactory::CreateGraphicsPipeline(
    const RHIGraphicsPipelineDesc& desc)
{
    if (desc.VertexShader == nullptr || desc.FragmentShader == nullptr)
    {
        XENGINE_LOG_ERROR("CreateGraphicsPipeline requires both vertex and fragment shaders");
        return nullptr;
    }
    return CreateGraphicsPipelineImpl(desc);
}
```

**After**:

```cpp
std::unique_ptr<RHIGraphicsPipeline> RHIResourceFactory::CreateGraphicsPipeline(
    const RHIGraphicsPipelineDesc& desc)
{
    if (desc.VertexShader == nullptr)
    {
        XENGINE_LOG_ERROR("CreateGraphicsPipeline requires a vertex shader");
        return nullptr;
    }
    if (desc.FragmentShader == nullptr)
    {
        XENGINE_LOG_ERROR("CreateGraphicsPipeline requires a fragment shader (depth-only pipelines still need one)");
        return nullptr;
    }
    if (desc.ColorAttachmentCount == 0)
    {
        if (desc.DepthFormat == RHIFormat::Undefined)
        {
            XENGINE_LOG_ERROR("Depth-only graphics pipeline requires a depth format");
            return nullptr;
        }
    }
    else
    {
        if (desc.ColorAttachmentCount > 1)
        {
            XENGINE_LOG_ERROR("Stage 6 does not support more than one color attachment");
            return nullptr;
        }
        if (desc.ColorFormat == RHIFormat::Undefined)
        {
            XENGINE_LOG_ERROR("Color-attached graphics pipeline requires a color format");
            return nullptr;
        }
    }
    return CreateGraphicsPipelineImpl(desc);
}
```

The wrapper in `RHIResourceFactory.cpp` is the only validation site for
both backends; `VulkanResourceFactory::CreateGraphicsPipelineImpl`
inherits the same guarantees.

### 5.6 Modify: `Vulkan/VulkanResourceFactory.cpp` — assert stage contract

After the validation in `RHIResourceFactory`, the Vulkan implementation
can assume the desc is well-formed. Add an explicit assert at the top of
`VulkanResourceFactory::CreateGraphicsPipelineImpl` for safety:

**Before**:

```cpp
std::unique_ptr<RHIGraphicsPipeline> VulkanResourceFactory::CreateGraphicsPipelineImpl(
    const RHIGraphicsPipelineDesc& desc)
{
    if (desc.VertexShader == nullptr || desc.FragmentShader == nullptr)
    {
        XENGINE_LOG_ERROR("Vulkan graphics pipeline requires both vertex and fragment shaders");
        return nullptr;
    }
    // ...
}
```

**After**:

```cpp
std::unique_ptr<RHIGraphicsPipeline> VulkanResourceFactory::CreateGraphicsPipelineImpl(
    const RHIGraphicsPipelineDesc& desc)
{
    // Validation already performed in RHIResourceFactory::CreateGraphicsPipeline.
    XE_ASSERT(desc.VertexShader != nullptr);
    XE_ASSERT(desc.FragmentShader != nullptr);
    XE_ASSERT(desc.ColorAttachmentCount <= 1);
    // ...
}
```

### 5.7 Modify: `Renderer/Private/Resources/GraphicsPipelineStateKey.h`

**Before** (the body of `GraphicsPipelineStateKey`):

```cpp
struct GraphicsPipelineStateKey
{
    RenderPassKind PassKind = RenderPassKind::ForwardOpaque;
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    RHIVertexLayoutId VertexLayout = InvalidVertexLayoutId;
    std::vector<RHIBindGroupLayoutId> BindGroupLayouts;
    u32 PushConstantSize = 0;
    RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;
    // ...
};
```

**After** — add `DepthBiasEnable` so the cache key for shadow pipelines
is unique when toggling bias on/off:

```cpp
struct GraphicsPipelineStateKey
{
    RenderPassKind PassKind = RenderPassKind::ForwardOpaque;
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    RHIVertexLayoutId VertexLayout = InvalidVertexLayoutId;
    std::vector<RHIBindGroupLayoutId> BindGroupLayouts;
    u32 PushConstantSize = 0;
    RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;
    bool DepthBiasEnable = false;
    // ...
};
```

Add a free-function helper (or static method) so call sites stay short:

```cpp
inline GraphicsPipelineStateKey MakeShadowDepthKey(
    RHIFormat depthFormat,
    RHIVertexLayoutId vertexLayout,
    std::vector<RHIBindGroupLayoutId> bindGroupLayouts)
{
    GraphicsPipelineStateKey key;
    key.PassKind = RenderPassKind::ShadowDepth;
    key.ColorFormat = RHIFormat::Undefined;
    key.DepthFormat = depthFormat;
    key.VertexLayout = vertexLayout;
    key.BindGroupLayouts = std::move(bindGroupLayouts);
    key.PushConstantSize = 0;
    key.PushConstantStages = RHIShaderStageFlags::Vertex;
    key.DepthBiasEnable = true;
    return key;
}
```

### 5.8 Modify: `Renderer/Private/Resources/RenderPipelineStateCache.cpp`

**Before** (the body of `CreateGraphicsPipeline`):

```cpp
RHIGraphicsPipeline* RenderPipelineStateCache::CreateGraphicsPipeline(
    const GraphicsPipelineStateKey& key,
    const RenderPipelineShaderSet& shaders,
    const VertexLayoutMap& layouts)
{
    RHIGraphicsPipelineDesc desc;
    desc.VertexShader = shaders.Vertex;
    desc.FragmentShader = shaders.Fragment;
    desc.ColorFormat = key.ColorFormat;
    desc.DepthFormat = key.DepthFormat;
    desc.VertexLayout = ResolveLayout(key.VertexLayout, layouts);
    for (auto id : key.BindGroupLayouts) desc.BindGroupLayouts.push_back(Lookup(id));
    desc.PushConstantSize = key.PushConstantSize;
    desc.PushConstantStages = key.PushConstantStages;
    desc.DebugName = "RenderPipeline";
    return m_Device->CreateGraphicsPipeline(desc).release();
}
```

**After**:

```cpp
RHIGraphicsPipeline* RenderPipelineStateCache::CreateGraphicsPipeline(
    const GraphicsPipelineStateKey& key,
    const RenderPipelineShaderSet& shaders,
    const VertexLayoutMap& layouts)
{
    if (key.PassKind == RenderPassKind::ShadowDepth)
    {
        auto desc = RHIGraphicsPipelineDesc::MakeDepthOnly(
            shaders.Vertex,
            shaders.Fragment,
            key.DepthFormat,
            ResolveLayout(key.VertexLayout, layouts),
            ResolveBindGroupLayouts(key.BindGroupLayouts),
            key.PushConstantSize,
            key.PushConstantStages,
            key.DepthBiasEnable,        // bool overload
            key.DepthBiasEnable ? 1.25f : 0.0f,
            key.DepthBiasEnable ? 1.75f : 0.0f,
            "ShadowDepth");
        return m_Device->GetResourceFactory().CreateGraphicsPipeline(desc).release();
    }

    RHIGraphicsPipelineDesc desc;
    desc.VertexShader = shaders.Vertex;
    desc.FragmentShader = shaders.Fragment;
    desc.ColorAttachmentCount = 1;
    desc.ColorFormat = key.ColorFormat;
    desc.DepthFormat = key.DepthFormat;
    desc.VertexLayout = ResolveLayout(key.VertexLayout, layouts);
    for (auto id : key.BindGroupLayouts) desc.BindGroupLayouts.push_back(Lookup(id));
    desc.PushConstantSize = key.PushConstantSize;
    desc.PushConstantStages = key.PushConstantStages;
    desc.DebugName = "RenderPipeline";
    return m_Device->GetResourceFactory().CreateGraphicsPipeline(desc).release();
}
```

`ResolveBindGroupLayouts` is a small helper to keep the call site
readable:

```cpp
std::vector<RHIBindGroupLayout*> RenderPipelineStateCache::ResolveBindGroupLayouts(
    const std::vector<RHIBindGroupLayoutId>& ids) const
{
    std::vector<RHIBindGroupLayout*> out;
    out.reserve(ids.size());
    for (auto id : ids) out.push_back(Lookup(id));
    return out;
}
```

If `CreateGraphicsPipeline` already returns `RHIGraphicsPipeline*` (raw
pointer owned by the cache), `.release()` is not needed. Adjust per
existing convention.

### 5.9 CMake

No edits. New file `Private/Resources/RHIPipeline.cpp` is picked up by
`GLOB_RECURSE`.

## 6. Implementation Order

1. Add `ColorAttachmentCount` and `MakeDepthOnly` to `RHIGraphicsPipelineDesc`.
2. Update `VulkanPipeline.cpp` to honour `ColorAttachmentCount = 0`.
3. Update `RHIResourceFactory::CreateGraphicsPipelineImpl` wrapper
   validation.
4. Update `RenderPipelineStateCache::CreateGraphicsPipeline` to construct
   a depth-only pipeline when `key.PassKind == RenderPassKind::ShadowDepth`.
5. Compile and run Editor + Sandbox to confirm existing pipelines still
   work.

## 7. Verification

- **Build:** Compiles.
- **Sandbox smoke test:** Forward PBR scene unchanged.
- **Editor smoke test:** Editor viewport unchanged.
- **Depth-only pipeline creation:** Add a temporary test in
  `RenderPipelineStateCache::CreateGraphicsPipeline` that requests a
  `RenderPassKind::ShadowDepth` pipeline and inspects the resulting
  `VkPipeline` via `RenderDoc`. Confirm `colorAttachmentCount == 0` and
  `depthAttachmentFormat == VK_FORMAT_D32_SFLOAT`.
- **RenderDoc:** A depth-only pipeline should not produce any color
  attachment writes — verify with a manual draw that calls
  `vkCmdBindPipeline` and `vkCmdDraw` and observing no colour writes.

## 8. Common Mistakes

- Setting `colorAttachmentCount = 0` but leaving
  `pColorAttachmentFormats = &someFormat` — Vulkan spec says when
  `colorAttachmentCount == 0`, `pColorAttachmentFormats` must be `nullptr`.
- Forgetting to set `colorBlend.attachmentCount = 0` for depth-only
  pipelines. Vulkan validation will catch it.
- Forgetting to set `colorBlendAttachment.colorWriteMask = 0` (although
  with `attachmentCount = 0` it does not matter, leaving a non-zero mask
  is misleading).
- Allowing `desc.ColorAttachmentCount > 1` in Stage 6. Stage 6 explicitly
  caps it at 1 (or 0). Reject with `XE_ASSERT` if a future caller passes
  >1.
- Constructing a depth-only pipeline without a fragment shader. Vulkan
  requires a fragment stage even if no colour is written. The validation
  in `RHIResourceFactory` should reject null fragment shader.

## 9. What This Stage Intentionally Does Not Do

- Does **not** implement the actual `ShadowDepthPass` in the Renderer.
  Stage 9.
- Does **not** add depth bias / front-face culling state to
  `RHIGraphicsPipelineDesc`. Stage 9.
- Does **not** allow multi-colour attachments (`ColorAttachmentCount > 1`).
  Deferred.
- Does **not** remove `RHITexture::GetNativeImageView` or the
  `RHIDevice::CreateX` wrappers. Stage 8.
- Does **not** introduce a generic per-pass view API. RenderGraph V1.
- Does **not** add `RHICapabilities` (max colour attachments per subpass,
  independent blend, etc.). Stage 7.
