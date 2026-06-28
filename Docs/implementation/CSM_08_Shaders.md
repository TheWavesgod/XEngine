# CSM_08 — Shaders

## Goal

Define the shader-side implementation of CSM. Three shader files are involved:

1. `Engine/Shaders/Lighting/ShadowTypes.slang` — new, mirrors `GPUShadowData` in Slang.
2. `Engine/Shaders/Lighting/ShadowSampling.slang` — new, cascade selection and PCF3x3 sampling.
3. `Engine/Shaders/Passes/DepthOnly.slang` — new, depth-only VS/FS for the shadow pass.
4. `Engine/Shaders/Passes/ForwardPBR.slang` — modified, sample shadow, apply to direct lighting.

`Engine/Shaders/Common/Types.slang` is also extended to add `GPUShadowData Shadow` to `GPUFrameData`.

## File Layout

```text
Engine/Shaders/
  Common/
    Types.slang                       (modified: add GPUShadowData to GPUFrameData)
  Lighting/
    ShadowTypes.slang                 (new)
    ShadowSampling.slang              (new)
  Passes/
    DepthOnly.slang                   (new)
    ForwardPBR.slang                  (modified)
```

## `ShadowTypes.slang`

```hlsl
#pragma once

static const uint MAX_SHADOW_CASCADES = 4;

// Must match Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h
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
    // x = enabled
    // y = cascade count
    // z = shadow resolution
    // w = visualize cascades
    float4 ShadowParams;
    GPUCascadeShadowData Cascades[MAX_SHADOW_CASCADES];
};
```

Static asserts in the C++ side must continue to hold:

- `sizeof(GPUCascadeShadowData) == 80` (Mat4 64 + Vec4 16).
- `sizeof(GPUShadowData) == 16 + MaxShadowCascades * 80`.

The Slang side has no equivalent static_assert, but if a Slang-side assert is desired, use `[StaticAssert(sizeof(GPUCascadeShadowData) == 80)]` syntax. This is supported by Slang.

## `ShadowSampling.slang`

See [CSM_07](CSM_07_FrameResources_And_ForwardSampling.md) for the full implementation. The key functions are:

- `SelectCascadeIndex(viewSpaceDepth, shadow)` — returns the cascade index.
- `ComputeShadowFactor(shadow, filterMode, worldPos, viewSpaceDepth, NdotL)` — returns the shadow factor in `[0, 1]`.
- `SampleShadowPCF3x3(shadow, cascadeIndex, worldPos, refDepth, NdotL)` — internal helper.
- `SampleShadow1Tap(shadow, cascadeIndex, worldPos, refDepth, NdotL)` — internal helper.

