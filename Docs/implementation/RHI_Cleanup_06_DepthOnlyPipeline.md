# Stage 6 — Depth-Only Graphics Pipeline

## 1. Goal

Allow the existing graphics-pipeline and dynamic-rendering paths to operate
without a color attachment. This is the last generic RHI feature required by
Stage 9 `ShadowDepthPass`.

The current descriptor already has `bool HasColorAttachment`; keep it for this
stage:

```text
HasColorAttachment = true   one color attachment (current forward path)
HasColorAttachment = false  zero color attachments (depth-only path)
```

Do not replace it with `ColorAttachmentCount` until MRT is implemented. An
integer that rejects every value above one is not useful future-proofing.

## 2. Corrections to the Earlier Draft

The previous plan contained three implementation-breaking errors:

1. Vulkan does **not** require a fragment shader for a depth-only graphics
   pipeline. `FragmentShader == nullptr` is valid when
   `HasColorAttachment == false`; the backend must omit the fragment stage.
2. Depth bias belongs to `VkPipelineRasterizationStateCreateInfo`, not
   `VkPipelineDepthStencilStateCreateInfo`.
3. The pipeline and command-recording sides must agree. Creating a zero-color
   pipeline is insufficient if `vkCmdBeginRendering` still declares one color
   attachment.

The earlier `RHIGraphicsPipelineDesc::MakeDepthOnly` helper is also removed.
It embedded shadow-specific bias defaults in the generic RHI. Renderer code
should fill the generic descriptor explicitly.

## 3. Descriptor Changes

Extend the current `RHIGraphicsPipelineDesc` only with generic rasterization
state:

```cpp
struct RHIGraphicsPipelineDesc
{
    RHIShader* VertexShader = nullptr;
    RHIShader* FragmentShader = nullptr;

    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    bool HasColorAttachment = true;

    bool EnableDepthTest = true;
    bool EnableDepthWrite = true;

    bool EnableDepthBias = false;
    f32 DepthBiasConstantFactor = 0.0f;
    f32 DepthBiasClamp = 0.0f;
    f32 DepthBiasSlopeFactor = 0.0f;

    // existing vertex layout, bind-group layouts, push constants, name...
};
```

Bias values are pipeline state in Stage 6. If Stage 9 needs to tune them every
frame, add dynamic depth bias as a separate, deliberate command-list feature;
do not silently omit the values from the pipeline cache key.

## 4. Factory Validation

`RHIResourceFactory::CreateGraphicsPipeline` is the common validation point:

```cpp
if (desc.VertexShader == nullptr)
{
    return nullptr;
}

if (desc.HasColorAttachment)
{
    if (desc.FragmentShader == nullptr ||
        desc.ColorFormat == RHIFormat::Undefined)
    {
        return nullptr;
    }
}

if (!desc.HasColorAttachment && desc.DepthFormat == RHIFormat::Undefined)
{
    // With no color and no depth, this stage has no usable attachment.
    return nullptr;
}

if ((desc.EnableDepthTest || desc.EnableDepthWrite) &&
    desc.DepthFormat == RHIFormat::Undefined)
{
    return nullptr;
}
```

Also validate that every shader and bind-group layout belongs to the factory's
device. Stage 7 adds complete format/capability validation.

## 5. Vulkan Pipeline Implementation

### 5.1 Shader stages

Build the shader-stage vector conditionally:

```cpp
std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
shaderStages.push_back(MakeShaderStage(*vertexShader));

if (desc.FragmentShader != nullptr)
{
    auto* fragmentShader = CheckedVulkanCast<VulkanShader>(
        desc.FragmentShader, device);
    if (fragmentShader == nullptr)
    {
        return false;
    }
    shaderStages.push_back(MakeShaderStage(*fragmentShader));
}
```

For the Stage 9 shadow pipeline, leave `FragmentShader` null. If a later depth
pass intentionally uses fragment tests (for example alpha-masked shadows), it
may supply a fragment shader while still setting `HasColorAttachment = false`.

### 5.2 Dynamic-rendering formats

```cpp
const VkFormat colorFormat = desc.HasColorAttachment
    ? RHIFormatToVulkanFormat(desc.ColorFormat)
    : VK_FORMAT_UNDEFINED;
const VkFormat depthFormat = RHIFormatToVulkanFormat(desc.DepthFormat);

VkPipelineRenderingCreateInfo renderingInfo {};
renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingInfo.colorAttachmentCount = desc.HasColorAttachment ? 1u : 0u;
renderingInfo.pColorAttachmentFormats =
    desc.HasColorAttachment ? &colorFormat : nullptr;
renderingInfo.depthAttachmentFormat = depthFormat;
renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
```

