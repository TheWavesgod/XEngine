# CSM_09 — Editor Debug UI, Validation, and Verification

## Goal

Cover three things:

1. Editor UI for the shadow debug toggles.
2. Validation of the CSM configuration at runtime.
3. Verification steps (build, runtime, RenderDoc).

## Editor Debug UI

### Required UI Controls

In `Engine/Source/Editor/Private/Panels/RendererDebugPanel.cpp`, the panel must read and write:

- `RendererDebugSettings::Shadows.VisualizeCascades` (bool, checkbox)
- `RendererDebugSettings::Shadows.FreezeShadowMatrices` (bool, checkbox)
- `RendererDebugSettings::Shadows.ShowShadowMap` (bool, checkbox — not implemented in V0; document as a future item)
- `RendererDebugSettings::Shadows.DebugCascadeLayer` (u32, slider 0..3)

The current `RendererDebugPanel.cpp` has placeholders. The new code (pseudocode):

```cpp
if (ImGui::CollapsingHeader("Shadows"))
{
    ShadowDebugSettings& s = context.RendererDebug->Shadows;

    ImGui::Checkbox("Visualize cascades", &s.VisualizeCascades);
    ImGui::Checkbox("Freeze shadow matrices", &s.FreezeShadowMatrices);
    ImGui::Checkbox("Show shadow map (Stage 10+)", &s.ShowShadowMap);
    ImGui::SliderInt("Debug cascade layer", reinterpret_cast<int*>(&s.DebugCascadeLayer), 0, 3);
}
```

The `context.RendererDebug` is the `RendererDebugSettings*` from `EditorContext`. `EditorSystem::OnCreate` already wires it.

### Visualize Cascades