The full code is in [CSM_07 § "Slang-Side Frame Data"](CSM_07_FrameResources_And_ForwardSampling.md#slang-side-frame-data).

## `DepthOnly.slang`

```hlsl
#include "../Common/Types.slang"

struct ShadowDepthPushConstants
{
    float4x4 Model;
    float4x4 LightViewProjection;
    uint     CascadeIndex;
    uint     _pad0;
    uint     _pad1;
    uint     _pad2;
};

[[vk::push_constant]]
ConstantBuffer<ShadowDepthPushConstants> g_Push;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(g_Push.Model, float4(input.position, 1.0));
    output.position = mul(g_Push.LightViewProjection, worldPos);
    return output;
}

// Depth-only fragment: emits SV_Depth from the rasterizer, no color write.
[shader("fragment")]
void fragmentMain(VSOutput input)
{
    // No color output. The pipeline has HasColorAttachment = false.
    // SV_Depth is derived from input.position by the rasterizer.
}
```

Notes:

- `VSInput` and `VSOutput` are reused from `Engine/Shaders/Common/Types.slang`. The vertex shader uses `input.position` (location 0, POSITION) and ignores `input.normal`, `input.uv`. The unused outputs (`normal`, `uv`) on `VSOutput` are not written and may be optimized out.
- The fragment shader is empty. Slang may require a non-void return; if so, add `[ForceInline]` and either `return;` or a dummy `return float4(0,0,0,0);`. The pipeline discards color writes.
- The push constant block has padding (`_pad0..2`) to reach 16-byte alignment. The C++ side must match this layout exactly.

## `ForwardPBR.slang` Integration

The current `ForwardPBR.slang` is in `Engine/Shaders/Passes/ForwardPBR.slang`. The relevant changes:

1. Add the include for `ShadowSampling.slang`.
2. Add the include for `EvaluateSceneLighting`'s shadow factor parameter (or restructure as documented in [CSM_07](CSM_07_FrameResources_And_ForwardSampling.md)).
3. Compute the per-light shadow factor and pass it to the lighting evaluation.

```hlsl
#include "../Lighting/ShadowSampling.slang"

// In fragmentMain, after evaluating the base color and surface:

// Compute view-space depth for cascade selection.
float4 viewPos4 = mul(g_FrameData.Camera.View, float4(input.worldPosition, 1.0));
float viewDepth = viewPos4.x;  // XEngine: camera forward is +X in view space.

// Compute per-light shadow factors. Stage 8C has a small fixed-size light
// array; in V0 only the first directional light is shadow-casting.
float shadowFactors[MAX_GPU_LIGHTS];
[unroll]
for (uint li = 0; li < MAX_GPU_LIGHTS; ++li)
{
    shadowFactors[li] = 1.0;
}

uint lightCount = (uint)g_FrameData.Lighting.LightCountAndPadding.x;
[unroll]
for (li = 0; li < MAX_GPU_LIGHTS && li < lightCount; ++li)
{
    GPULight light = g_FrameData.Lighting.Lights[li];
    if (light.DirectionType.w == 0.0 &&  // directional
        light.SpotAnglesShadow.z > 0.5)   // CastShadow
    {
        float NdotL = saturate(dot(surface.Normal, light.DirectionType.xyz));
        int filterMode = (g_FrameData.Shadow.ShadowParams.y > 0.0) ? 1 : 0;
        shadowFactors[li] = ComputeShadowFactor(
            g_FrameData.Shadow,
            filterMode,
            input.worldPosition,
            viewDepth,
            NdotL);
    }
}

// Pass to the lighting evaluation.
float3 color = EvaluateSceneLighting(
    g_FrameData.Lighting,
    surface,
    input.worldPosition,
    surface.Normal,
    V,
    shadowFactors);
```

`EvaluateSceneLighting` must be extended to accept `shadowFactors`. The current signature is in `Engine/Shaders/Lighting/Lighting.slang`:

```hlsl
float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 N,
    float3 V);
```

Add the parameter:

```hlsl
float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 N,
    float3 V,
    float shadowFactors[MAX_GPU_LIGHTS])
{
    // Existing per-light loop. For each light, multiply the direct contribution
    // by shadowFactors[i] (or skip if 1.0).
    float3 result = surface.AO * lighting.AmbientColorIntensity.rgb * lighting.AmbientColorIntensity.w * surface.Albedo;
    for (uint i = 0; i < MAX_GPU_LIGHTS; ++i)
    {
        if (i >= (uint)lighting.LightCountAndPadding.x) break;
        GPULight light = lighting.Lights[i];
        float3 directContribution = /* existing per-light BRDF eval */;
        result += directContribution * shadowFactors[i];
    }
    return result;
}
```

The exact body depends on the current implementation. Acceptable refactor: introduce the parameter at the end of the parameter list (with a default), so existing call sites continue to work. Slang supports default parameters:

```hlsl
float3 EvaluateSceneLighting(
    GPULightingData lighting,
    SurfaceData surface,
    float3 worldPosition,
    float3 N,
    float3 V,
    float shadowFactors[MAX_GPU_LIGHTS] = {})  // default: all zero (treated as 1.0)
{
    // ...
}
```

With a default empty array, existing call sites that pass no shadow factors get `shadowFactors[i] = 0.0`, which would zero out the lighting. The helper must interpret `0.0` as "no shadowing" and default to `1.0`:

```hlsl
float SafeShadowFactor(float f) { return (f == 0.0) ? 1.0 : f; }
// ...
result += directContribution * SafeShadowFactor(shadowFactors[i]);
```

Or: have the caller always pass an initialized array (preferred for clarity).

## Visualize Cascades

When `g_FrameData.Shadow.ShadowParams.w > 0.5`, the fragment shader overrides the final color with a cascade-color visualization:

```hlsl
if (g_FrameData.Shadow.ShadowParams.w > 0.5)
{
    int cascadeIndex = SelectCascadeIndex(viewDepth, g_FrameData.Shadow);
    float3 cascadeColor = float3(
        cascadeIndex == 0 ? 1.0 : 0.0,
        cascadeIndex == 1 ? 1.0 : 0.0,
        cascadeIndex == 2 ? 1.0 : 0.0);
    return float4(cascadeColor, 1.0);
}
```

This is a debug overlay. It is applied after the main lighting evaluation and overrides the final color.

## Slang-to-SPIR-V Notes

- `Texture2DArray<float>` is the type for the shadow map. The shadow comparison is done in the shader (no `SampleCmp`).
- `[vk::binding(N, set)]` is the syntax for Vulkan binding declarations.
- `[[vk::push_constant]]` is the syntax for push constants. The push constant block must match the C++ `RHIBufferRange` or the `RHIPushConstants` layout.
- `[[vk::location(N)]]` is the syntax for vertex input/output locations. Match the C++ `RHIVertexAttributeDesc::Location`.

The shader compiler is Slang. The compiled SPIR-V is then handed to Vulkan. There is no HLSL path; Slang generates SPIR-V directly.

## Common Mistakes

1. **Wrong view-space depth extraction.** XEngine uses +X for camera forward in view space. `viewDepth = viewPos.x`, not `viewPos.z`. Using `viewPos.z` would select the wrong cascade for every fragment.

2. **Forgetting to apply `shadowFactors[i]` to the per-light direct contribution.** If you apply it to the final color, the ambient and emissive terms are also darkened. Multiply per-light, not globally.

3. **Forgetting the `Y` flip in the UV transform.** Vulkan clip space has Y pointing down. The texture's V coordinate is Y-up. Flipping is `uv.y = -clip.y / clip.w * 0.5 + 0.5` (i.e. `* float2(0.5, -0.5) + float2(0.5, 0.5)`).

4. **Clamping the UV to `[0, 1]` in the wrong way.** The PCF offset is added before the clamp. The order is:
   - Compute the projected UV (which can be outside `[0, 1]`).
   - Add the PCF offset.
   - Clamp with `saturate`.
   - Sample.

5. **Forgetting to handle `cascadeIndex == -1` (or last cascade overflow).** The `SelectCascadeIndex` function returns the last cascade index for depths past the last split. The shader must handle this case (e.g. `gpuShadowMap.Sample(g_ShadowSampler, float3(uv, layer))` with `layer = lastCascade` is correct; the loop just needs to bound correctly).

6. **Using a `Texture2DArray` with `SampledTexture` binding type in the bind group layout but `CombinedImageSampler` in the binding resource.** They are different binding types. For the shadow texture array, use `RHIBindingType::SampledTexture` (not combined) and bind the sampler separately.

7. **Adding `ShadowTypes.slang` to a wrong include path.** The shader must be included from `Common/Types.slang` and from `ShadowSampling.slang`. Use relative paths: `../Lighting/ShadowTypes.slang`.

8. **Compiling the depth-only shader with a `ColorAttachment` enabled.** The pipeline's `RHIGraphicsPipelineDesc::HasColorAttachment` must be `false`. If the pipeline cache creates a forward-style pipeline for the shadow pass by accident, the depth writes are silently dropped or the validation layer complains.

9. **Not handling the `g_FrameData.Shadow.ShadowParams.x == 0` case early in `ComputeShadowFactor`.** If the shadow manager is not initialized or shadows are disabled, the function must return `1.0` immediately. Otherwise, a null texture would cause a Vulkan validation error or a garbage result.
