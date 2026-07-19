# CSM_07 — Frame Resources, Bind Groups, and Forward Sampling

## Goal

This document covers three things that tie the renderer together with the CSM pipeline:

1. The `GPUShadowData` upload as part of `GPUFrameData`.
2. The frame bind group layout extension to include the shadow texture array and the shadow sampler.
3. The `ForwardOpaquePass` and `ForwardPBR.slang` integration that samples the shadow map and applies the visibility factor to direct lighting.

## Files to Modify

- `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.h/.cpp` — extend `BuildGPUFrameData` to include shadow data.
- `Engine/Source/Runtime/Renderer/Private/Resources/RenderResourceContext.h` — add `ShadowManager*` (already covered in [CSM_06](CSM_06_ShadowDepthPass_And_Pipeline.md); repeated here for context).
- `Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp` — bind the shadow texture + sampler (no-op if shadow bind group is shared with frame bind group).
- `Engine/Shaders/Common/Types.slang` — add `GPUShadowData Shadow` to `GPUFrameData`.
- `Engine/Shaders/Passes/ForwardPBR.slang` — sample shadow, apply to direct lighting.
- `Engine/Shaders/Lighting/ShadowTypes.slang` — new file, mirror `GPUShadowData` in Slang.
- `Engine/Shaders/Lighting/ShadowSampling.slang` — new file, cascade selection and PCF.

## Frame Bind Group Layout

The current `RenderFrameResources` has one bind group for `GPUFrameData` at `binding(0, 0)`. The group layout has one entry:

```cpp
RHIBindGroupLayoutDesc layoutDesc;
layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
    0,
    RHIBindingType::UniformBuffer,
    RHIShaderStageFlags::AllGraphics,
    1
});
```

For CSM, the frame bind group must also expose:

- The shadow texture array (whole-array `Texture2DArray` sampled view).
- The shadow sampler.

These are at `binding(1, 0)` and `binding(2, 0)` respectively, or any two free slots in set 0. The exact slots are a Slang/RHI coordination point; the recommended layout is:

```text
Set 0:
  binding 0 = uniform buffer (GPUFrameData)
  binding 1 = sampled texture (Texture2DArray, shadow map)
  binding 2 = sampler (shadow comparison)
```

The frame bind group layout becomes:

```cpp
layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
    0, RHIBindingType::UniformBuffer, RHIShaderStageFlags::AllGraphics, 1
});
layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
    1, RHIBindingType::SampledTexture, RHIShaderStageFlags::Fragment, 1
});
layoutDesc.Entries.push_back(RHIBindGroupLayoutEntry {
    2, RHIBindingType::Sampler, RHIShaderStageFlags::Fragment, 1
});
```

The corresponding `RHIBindGroupDesc::Resources` adds:

```cpp
bindGroupDesc.Resources.push_back(RHIBindingResource {
    1, RHIBindingType::SampledTexture, shadowSampledView, nullptr, nullptr, 0, 0
});
bindGroupDesc.Resources.push_back(RHIBindingResource {
    2, RHIBindingType::Sampler, nullptr, shadowSampler, nullptr, 0, 0
});
```

When shadows are disabled, the texture and sampler are null. The shader must guard against null binding (the `RHIBindGroupLayout` validator will already check that the binding types match; passing null for a `SampledTexture` is allowed at the RHI level as long as the shader does not dereference it).

The current `RHIResourceFactory::CreateBindGroup` validator does **not** check for null texture or sampler in `RHIBindingResource`. The shader is responsible for not sampling when `ShadowParams.x < 0.5`. This is fine for V0.

## `RenderFrameResources::Update` Extension

```cpp
void RenderFrameResources::Update(const RenderFrameContext& frame, const RenderScene& scene)
{
    RHIBuffer* buffer = GetFrameBuffer(frame.FrameIndex);
    if (buffer == nullptr)
    {
        return;
    }

    GPUFrameData data = BuildGPUFrameData(frame, scene);

    // Fill the shadow field from the manager.
    if (m_ShadowManager != nullptr)
    {
        m_ShadowManager->FillGPUShadowData(data.Shadow);
        // The visualize-cascades flag is owned by the debug panel; set it here
        // because the manager is not aware of debug overlays.
        data.Shadow.ShadowParams.w = m_VisualizeCascades ? 1.0f : 0.0f;
    }
    else
    {
        // No shadow manager: zero out the data.
        data.Shadow = {};
    }

    if (!buffer->Update(&data, sizeof(data)))
    {
        XENGINE_LOG_ERROR("Failed to update GPUFrameData buffer");
    }
}
```

