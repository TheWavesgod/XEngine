# Renderer

## 1. Module Purpose

Renderer is the engine's render-side subsystem. It owns RenderSystem, the
forward pipeline implementation (ForwardRenderPipeline), all renderer
managers (texture/mesh/material/shader/pipeline cache/frame resources/
shadow), the shadow subsystem, the scene -> render-scene bridge, and the
RenderGraph runtime (currently V0).

Renderer reads Scene and Asset data and consumes the public RHI surface.
It does not include RHI backend code; Vulkan types stay in `RHI/Private/Vulkan/`.

## 2. Responsibilities

- Build the active Scene's RenderScene every frame (`RenderExtraction`).
- Build the View / Projection for the frame.
- Prepare and use the cascade shadow map (RenderShadowManager).
- Update per-frame GPU data and bind it through the Set-0 bind groups
  (`RenderFrameResources`).
- Cache graphics pipelines by `GraphicsPipelineStateKey` (`RenderPipelineStateCache`).
- Run the per-frame `ForwardRenderPipeline` and emit Vulkan commands
  through RHI.
- Provide optional `OverlayCallback` / `ViewProvider` / `OutputProvider`
  integration points for the editor.

## 3. Non-Responsibilities

- Does not depend on Renderer-only types from outside the renderer module.
- Does not own the GPU device itself; it borrows `RHIDevice` via
  `RHISystem`.
- Does not implement Vulkan directly.
- Does not own Scene or Asset records.

## 4. Public API Surface

`Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/`:

- `RenderSystem.h` - the subsystem entry point.
- `RenderView.h`, `RenderScene.h`, `RenderTypes.h`.
- `Material.h`, `MaterialTypes.h`.
- `Mesh.h`.
- `Texture.h`.
- `CameraData.h`.
- `RendererSettings.h`, `RendererDebugSettings.h`.

Private:

- `Private/RenderSystem.{h,cpp}` - subsystem implementation.
- `Private/Pipeline/` - `RenderPipeline.{h,cpp}`, `ForwardRenderPipeline`,
  `RenderFrameContext`, `RenderProjection`.
- `Private/Passes/` - `ClearPass`, `ForwardOpaquePass`, `PresentPass`,
  `ShadowDepthPass`, plus unreferenced stubs.
- `Private/Resources/` - the 7 persistent managers plus their
  graphics-pipeline-key and shader-key types.
- `Private/Shadows/` - `RenderShadowManager`, `DirectionalShadowPlanner`,
  `ShadowResourceCache`, `RenderShadowType`.
- `Private/Scene/RenderExtraction.{h,cpp}`.
- `Private/RenderGraph/` - V0 linear runtime + dead stubs (compiler,
  executor, resource).
- `Private/ShaderInterop/` - `GPUFrameTypes.h`, `GPULightingTypes.h`,
  `GPUShadowTypes.h`.
- `Private/Materials/`, `Private/Mesh/`, `Private/GPUScene/` - extra
  helper namespaces (currently partial).

## 5. Dependencies

### Depends on

- `XEngineFoundation` (PUBLIC).
- `XEngineCoreRuntime` (PUBLIC).
- `XEngineShader` (PUBLIC).
- `XEngineRHI` (PUBLIC).
- `XEngineAsset` (PRIVATE).
- `XEngineScene` (PRIVATE).

### Used by

- `Engine/Source/Editor` (PUBLIC).
- `Apps/Sandbox`, `Apps/EditorApp` (PUBLIC).

## 6. Ownership and Lifetime

- `RenderSystem` owns exactly one instance of each of the seven persistent
  managers (declared as `std::unique_ptr` in
  `Renderer/Private/RenderSystem.cpp:46-58`).
- `RenderSystem` also owns the active `RenderPipeline` (currently always
  `ForwardRenderPipeline`).
- `RenderResourceContext` is populated inside `RenderSystem::OnCreate` to
  expose the seven pointers to passes.
- Per-frame ownership: `RenderScene` (member of `RenderSystem::Impl`),
  `RenderFrameContext` (local to `Render`), per-pass transient data
  (created inside the pass execution).

## 7. Runtime Flow

The per-frame order inside `RenderSystem::Render(float)` is fixed; see
`Docs/Architecture/02_Frame_Runtime_Flow.md` for the sequence diagram.

In summary:

1. `RHIDevice::BeginFrame` -> active command list.
2. `RenderExtraction::Extract(scene, assetSystem, ctx, sceneData)`.
3. Build `RenderFrameContext` (view/projection/camera world position/time).
4. `ShadowManager.PrepareFrame`.
5. `FrameResources::SetShadowBindings` if cache regenerated.
6. `FrameResources::Update(frame, sceneData, shadowManager)`.
7. `ActivePipeline->Render(frame, sceneData, ctx)` -> `ForwardRenderPipeline`
   runs the graph:
   - Clear (swapchain only)
   - ShadowDepth (per cascade)
   - ForwardOpaque
   - Present (swapchain only, no-op)
