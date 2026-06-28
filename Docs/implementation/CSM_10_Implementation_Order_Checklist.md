# CSM_10 — Implementation Order and Final Checklist

## Goal

Provide a single, ordered checklist of the manual implementation steps required to bring Stage 9 V0 (CSM) into a working, validated state. Each step is small enough to commit independently and references the detailed documents above.

## Implementation Order

### Phase 1 — RHI Extensions and Data Layer (no rendering changes yet)

1. **Verify `RHITexture` array support.** [x] Done in RHI cleanup.
2. **Verify `RHITextureView` per-layer view support.** [x] Done in RHI cleanup.
3. **Verify `RHIResourceFactory::CreateTextureView` accepts `ArrayLayerCount = 1`.** [x] Done in RHI cleanup.
4. **Verify `RHIResourceFactory::CreateTextureView` accepts `ArrayLayerCount = 0` (all layers).** [x] Done in RHI cleanup.
5. **Add `MaxShadowCascades = 4` constant** to `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h`. [x] Already present.
6. **Add `RenderDirectionalShadowFrameData` struct** to `RenderShadowType.h`. [x] Already present.
7. **Add `RenderShadowCascade` struct** to `RenderShadowType.h`. [x] Already present.
8. **Add `RenderShadowFrameData` struct** to `RenderShadowType.h`. [x] Already present.
9. **Add `DirectionalShadowPlanDesc` struct** to `DirectionalShadowPlanner.h`. [x] Already present.
10. **Add `DirectionalShadowSettings` / `ShadowSettings` / `RendererSettings`** to `RendererSettings.h`. [x] Already present.
11. **Add `ShadowDebugSettings` / `RendererDebugSettings::Shadows`** to `RendererDebugSettings.h`. [x] Already present.
12. **Add `GPUCascadeShadowData` and `GPUShadowData` structs** with static_asserts in `Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h`. [x] Already present.
13. **Add `RenderShadowManager` class declaration** to `RenderShadowManager.h`. [x] Already present.
14. **Add `ShadowResourceCache` class declaration** to `ShadowResourceCache.h`. [x] Already present.
15. **Add `DirectionalShadowResources` / `DirectionalShadowResourceDesc`** to `ShadowResourceCache.h`. [x] Already present.

### Phase 2 — CPU Math (no rendering changes yet)