The `m_ShadowManager` and `m_VisualizeCascades` members are added to `RenderFrameResources`:

```cpp
class RenderFrameResources
{
public:
    // ... existing API ...

    void SetShadowManager(RenderShadowManager* manager) { m_ShadowManager = manager; }
    void SetVisualizeCascades(bool enable) { m_VisualizeCascades = enable; }

private:
    RenderShadowManager* m_ShadowManager = nullptr;
    bool m_VisualizeCascades = false;
    // ... existing members ...
};
```

`RenderSystem` wires these:

```cpp
// In RenderSystem::Render or OnCreate:
impl.FrameResources->SetShadowManager(impl.ShadowManager.get());
impl.FrameResources->SetVisualizeCascades(impl.DebugSettings.Shadows.VisualizeCascades);
```

The `VisualizeCascades` mirror at `RendererDebugSettings::VisualizeCascades` (top level) is also accepted for backward compatibility. Read from `Shadows.VisualizeCascades` only.

## Per-Frame Bind Group Update

The frame bind group is created once in `RenderFrameResources::Initialize` and bound once per pass. However, the texture and sampler references in the bind group become stale if `ShadowResourceCache` recreates the resources (e.g. resolution or cascade count change).

There are two options:

1. **Recommended — recreate the frame bind group when the shadow resources change.** `RenderFrameResources::Update` checks if the shadow texture/sampler pointers have changed since the last frame; if so, it calls `factory.CreateBindGroup` again. The `RHIBindGroup` is reference-counted, so the old group is freed automatically.

2. **Alternative — recreate the bind group on every frame.** Wasteful but simple. For V0 with 3 frames in flight, this is a 3x per-frame cost. Not recommended.

Use option 1. Pseudocode:

```cpp
void RenderFrameResources::Update(...)
{
    if (m_ShadowManager != nullptr)
    {
        const RenderShadowFrameData& shadowFrame = m_ShadowManager->GetFrameData();
        RHITextureView* newSampled = shadowFrame.Directional.SampledView;
        RHISampler* newSampler = shadowFrame.Directional.Sampler;
        if (newSampled != m_LastBoundShadowView || newSampler != m_LastBoundShadowSampler)
        {
            m_LastBoundShadowView = newSampled;
            m_LastBoundShadowSampler = newSampler;
            // Recreate all m_FrameBindGroups[i] with the new view/sampler.
            RecreateFrameBindGroups();
        }
    }
    // ... upload GPUFrameData ...
}
```

`RecreateFrameBindGroups` is a private helper that rebuilds all `m_FrameBindGroups[i]` with the new shadow view/sampler.

If shadow resources are null (e.g. no shadow-casting light), the bind group is recreated with null texture/sampler entries. The shader guards with `ShadowParams.x < 0.5`.

## Slang-Side Frame Data

In `Engine/Shaders/Common/Types.slang`:

```hlsl
#include "../Lighting/ShadowTypes.slang"

struct GPUFrameData
{
    GPUCameraData   Camera;
    GPULightingData Lighting;
    GPUShadowData   Shadow;  // NEW
};
```

`ShadowTypes.slang` (new file, in `Engine/Shaders/Lighting/`):

```hlsl
#pragma once

static const uint MAX_SHADOW_CASCADES = 4;

// Must match the C++ GPUShadowData layout in
// Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h.
struct GPUCascadeShadowData
{
    float4x4 LightViewProjection;
    // x = split far in view-space depth
    // y = depth bias
    // z = normal bias
    // w = texel size (1.0 / resolution)
    float4 Params;
};

struct GPUShadowData
{
    // x = enabled (0/1)
    // y = cascade count
    // z = shadow resolution
    // w = visualize cascades (0/1)
    float4 ShadowParams;
    GPUCascadeShadowData Cascades[MAX_SHADOW_CASCADES];
};
```

`ShadowSampling.slang` (new file):