When enabled, the `ForwardPBR.slang` fragment shader overrides the final color with a per-cascade color. See [CSM_08 § Visualize Cascades](CSM_08_Shaders.md#visualize-cascades). The toggle propagates through `RenderSystem` → `RenderFrameResources::SetVisualizeCascades` → `GPUFrameData.Shadow.ShadowParams.w`.

### Freeze Shadow Matrices

When enabled, `RenderShadowManager::PrepareFrame` stops recomputing matrices and reuses the snapshot. The data flows through `m_FrozenFrameData`.

### Show Shadow Map (deferred)

A dedicated shadow map viewer requires either an ImGui image or an offscreen debug pass that samples a single cascade layer. This is out of scope for V0. The toggle is reserved but unchecked; the comment notes that it is a future feature.

## Validation

### `RenderShadowManager::PrepareFrame` Checks

- `settings.Enabled` — master switch.
- `settings.Technique == DirectionalShadowTechnique::CascadedShadowMaps` — V0 only.
- `settings.StorageMode == ShadowMapStorageMode::Texture2DArray` — V0 only.
- `settings.CascadeCount >= 1 && settings.CascadeCount <= MaxShadowCascades`.
- `settings.Resolution` is a power of two (recommended; not strictly required).
- `desc.Light != nullptr && desc.Light->Type == Directional && desc.Light->DirectionToLight.Length() > 0`.

The first three checks are inside `PrepareDirectionalShadow`. The remaining are inside `DirectionalShadowPlanner::BuildPlan` (which already validates them — see the current implementation lines 219-249).

### `ShadowResourceCache::GetOrCreateDirectionalShadowResources` Checks

- `desc.StorageMode == Texture2DArray`.
- `desc.CascadeCount` in `[1, MaxShadowCascades]`.
- `desc.Resolution > 0`.
- `desc.DepthFormat == D32Float`.

If any check fails, log an error and return an empty `DirectionalShadowResources` (all `nullptr`). The manager treats this as "shadows disabled this frame" and `HasDirectionalShadow()` returns `false`.

### `RenderFrameResources` Checks

- `m_ShadowManager != nullptr` before calling `FillGPUShadowData`.
- `buffer->Update(...)` returns `true`; otherwise log an error (already done).

### `ShadowDepthPass` Checks

- `shadowManager != nullptr && shadowManager->HasDirectionalShadow()`.
- For each cascade, `depthView != nullptr` and `depthPipeline != nullptr`.
- `mesh != nullptr && mesh->VertexBuffer && mesh->IndexBuffer` before drawing.

### Layout Transition

The shadow texture array must transition from `DEPTH_ATTACHMENT_OPTIMAL` (after `ShadowDepthPass`) to `SHADER_READ_ONLY_OPTIMAL` (before `ForwardOpaquePass`). This is performed by `commandList->TransitionTextureToShaderRead(dir.SampledView)` at the end of the shadow pass. The current `VulkanCommandList::TransitionTextureToShaderRead` handles depth aspects; verify that the layout update tracks correctly across cascades (the `VulkanTexture::GetLayoutPtr` allows the command list to read and write the texture's current layout).

If the transitions are not inserted, the Vulkan validation layer reports `VUID-VkImageMemoryBarrier-oldLayout-01209` and the shadow sampling reads stale data.

## Verification

### Build

```bash
cd Build/default
cmake --build . --target XEngineEditor
cmake --build . --target XEngineSandbox
```

Both targets should compile and link cleanly. The current state of HEAD builds successfully (verified during the RHI cleanup pass).

### Runtime

1. Launch the editor.
2. Open a scene with a directional light, a few shadow-casting meshes, and a ground plane.
3. Verify that shadows appear in the viewport.

### RenderDoc

1. Capture a frame in RenderDoc.
2. Inspect the `ShadowDepthPass.C0`, `ShadowDepthPass.C1`, etc. passes. Each should:
   - Bind a per-layer depth view of the shadow texture array.
   - Set the viewport to `(0, 0, resolution, resolution)`.
   - Set the depth pipeline.
   - Issue draw calls.
3. Inspect the `ForwardOpaquePass`. The frame bind group should bind:
   - `binding 0`: `GPUFrameData` uniform buffer.
   - `binding 1`: shadow texture array (`Texture2DArray<float>`).
   - `binding 2`: shadow sampler.
4. Inspect the sampled texture array. Each layer should have shadow-cast depth data with values in `[0, 1]` (reverse-Z: 1.0 = near, 0.0 = far).
5. Verify that the cascades are visually distinct (different orientations of the sphere fit).

### Visual Tests

1. **No shadows** — disable `Enabled` in the directional light's `LightComponent`. The `ForwardOpaquePass` should not sample the shadow map (`ShadowParams.x < 0.5`).
2. **Cascade visualization** — enable `VisualizeCascades` in the debug panel. Surfaces should render in distinct colors per cascade.
3. **Freeze** — enable `FreezeShadowMatrices`, rotate the camera. The shadow projection should not move.
4. **Resolution change** — change `Resolution` from 2048 to 1024 in the settings. The shadow resources should be recreated. Verify with RenderDoc that the new resolution is used.
5. **Cascade count change** — change `CascadeCount` from 4 to 2. The shadow pass should render only 2 layers.
6. **Camera near/far** — set the camera's near plane to 0.5 and far to 100. The cascade splits should adjust.
7. **Light rotation** — rotate the directional light. The cascades should reorient.

### Vulkan Validation

With `EnableValidation = true` in `VulkanDeviceCreateInfo`, the Vulkan validation layer should report no errors. Common validation messages and their fixes:

- `VK_ERROR_IMAGE_USAGE` — shadow texture missing `Sampled` or `DepthStencilAttachment` usage.
- `VUID-VkWriteDescriptorSet-descriptorType-00333` — wrong binding type in `RHIBindGroupDesc`.
- `VUID-VkImageMemoryBarrier-oldLayout-01209` — missing layout transition between shadow pass and forward pass.
- `VK_SAMPLE_COUNT_1_BIT` mismatch — multisampling mismatch between attachment and pipeline.
- `VUID-VkPipelineRenderingCreateInfo-pColorAttachmentFormats-06589` — `HasColorAttachment` mismatch.

## Build Integration

The shader files must be added to the project's shader discovery. The exact mechanism depends on the project's shader build pipeline. For XEngine:

- Slang shaders are in `Engine/Shaders/`.
- The shader compiler is invoked during the C++ build by the `XEngineShader` module (see `Engine/Source/Runtime/Shader/`).
- New `.slang` files in `Engine/Shaders/Passes/`, `Engine/Shaders/Lighting/`, etc. are picked up automatically by the shader module's file scanner.

After adding the new `.slang` files, force a recompile of the shader module:

```bash
cmake --build Build/default --target XEngineShader
```

The compiled SPIR-V binaries are stored in `Build/Generated/Shaders/`. The `RenderShaderLibrary` loads them at runtime.

## What This Document Does Not Do

- It does not describe the visual design of the shadow map viewer (out of scope for V0).
- It does not describe performance tuning (PCF kernel size, cascade count trade-offs) — see [CSM_10](CSM_10_Implementation_Order_Checklist.md).
- It does not describe how to instrument the GPU side for profiling.

## Common Mistakes

1. **Saving the `FreezeShadowMatrices` toggle to `.xscene`.** It is a debug toggle, not a content setting. The `RendererDebugSettings` struct is in-memory only.

2. **Putting the `VisualizeCascades` mirror field at the top level of `RendererDebugSettings` in the wrong order.** The `Shadows.VisualizeCascades` is the canonical path; the top-level mirror is for editor UI convenience. Always read from `Shadows.VisualizeCascades` in the renderer.

3. **Forgetting to update `FrameResources::SetVisualizeCascades` on every frame.** The toggle is in `RendererDebugSettings`; the renderer must read it each frame and pass it down. The `RenderSystem` should do this in `Render()`.

4. **Compiling with `EnableValidation = false` and missing a layout transition.** The shadow pipeline would silently produce wrong shadows. Always run with validation enabled during Stage 9 development.

5. **Forgetting to flush the shader module after adding new `.slang` files.** A stale `XEngineShader.lib` would not include the new shaders, and `RenderShaderLibrary` would fail to find them at runtime. Force a clean rebuild:

   ```bash
   cmake --build Build/default --target XEngineShader --clean-first
   ```