Reject undefined color format only when a color attachment exists. Reject an
undefined depth format when depth test/write is enabled or when the pipeline
has no color attachment.

### 5.3 Color blend state

```cpp
VkPipelineColorBlendAttachmentState colorAttachment {};
colorAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

VkPipelineColorBlendStateCreateInfo colorBlend {};
colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlend.attachmentCount = desc.HasColorAttachment ? 1u : 0u;
colorBlend.pAttachments = desc.HasColorAttachment ? &colorAttachment : nullptr;
```

### 5.4 Rasterization depth bias

Apply bias to the existing rasterization state:

```cpp
rasterization.depthBiasEnable = desc.EnableDepthBias ? VK_TRUE : VK_FALSE;
rasterization.depthBiasConstantFactor = desc.DepthBiasConstantFactor;
rasterization.depthBiasClamp = desc.DepthBiasClamp;
rasterization.depthBiasSlopeFactor = desc.DepthBiasSlopeFactor;
```

Do not write these members on `VkPipelineDepthStencilStateCreateInfo`; they do
not exist there.

## 6. Dynamic-Rendering Command Path

Update `VulkanCommandList::BeginRenderingIfNeeded` so attachment count follows
the actual output descriptor:

```cpp
const bool hasColor = renderToSwapchain || m_RenderOutput.ColorTarget != nullptr;
const bool hasDepth = m_RenderOutput.DepthTarget != nullptr;

VkRenderingInfo renderingInfo {};
renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
renderingInfo.renderArea = renderArea;
renderingInfo.layerCount = 1;
renderingInfo.colorAttachmentCount = hasColor ? 1u : 0u;
renderingInfo.pColorAttachments = hasColor ? &colorAttachment : nullptr;
renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;
```

Rules:

- swapchain rendering always has one color attachment;
- offscreen color + depth keeps the existing path;
- depth-only requires `ColorTarget == nullptr`, a valid `DepthTarget`, and
  `ColorFormat == Undefined` by convention;
- reject an offscreen descriptor with neither color nor depth;
- derive attachment image views from Stage-5 view objects.

## 7. Renderer Pipeline Cache

`GraphicsPipelineStateKey` must contain every field that changes the native
pipeline. Add:

```cpp
bool HasColorAttachment = true;
bool EnableDepthBias = false;
f32 DepthBiasConstantFactor = 0.0f;
f32 DepthBiasClamp = 0.0f;
f32 DepthBiasSlopeFactor = 0.0f;
```

Update equality and hashing for all five fields. Hash floating-point values by
their bit representation (for example `std::bit_cast<u32>`), and normalize
`-0.0f` to `0.0f` when constructing the key.

For `RenderPassKind::ShadowDepth`, `RenderPipelineStateCache` constructs:

```cpp
RHIGraphicsPipelineDesc desc;
desc.VertexShader = shadowVertexShader;
desc.FragmentShader = nullptr;
desc.HasColorAttachment = false;
desc.ColorFormat = RHIFormat::Undefined;
desc.DepthFormat = key.DepthFormat;
desc.EnableDepthTest = true;
desc.EnableDepthWrite = true;
desc.EnableDepthBias = key.EnableDepthBias;
desc.DepthBiasConstantFactor = key.DepthBiasConstantFactor;
desc.DepthBiasClamp = key.DepthBiasClamp;
desc.DepthBiasSlopeFactor = key.DepthBiasSlopeFactor;
```

Shader paths, bias policy, and default bias values belong to the Renderer/
Stage 9 plan, not RHI.

## 8. Implementation Order

1. Add generic depth-bias fields to `RHIGraphicsPipelineDesc`.
2. Update common factory validation, including optional fragment shader.
3. Make Vulkan shader stages, rendering formats, color blend state, and
   rasterization bias conditional.
4. Make `vkCmdBeginRendering` use zero color attachments for depth-only output.
5. Extend `GraphicsPipelineStateKey` equality/hash and create the
   `ShadowDepth` branch.
6. Build and validate forward, editor offscreen, and depth-only paths.

## 9. Verification

- Existing forward pipelines still contain vertex + fragment stages and one
  color attachment.
- A depth-only test pipeline succeeds with `FragmentShader == nullptr`.
- RenderDoc shows zero color formats/blend attachments for that pipeline.
- `vkCmdBeginRendering` uses `colorAttachmentCount == 0` and
  `pColorAttachments == nullptr` for the test pass.
- Pipeline cache entries differ when any baked depth-bias value differs.
- Vulkan validation has no dynamic-rendering compatibility warnings.

## 10. Out of Scope

- MRT and `ColorAttachmentCount > 1`.
- Dynamic depth bias commands.
- Alpha-masked shadow materials.
- CSM planning/drawing.
- Full resource-state tracking.