```hlsl
#pragma once

#include "ShadowTypes.slang"
#include "../Common/Types.slang"
#include "../Common/Math.slang"

// Set 0, binding 1: shadow texture array (Texture2DArray).
[[vk::binding(1, 0)]]
Texture2DArray<float> g_ShadowMap;

// Set 0, binding 2: shadow sampler.
[[vk::binding(2, 0)]]
SamplerState g_ShadowSampler;

// Selects the cascade index for a given view-space depth.
// Returns 0 if depth is below the first split, last cascade if depth is past
// the last split. Returns 0 if shadows are disabled.
int SelectCascadeIndex(float viewSpaceDepth, GPUShadowData shadow)
{
    uint cascadeCount = (uint)shadow.ShadowParams.y;
    if (cascadeCount == 0) return 0;
    uint lastCascade = cascadeCount - 1;

    int selected = (int)lastCascade;
    for (uint i = 0; i < lastCascade; ++i)
    {
        if (viewSpaceDepth < shadow.Cascades[i].Params.x)
        {
            selected = (int)i;
            break;
        }
    }
    return selected;
}

// Projects a world-space position into the cascade's clip space.
float4 ProjectToCascadeClip(float3 worldPos, float4x4 lightViewProj)
{
    return mul(lightViewProj, float4(worldPos, 1.0));
}

// Converts clip-space to shadow UV. Reverse-Z: the depth test passes when
// the shadow map's depth is greater than or equal to the reference depth.
float2 ClipToShadowUV(float4 clip)
{
    return clip.xy / clip.w * float2(0.5, -0.5) + float2(0.5, 0.5);  // flip Y for Vulkan
}

float ClipToShadowDepth(float4 clip)
{
    return clip.z / clip.w;
}

// Single-tap shadow test. Returns 1.0 if lit, 0.0 if in shadow.
float SampleShadow1Tap(GPUShadowData shadow, int cascadeIndex,
                      float3 worldPos, float refDepth, float NdotL)
{
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float2 uv = saturate(ClipToShadowUV(clip));
    float layer = (float)cascadeIndex;
    float shadowMapDepth = g_ShadowMap.Sample(g_ShadowSampler, float3(uv, layer)).r;
    float biasedRef = refDepth + shadow.Cascades[cascadeIndex].Params.y
                    + max(0.0, 1.0 - NdotL) * shadow.Cascades[cascadeIndex].Params.z;
    return (shadowMapDepth >= biasedRef) ? 1.0 : 0.0;
}

// 3x3 PCF. Average 9 taps for a soft shadow.
float SampleShadowPCF3x3(GPUShadowData shadow, int cascadeIndex,
                         float3 worldPos, float refDepth, float NdotL)
{
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float2 uv = ClipToShadowUV(clip);
    if (any(uv < 0.0) || any(uv > 1.0))
    {
        // Outside the shadow map: treat as fully lit (avoid border artifacts).
        return 1.0;
    }
    float texelSize = shadow.Cascades[cascadeIndex].Params.w;
    float layer = (float)cascadeIndex;
    float biasedRef = refDepth + shadow.Cascades[cascadeIndex].Params.y
                    + max(0.0, 1.0 - NdotL) * shadow.Cascades[cascadeIndex].Params.z;

    float sum = 0.0;
    [unroll]
    for (int dy = -1; dy <= 1; ++dy)
    {
        [unroll]
        for (int dx = -1; dx <= 1; ++dx)
        {
            float2 offset = float2(dx, dy) * texelSize;
            float2 sampleUV = saturate(uv + offset);
            float sampleDepth = g_ShadowMap.Sample(g_ShadowSampler, float3(sampleUV, layer)).r;
            sum += (sampleDepth >= biasedRef) ? 1.0 : 0.0;
        }
    }
    return sum / 9.0;
}

// Returns the shadow factor in [0, 1]. 1.0 = fully lit, 0.0 = fully shadowed.
float ComputeShadowFactor(GPUShadowData shadow, int filterMode,
                          float3 worldPos, float viewSpaceDepth, float NdotL)
{
    if (shadow.ShadowParams.x < 0.5) return 1.0;  // shadows disabled

    int cascadeIndex = SelectCascadeIndex(viewSpaceDepth, shadow);

    // refDepth in [0, 1] reverse-Z, derived from world position projected to clip.
    float4 clip = ProjectToCascadeClip(worldPos, shadow.Cascades[cascadeIndex].LightViewProjection);
    float refDepth = ClipToShadowDepth(clip);

    if (filterMode == 0)  // Hard
    {
        return SampleShadow1Tap(shadow, cascadeIndex, worldPos, refDepth, NdotL);
    }
    else                  // PCF3x3
    {
        return SampleShadowPCF3x3(shadow, cascadeIndex, worldPos, refDepth, NdotL);
    }
}
```