8. Off-screen render target -> `TransitionTextureToShaderRead`.
9. `OverlayCallback` (editor's Vulkan overlay path).
10. `RHIDevice::EndFrame` -> present.

## 8. Important Invariants

- `RenderScene` is rebuilt every frame; do not cache `RenderObject`s across
  frames.
- The shadow cache is shared across frames; `SetShadowBindings` only
  rebuilds when shape changes.
- All materials and meshes are looked up through Handles; the same
  `MeshHandle` in successive frames must refer to the same `RHITexture`
  / `RHIBuffer` so the runtime stays consistent.
- The 0-th directional light in `GPULightingData` is the only one that
  participates in the ForwardPBR shadow factor computation today
  (`ForwardPBR.slang`).

## 9. Main Classes and Collaborators

- `RenderSystem`, `ForwardRenderPipeline`.
- `RenderScene`, `RenderObject`, `RenderLight`.
- `RenderFrameContext`, `RenderResourceContext`.
- `RenderFrameResources`, `RenderTextureManager`, `RenderMeshManager`,
  `RenderMaterialSystem`, `RenderShaderLibrary`,
  `RenderPipelineStateCache`, `RenderShadowManager`.
- `RenderExtraction`.
- `DirectionalShadowPlanner`, `ShadowResourceCache`, `RenderShadowType`.
- `RenderGraph` (V0), `RenderGraphPass`, `RenderGraphContext`,
  `RenderGraphBuilder`.

## 10. Design Rationale

- A single `RenderSystem` subsystem keeps lifecycle simple: subsystem
  registration handles ordering and shutdown.
- `RenderResourceContext` collapses many manager pointers into one
  parameter for passes - cleaner signatures and a single point of
  change when adding a new manager.
- Passes are pure functions of `(RenderGraph&, frame, scene, ctx)`; this
  makes future RenderGraph V1 work straightforward.

### Alternatives considered

- A separate command-recorder subsystem feeding the renderer. Deferred to
  the RenderGraph V1 path.
- Per-frame allocator of RenderObjects. Rejected: the count is bounded
  for V0 and clearing per frame is fine.

### Trade-offs

- View / Projection building lives inside `RenderSystem::Render`,
  spilling into a 60+ line branching block. See
  `Docs/Architecture/03_Ownership_And_Lifetime.md` and the class doc for
  `RenderSystem`.
- Shadows are a single directional cascade; other light types are
  buffer-packed but not visually shaded yet.

## 11. Failure Modes and Debugging

- Pipeline creation failure inside `VulkanPipeline` is logged via
  `XENGINE_LOG_ERROR`; `RenderPipelineStateCache` returns `nullptr`, the
  pass's `if (pipeline == nullptr) continue;` gracefully skips the
  affected object.
- Shadow cache regeneration mid-frame is supported but during the same
  frame the new resources must already be in the device pool.
- A `RenderFrameResources::SetShadowBindings` call with non-matching
  pointers short-circuits (no rebuild).

## 12. Current Limitations

- One-frame-in-flight + ring-of-3 in renderer mismatch documented at
  `Docs/Architecture/07_Frames_In_Flight_And_GPU_Synchronization.md`.
- Five pass files are dead stubs; two more (`ForwardMeshPass`,
  `TrianglePass`) are full implementations that are not wired.
- No async compute.
- No culling: every `RenderObject` is drawn unless explicitly invisible.

## 13. Source References

- `Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/*`
- `Engine/Source/Runtime/Renderer/Private/RenderSystem.{h,cpp}`
- `Engine/Source/Runtime/Renderer/Private/Pipeline/`
- `Engine/Source/Runtime/Renderer/Private/Passes/`
- `Engine/Source/Runtime/Renderer/Private/Resources/`
- `Engine/Source/Runtime/Renderer/Private/Shadows/`
- `Engine/Source/Runtime/Renderer/Private/RenderGraph/`
- `Engine/Source/Runtime/Renderer/Private/Scene/`
- `Engine/Source/Runtime/Renderer/Private/ShaderInterop/`

## 14. Future Work

- RenderGraph V1 (topological analysis, resource aliasing, barrier
  generation).
- Bindless resources / descriptor arena (the current per-material
  bind groups do not scale).
- Real view / projection extraction (see `02_Frame_Runtime_Flow.md`
  discussion).
- Async compute queue, deferred lighting, TAA, IBL.