16. **Implement `ComputeCascadeSplits`** in `DirectionalShadowPlanner.cpp`. [x] Already implemented.
17. **Implement `GetCameraFrustumCornersWorldSpace`** in `DirectionalShadowPlanner.cpp`. [x] Already implemented.
18. **Implement `GetCascadeFrustumCornersWorldSpace`** in `DirectionalShadowPlanner.cpp`. [x] Already implemented.
19. **Implement `ComputeAverageCenter` / `ComputeBoundingSphereRadius`** in `DirectionalShadowPlanner.cpp`. [x] Already implemented.
20. **Implement `BuildLightBasis`** in `DirectionalShadowPlanner.cpp`. **Apply the fix in [CSM_04](CSM_04_DirectionalShadowPlanner_Coordinates.md#light-basis) (cross-product order) for left-handed consistency.** [ ] To do.
21. **Implement `ComputeTexelSnapOffset`** in `DirectionalShadowPlanner.cpp`. [x] Already implemented.
22. **Implement `BuildPlan`** in `DirectionalShadowPlanner.cpp`. [x] Already implemented (with the `AABB::FromPoints` patch).

### Phase 3 — ShadowResourceCache Implementation (no rendering changes yet)

23. **Implement `ShadowResourceCache::Initialize`**. [ ] To do (trivial).
24. **Implement `ShadowResourceCache::Shutdown`**. [ ] To do (trivial).
25. **Implement `ShadowResourceCache::GetOrCreateDirectionalShadowResources`** following [CSM_03](CSM_03_ShadowResources.md#shadowresourcecachegetorcreatedirectionalshadowresources). [ ] To do.

### Phase 4 — RenderShadowManager Implementation (no rendering changes yet)

26. **Implement `RenderShadowManager::Initialize`**. [ ] To do.
27. **Implement `RenderShadowManager::Shutdown`**. [ ] To do.
28. **Implement `RenderShadowManager::HasDirectionalShadow`**. [ ] To do.
29. **Implement `RenderShadowManager::GetFrameData`**. [ ] To do.
30. **Implement `RenderShadowManager::PrepareFrame`** with `FreezeShadowMatrices` support. [ ] To do.
31. **Implement `RenderShadowManager::PrepareDirectionalShadow`** following [CSM_05](CSM_05_RenderShadowManager.md#preparedirectionalshadow). [ ] To do.
32. **Implement `RenderShadowManager::FillGPUShadowData`** following [CSM_05](CSM_05_RenderShadowManager.md#fillgpushadowdata). [ ] To do.
33. **Add `RenderShadowManager` to `RenderSystem::Impl`**. [ ] To do.
34. **Initialize/shutdown `RenderShadowManager` in `RenderSystem::OnCreate`/`OnDestroy`**. [ ] To do.
35. **Add `ShadowManager*` to `RenderResourceContext`**. [ ] To do.
36. **Wire `ShadowManager*` in `RenderSystem`**. [ ] To do.

### Phase 5 — Shader Additions

37. **Add `Engine/Shaders/Lighting/ShadowTypes.slang`**. [ ] To do. See [CSM_08 § ShadowTypes.slang](CSM_08_Shaders.md#shadowtypesslang).
38. **Add `Engine/Shaders/Lighting/ShadowSampling.slang`**. [ ] To do. See [CSM_07 § Slang-Side Frame Data](CSM_07_FrameResources_And_ForwardSampling.md#slang-side-frame-data).
39. **Add `Engine/Shaders/Passes/DepthOnly.slang`**. [ ] To do. See [CSM_06 § Shader: DepthOnly.slang](CSM_06_ShadowDepthPass_And_Pipeline.md#shader-depthonlyslang) and [CSM_08 § DepthOnly.slang](CSM_08_Shaders.md#depthonlyslang).
40. **Update `Engine/Shaders/Common/Types.slang`** to include `GPUShadowData Shadow` in `GPUFrameData`. [ ] To do.
41. **Force-rebuild the `XEngineShader` module** to pick up the new shaders. [ ] To do.
42. **Add `RenderShaderLibrary` keys for `DepthOnlyVertex` and `DepthOnlyFragment`**. [ ] To do.

### Phase 6 — Frame Resources Integration

43. **Add `ShadowManager*` and `m_VisualizeCascades` to `RenderFrameResources`**. [ ] To do.
44. **Add setters `SetShadowManager` and `SetVisualizeCascades`**. [ ] To do.
45. **Extend the frame bind group layout** with shadow texture and sampler bindings. [ ] To do. See [CSM_07 § Frame Bind Group Layout](CSM_07_FrameResources_And_ForwardSampling.md#frame-bind-group-layout).
46. **Add a per-frame bind group recreation path** when shadow resources change. [ ] To do. See [CSM_07 § Per-Frame Bind Group Update](CSM_07_FrameResources_And_ForwardSampling.md#per-frame-bind-group-update).
47. **Extend `BuildGPUFrameData`** to fill `data.Shadow`. [ ] To do.
48. **Extend `RenderFrameResources::Update`** to call `m_ShadowManager->FillGPUShadowData`. [ ] To do.
49. **Extend `RenderFrameContext` with `CameraNear` and `CameraFar`** and set them in `RenderSystem::Render`. [ ] To do.

### Phase 7 — ShadowDepthPass

50. **Add the `AddShadowDepthPass` API** to `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.h`. [ ] To do.
51. **Implement `AddShadowDepthPass`** following [CSM_06](CSM_06_ShadowDepthPass_And_Pipeline.md#shadowdepthpasscpp-implementation). [ ] To do.
52. **Add `RenderPassKind::ShadowDepth`** to the pass-kind enum. [ ] To do.
53. **Add `ShadowDepthPushConstants`** to `RenderShaderTypes.h`. [ ] To do.
54. **Register `DepthOnly` shader in `RenderShaderLibrary`**. [ ] To do.
55. **Create the depth-only pipeline** (lazy / cached) with `HasColorAttachment = false` and `DepthFormat = D32Float`. [ ] To do.
56. **Add a layout transition** at the end of the shadow pass to `SHADER_READ_ONLY_OPTIMAL`. [ ] To do.

### Phase 8 — ForwardPass Integration

57. **Call `AddShadowDepthPass`** in `ForwardRenderPipeline::Render`. [ ] To do.
58. **Bind the frame bind group** in `AddForwardOpaquePass`. [x] Already binds the frame group.
59. **Update `ForwardPBR.slang`** to include `ShadowSampling.slang`. [ ] To do.
60. **Update `ForwardPBR.slang`** to compute the per-light shadow factor. [ ] To do.
61. **Update `Lighting.slang::EvaluateSceneLighting`** to accept a `shadowFactors[]` parameter. [ ] To do.
62. **Update `ForwardPBR.slang`** to pass the shadow factors to `EvaluateSceneLighting`. [ ] To do.
63. **Add the `VisualizeCascades` debug override** in `ForwardPBR.slang`. [ ] To do.

### Phase 9 — Editor UI

64. **Add the "Shadows" section** to `Engine/Source/Editor/Private/Panels/RendererDebugPanel.cpp`. [ ] To do.
65. **Wire the `VisualizeCascades` toggle** through `RenderSystem` → `FrameResources::SetVisualizeCascades`. [ ] To do.
66. **Wire the `FreezeShadowMatrices` toggle** through `RenderSystem` → `RenderShadowManager::PrepareFrame`. [ ] To do.
67. **Reserve the `ShowShadowMap` toggle** (deferred to Stage 10+). [ ] To do.

### Phase 10 — Validation and Polish

68. **Build the editor and sandbox targets** and resolve any compile errors. [ ] To do.
69. **Run the editor with validation enabled** and resolve any Vulkan validation errors. [ ] To do.
70. **Capture a frame in RenderDoc** and verify the shadow pass and forward pass. [ ] To do.
71. **Run the visual test suite** from [CSM_09](CSM_09_Editor_Debug_Validation.md#visual-tests). [ ] To do.

## Final Checklist

Before declaring Stage 9 V0 complete, verify the following:

### Build

- [ ] `XEngineRHI.lib` builds cleanly.
- [ ] `XEngineRenderer.lib` builds cleanly.
- [ ] `XEngineEditor.lib` builds cleanly.
- [ ] `XEngineEditorApp.exe` links cleanly.
- [ ] `XEngineSandbox.exe` links cleanly.

### Runtime

- [ ] Existing forward rendering still works (no regression).
- [ ] Existing material textures still bind.
- [ ] Existing offscreen / render target output still works.
- [ ] No Vulkan validation errors.
- [ ] No public Vulkan types leaked into RHI headers.
- [ ] No `RHITexture::GetDefaultView()` calls.
- [ ] No `dynamic_cast<VulkanX*>` calls.

### CSM-Specific

- [ ] Directional light with `CastShadow = false` disables shadow rendering (no extra passes, no GPU work).
- [ ] Directional light with `CastShadow = true` creates the shadow texture array on first frame.
- [ ] `ShadowDepthPass` renders one layer per cascade (`ShadowDepthPass.C0`, `ShadowDepthPass.C1`, ...).
- [ ] `ForwardPBR.slang` samples the correct cascade for each fragment.
- [ ] `VisualizeCascades` shows stable cascade regions.
- [ ] `FreezeShadowMatrices` stops shadow shimmering while the camera moves.
- [ ] Changing `CascadeCount` or `Resolution` recreates the shadow resources cleanly (no leaks, no validation errors).
- [ ] Changing `StorageMode != Texture2DArray` logs an error and disables shadows (V0 restriction).
- [ ] No right-handed matrix assumptions (test by setting the directional light along world +X; cascade should not flip).
- [ ] Cascade selection in the shader matches the `SplitFar` distances in the CPU planner.
- [ ] PCF 3x3 produces a soft shadow (not a hard single-tap result).
- [ ] `DepthBias` and `NormalBias` produce visible changes in shadow acne / peter-panning.
- [ ] The shadow texture array is transitioned to `SHADER_READ_ONLY_OPTIMAL` after the last cascade and before the forward pass.

### Resource State

- [ ] `ShadowTexture`, `SampledView`, `Sampler`, and per-layer `CascadeDepthViews` are all valid.
- [ ] `RenderShadowFrameData::Directional::Enabled` is `true` when a shadow-casting light is active, `false` otherwise.
- [ ] `RenderShadowFrameData::Directional::CascadeCount` matches the active settings.
- [ ] `GPUShadowData::ShadowParams` is correct (enabled, count, resolution, visualize).
- [ ] `GPUShadowData::Cascades[i].LightViewProjection` is correct.
- [ ] `GPUShadowData::Cascades[i].Params` is correct (splitFar, depthBias, normalBias, texelSize).

### Visual

- [ ] Shadow direction is consistent with the light direction.
- [ ] Shadow distance is correct at the camera near plane.
- [ ] Shadow distance is correct at the camera far plane.
- [ ] Cascades do not exhibit a visible seam (the union of cascade projections covers the full view frustum).
- [ ] No shadow acne on planar surfaces.
- [ ] No peter-panning (over-bias) on curved surfaces.

## Common Mistakes (Cross-Cutting)

1. **Forgetting to register the new shaders** in `RenderShaderLibrary` after adding `.slang` files. The library will fail to find them and the depth pipeline creation will fail.
2. **Stale shader binaries** after adding new `.slang` files. Force a clean rebuild of the shader module.
3. **Stale `XEngineRenderer.lib` or `XEngineEditor.lib`** after editing shadow code. Clean rebuild.
4. **Not running the Vulkan validation layer** during development. Layout transitions and bind group mismatches would silently produce wrong results.
5. **Not testing with the directional light aligned to world +X.** This is the degenerate case that the basis cross-product fix addresses.
6. **Adding `HasColorAttachment = true` to the shadow depth pipeline.** The shadow map is depth-only; the color attachment must be absent.
7. **Forgetting to flip the Y axis in the shadow UV transform.** Shadows would be vertically mirrored.
8. **Forgetting the depth bias in the shader.** Shadows would self-shadow (acne) on every surface.
9. **Using `OrthographicLH_ZO` with the wrong near/far order for reverse-Z.** The shadow map depth range would be flipped; the comparison would always pass or always fail.
10. **Forgetting the `RHIRenderOutputDesc::RenderToSwapchain = false` in `ShadowDepthPass`.** The pass would be treated as a swapchain pass; the depth attachment would be ignored.

## Out of Scope (Deferred to Future Stages)

- Bindless descriptor indexing.
- Ray-traced shadows.
- VSM / EVSM.
- Spot and point light shadows.
- Async compute shadow pass.
- RenderGraph transient resource allocator.
- Full automatic resource state tracking.
- Per-cascade frustum culling for shadow casters.
- `RHIBindGroupLayout` caching for shadow-specific layouts.
- Dedicated shadow map viewer in the editor UI.

This concludes the Stage 9 V0 CSM implementation plan.