`SelectCascadeIndex` and `ComputeShadowFactor` are the public entry points. `ForwardPBR.slang` calls `ComputeShadowFactor(g_FrameData.Shadow, /* filterMode = */ 1, worldPos, viewDepth, NdotL)` per directional light and multiplies the result into the direct lighting term.

## `ForwardPBR.slang` Integration

In `Engine/Shaders/Passes/ForwardPBR.slang`:

```hlsl
#include "../Lighting/ShadowSampling.slang"

// In the fragment shader, after computing surface and per-light contributions:

// Compute view-space depth for cascade selection.
float4 viewPos = mul(g_FrameData.Camera.View, float4(input.worldPosition, 1.0));
float viewDepth = viewPos.x;  // XEngine: camera forward is +X in view space

// Apply shadow factor for the directional light(s).
// Stage 8C has a single directional light. For V0, apply shadow to whichever
// directional light is at index 0 and has CastShadow = 1.
if (g_FrameData.Lighting.LightCountAndPadding.x > 0.0)
{
    uint lightIndex = 0;
    if (g_FrameData.Lighting.Lights[lightIndex].DirectionType.w == 0.0  // LIGHT_TYPE_DIRECTIONAL
        && g_FrameData.Lighting.Lights[lightIndex].SpotAnglesShadow.z > 0.5)  // CastShadow
    {
        float3 L = g_FrameData.Lighting.Lights[lightIndex].DirectionType.xyz;
        float NdotL = saturate(dot(surface.Normal, L));
        int filterMode = (g_FrameData.Shadow.ShadowParams.y > 0.0) ? 1 : 0;
        float shadowFactor = ComputeShadowFactor(
            g_FrameData.Shadow,
            filterMode,
            input.worldPosition,
            viewDepth,
            NdotL);
        // Modulate the directional light contribution.
        // (The exact integration depends on how Stage 8C structures EvaluateSceneLighting;
        // see the section "Integration with EvaluateSceneLighting" below.)
    }
}
```

### Integration with `EvaluateSceneLighting`

`Engine/Shaders/Lighting/Lighting.slang` defines `EvaluateSceneLighting`. For V0, the simplest path is to apply the shadow factor after the function returns:

```hlsl
float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 N,
    float3 V);

// In ForwardPBR.slang:
float3 color = EvaluateSceneLighting(...);
// For V0, apply shadow by multiplying the directional contribution only.
// The directional contribution is stored in `light.DirectionType.w == 0`.
// Easiest: recompute the diffuse factor here, multiply by shadow, and subtract.
if (g_FrameData.Lighting.LightCountAndPadding.x > 0.0)
{
    // ... as above, computing shadowFactor ...
    // We can't easily subtract the unshadowed contribution from `color`.
    // The cleanest path is to extend EvaluateSceneLighting to accept a shadow
    // factor or to expose the directional-only contribution.
}
```

A more elegant refactor (acceptable for V0) is to extend `EvaluateSceneLighting` with a shadow factor parameter:

```hlsl
float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 N,
    float3 V,
    float3 shadowFactors);  // NEW: per-light shadow factor, 1.0 = lit, 0.0 = shadowed
```

Where `shadowFactors[i]` corresponds to `lighting.Lights[i]`. If the array is larger than `MAX_GPU_LIGHTS`, the rest is 1.0.

This requires modifying `Lighting.slang` and `ForwardPBR.slang` together. The change is small but touches the lighting module. Acceptable for Stage 9 V0.

If a smaller change is preferred, leave `EvaluateSceneLighting` unchanged and apply the shadow factor in a post-process:

```hlsl
// Compute shadow factor for the directional light, then re-evaluate just the
// directional contribution and blend. Stage 9 V0 acceptance: this is hacky
// but works.

// 1. Save the unshadowed result.
float3 unshadowed = EvaluateSceneLighting(...);

// 2. Re-evaluate only the directional contribution with shadowFactor applied.
float3 directionalShadowed = ComputeDirectionalLight(...) * shadowFactor;
float3 directionalUnshadowed = ComputeDirectionalLight(...);
float3 color = unshadowed - directionalUnshadowed + directionalShadowed;
```

This requires the BRDF code to be split into a per-light helper. For V0, the recommended approach is the cleaner `EvaluateSceneLighting(..., shadowFactors)` extension.

## Layout Transitions

The shadow texture array is written by `ShadowDepthPass` (depth attachment) and read by `ForwardOpaquePass` (sampled view). The current `VulkanCommandList` performs an implicit layout transition:

- For depth attachments: `BeginRenderingIfNeeded` transitions the depth view to `VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL` on the first write.
- For sampled views: `VulkanBindGroup::Create` uses `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` for sampled image bindings (see `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp` line 171).

These two layouts require a barrier between the depth pass and the sampling pass. The barrier is currently not inserted automatically; the layout transition is performed by `vkCmdPipelineBarrier` inside `VulkanCommandList::TransitionDepthImage` if a transition is requested.

**Action item:** In `ShadowDepthPass` end, after the last cascade writes, transition the shadow array back to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` for sampling. The simplest way is to call:

```cpp
commandList->TransitionTextureToShaderRead(dir.SampledView);
```

at the end of the pass. This inserts a barrier and updates the texture's tracked layout.

The current `TransitionTextureToShaderRead` implementation in `VulkanCommandList` handles both color and depth aspects; depth textures transition to `SHADER_READ_ONLY_OPTIMAL` with a depth-aspect barrier. See `VulkanCommandList.cpp` line 149-174.

Verify that the transition handles `Texture2DArray` correctly. The current code uses `texture.GetDesc().ArrayLayers` to set the barrier's `layerCount` — for a `Texture2DArray` this is the cascade count, which is correct.

## Common Mistakes

1. **Forgetting to recreate the frame bind group when shadow resources change.** The bind group holds raw pointers to the texture and sampler views. If `ShadowResourceCache` rebuilds them (resolution change, cascade count change), the bind group's pointers are stale. Either recreate the bind group or accept that resolution/cascade count changes require a restart.

2. **Setting the cascade count in `ShadowParams.y` to the wrong value.** It must be `dir.CascadeCount` (the active count, not `MaxShadowCascades`).

3. **Using `texelSize` (1.0 / resolution) for the PCF offset.** The PCF offset is `±1 texel` in UV space, which is `±texelSize` (1.0 / resolution). Verify that `Params.w` is `1.0 / resolution` (not `2.0 / resolution`, which would be the case if the orthographic projection is normalized to `[-1, 1]`).

4. **Flipping the Y axis in the UV transform.** Vulkan has Y-down in clip space; after projection the Y must be flipped (`* float2(0.5, -0.5)`) to get the texture's V coordinate. Forgetting the flip produces shadows that are vertically mirrored.

5. **Forgetting to apply the depth bias in the shader.** The CPU planner stores the bias in `BiasParams` (`Cascades[i].BiasParams.x = depthBias`) and forwards it via `GPUCascadeShadowData::Params.y`. The shader must add it to `refDepth` before the comparison.

6. **Forgetting to clamp the UV to `[0, 1]` before sampling.** The shadow sampler has `ClampToBorder` with `OpaqueBlack` border color. Sampling outside `[0, 1]` returns `0.0`, which the comparison interprets as "in shadow". Clamping to `[0, 1]` is required. The `SampleShadow1Tap` and `SampleShadowPCF3x3` helpers above use `saturate` for this.

7. **Treating the cascade's `SplitFar` as a view-space depth vs a world-space distance.** It is a view-space depth. The shader's `viewDepth` is also in view space. Do not mix.

8. **Letting the shadow sampler filter the depth comparison.** The V0 sampler is a regular bilinear sampler (not a comparison sampler). All comparison math is in the shader. If you switch to a comparison sampler (`SampleCmpLevel`), the comparison must move to the sampler and the shader simplifies.
