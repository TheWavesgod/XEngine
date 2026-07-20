# Code Comment Recommendations

This document lists places where additional inline code comments are worth
adding. Each entry names the file, the class or function, the rationale
("why this is non-obvious"), and what the comment should explain.

> Note: this is documentation **about** source comments, not the comments
> themselves. Apply this in a separate change after rebasing past V0
> freeze. Do not add any of these in the current docs-only pass.

## Severity: High

These are non-obvious behaviors where misreading the code leads to subtle
bugs or maintenance hazards.

### 1. `RenderSystem::Render` view/projection branching block

- **File**: `Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp:131-284`
- **Function**: `RenderSystem::Impl::Render(float deltaTime)`
- **Why**: The function carries seven distinct responsibilities
  (begin frame, extract, set up render output, build view/projection,
  shadow prep, frame resource upload, pipeline render, transitions,
  overlay, end frame). The view/projection block contains three implicit
  branches that produce different `RenderFrameContext` field subsets;
  dropping one field by accident silently uses a stale identity.
- **What the comment should explain**:
  - The three branches (ViewProvider, SceneSystem primary camera,
    `FallbackViewProjection`).
  - For each branch: which `RenderFrameContext` fields are filled
    and which remain at default.
  - That `ApplyRHIClipSpaceConvention` is the only place where Y-flip
    happens; the shader sees a Y-flipped projection already.
  - That this is a known refactor candidate (see
    `Docs/Architecture/02_Frame_Runtime_Flow.md` and the
    `RenderSystem` class doc).
- **What NOT to add**: a step-by-step narration of every assignment.

### 2. `RenderSystem::OnCreate` strict manager ordering

- **File**: `Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp:297-397`
- **Function**: `RenderSystem::OnCreate`
- **Why**: The seven `Initialize` calls have hidden dependencies:
  `Materials` needs `Textures`; `FrameResources` needs the shadow
  subsystem to be initialized; `PipelineStates` needs `Shaders +
  Materials + FrameResources`. Reordering fails silently in some cases.
- **What the comment should explain**:
  - The dependency order and why each ordering constraint exists.
  - That future code paths adding a new manager must respect this
    order.
- **What NOT to add**: per-line narration of what each call does.

### 3. `VulkanPipeline` push-constant size guard

- **File**: `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp`
- **Function**: `VulkanPipeline::VulkanPipeline(...)`
- **Why**: The guard exists because Intel Iris Xe Gen 11/12 (and
  similar drivers that match the Vulkan spec minimum
  `maxPushConstantsSize` = 128 bytes) crash inside
  `vkCreatePipelineLayout` instead of returning a Vulkan error.
  Without the guard, a future refactor that pushes >128 bytes would
  silently take down the engine.
- **What the comment should explain**:
  - That this is a known driver bug; not a portable defensive
    measure.
  - That the existing `ShadowDepthPushConstants` (144 bytes) triggers
    this guard; the recommended fix is to move `LightViewProjection`
    into a Set 0 UBO rather than shrinking the struct.
  - The interplay between this guard and the `m_Capabilities.MaxPushConstantSize`
    probe in `VulkanDevice::Initialize`.
- **What NOT to add**: the historical commit / bug-tracker id.

### 4. Shadow pipeline lifetime vs. cache identity

- **File**: `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp`
- **Function**: `ShadowResourceCache::GetOrCreateDirectionalShadowResources(...)`
- **Why**: The cache recreates the texture array + sampled view + per-
  layer depth views + sampler whenever shape parameters change. The
  `RenderFrameResources::SetShadowBindings` path consumes these as raw
  pointers; if any caller holds a stale pointer across a cache
  regeneration, the bind group is silently invalid.
- **What the comment should explain**:
  - Why rebuilding the cache implicitly invalidates existing
    bind groups.
  - That `RenderFrameResources::SetShadowBindings` is the single
    re-registration point and must be called after cache creation.
  - That the cache currently rebuilds in place rather than
    queueing+resolving on the next frame, which means the caller has
    to re-register on the same frame as the rebuild to avoid a one-
    frame stale bind.
- **What NOT to add**: a full Vulkan descriptor set primer.

### 5. `RenderFrameResources::SetShadowBindings` raw pointer contract

- **File**: `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp:95-112`
- **Function**: `RenderFrameResources::SetShadowBindings(...)`
- **Why**: Raw `RHITextureView*` / `RHISampler*` pointers are stored;
  the method short-circuits when the pair is unchanged but the
  semantics around "caller must guarantee lifetime" are not stated
  anywhere in code.
- **What the comment should explain**:
  - That the caller guarantees the resources outlive the renderer.
  - That the bind group is rebuilt only when the new pair differs
    from the previous pair, so calling with the same pointers is
    free.
  - That the `reinterpret_cast<std::uintptr_t>` debug print on the
    same lines is dead code and should be removed.
- **What NOT to add**: a description of the renderer / Vulkan
  relation.

### 6. `VulkanDevice::BeginFrame` synchronization semantics

- **File**: `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp:285-362`
- **Function**: `VulkanDevice::BeginFrame()`
- **Why**: The order of `vkWaitForFences`, `vkAcquireNextImageKHR`,
  `vkResetFences`, `vkResetCommandPool`, `vkBeginCommandBuffer` is
  load-bearing and it is not immediately obvious why the
  `vkResetCommandPool` resets the buffer (it is the only command
  buffer in the pool).
- **What the comment should explain**:
  - That the wait is per-frame-on-CPU and gates all submitted work.
  - That `image_available_semaphore` is single-instance and the
    render-finished semaphore is per-swapchain image.
  - That `vkResetCommandPool` is correct because the pool has a
    single `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`
    buffer.
  - The relationship to the in-flight fence (single instance,
    `MAX_FRAMES_IN_FLIGHT = 1`).
- **What NOT to add**: a step-by-step Vulkan tutorial.

### 7. `RHISystem::EnableValidation` default + override

- **File**: `Engine/Source/Runtime/RHI/Private/RHIDevice.cpp:62-79`
- **Function**: `RHISystem::OnCreate`
- **Why**: When no config is supplied, validation defaults to true;
  CI and headless mode will want it false. Today there is no env
  override (only `EngineConfig::EnableValidation`).
- **What the comment should explain**:
  - That the default is "true" for development safety.
  - That CI / headless targets should set `EngineConfig::EnableValidation = false`.
  - That there is no environment-variable override today (intentional).
- **What NOT to add**: anything about Vulkan validation layer
  mechanics.

### 8. `RenderExtraction::Extract` direction and ID conventions

- **File**: `Engine/Source/Runtime/Renderer/Private/Scene/RenderExtraction.cpp:30-116`
- **Function**: `RenderExtraction::Extract(...)`
- **Why**: Two non-obvious math conventions are used:
  - `DirectionToLight = Math::Normalize(-forward)` (the comment exists
    today but is buried in the implementation; lift it to the loop
    block).
  - `ObjectId = entity.Index + 1u` (reserves `0` as invalid).
- **What the comment should explain**:
  - That `+X` is light-forward per Project_Cache convention, and
    `DirectionToLight` is from surface to light.
  - That `0` is reserved to keep `IsValid()` checks cheap.
  - That empty `RenderScene` is the caller's responsibility to clear
    before calling.
- **What NOT to add**: full coordinate-system primer (see
  `Docs/Architecture/04_Coordinate_And_Matrix_Conventions.md`).

## Severity: Medium

These are time-savers where misreading the code costs maintenance time
but does not introduce bugs directly.

### 9. `RenderSystem::OutputProvider` semantics

- **File**: `Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp:160-180`
- **Function**: `Render` (the `OutputProvider` branch)
- **Why**: Off-screen render targets skip ClearPass and PresentPass;
  this is implicit in the `RenderToSwapchain == false` checks at the
  pass-adding boundary.
- **What the comment should explain**:
  - That when `OutputProvider` returns a non-swapchain view, the
    pipeline graph swaps around the off-screen color+depth target.
  - That the resulting `output.ColorTargetView` is transitioned to
    shader-read after the pipeline completes
    (`commandList->TransitionTextureToShaderRead`).
- **What NOT to add**: full explanation of dynamic rendering.

### 10. `RenderPipelineStateCache::GraphicsPipelineStateKey` floatBits quirk

- **File**: `Engine/Source/Runtime/Renderer/Private/Resources/GraphicsPipelineStateKey.h:54-57`
- **Function**: `GraphicsPipelineStateKeyHash::operator()`
- **Why**: The `floatBits(value == 0.0f ? 0.0f : value)` line is
  custom handling for `-0.0` vs `+0.0`. Without a comment it looks
  copy-pasted.
- **What the comment should explain**: 
  - That this normalizes `-0.0` and `+0.0` to the same hash so a
    pipeline that was created with `+0.0` is reused for `-0.0`.
- **What NOT to add**: full IEEE-754 explainer.

### 11. `ShaderSystem::m_Compiler` lazy creation

- **File**: `Engine/Source/Runtime/Shader/Private/ShaderSystem.cpp` (and
  `Engine/Source/Runtime/Shader/Public/XEngine/Shader/ShaderSystem.h`)
- **Function**: `ShaderSystem::OnCreate` or first `Compile` call
- **Why**: `m_Compiler` is constructed lazily. Readers may wonder
  why the optional `EnableShaderCompiler` flag disables the
  compiler rather than just disabling individual compile requests.
- **What the comment should explain**:
  - That without a compiler, `Compile` returns
    `ShaderCompileResult::CompilerUnavailable`.
  - That this lazy creation is in case a future stage wants to swap
    compilers at runtime.

### 12. `Scene::Scene` hierarchy traversal behavior

- **File**: `Engine/Source/Runtime/Scene/Public/XEngine/Scene/Scene.h` (and
  `Engine/Source/Runtime/Scene/Private/Scene.cpp`)
- **Functions**: `SetParent`, `IsDescendantOf`, `GetChildren`
- **Why**: The hierarchy is `parent -> children` plus `child ->
  parent`; `SetParent` must keep both maps in sync, including bumping
  transform-dirty flags. A future refactor that mutates only one of
  the two maps will silently break world transforms.
- **What the comment should explain**:
  - That both maps must be updated together.
  - That `SetParent` rejects cycles.
  - That descendant transforms are invalidated on reparenting.

### 13. `RHIResourceFactory` validation path

- **File**: `Engine/Source/Runtime/RHI/Private/RHIResourceFactory.cpp:39-299`
- **Why**: Each `Create*` call runs descriptor validation before the
  backend hook. The validation rules (count == 1, type matches, device
  ownership) are not obvious from a quick read.
- **What the comment should explain**:
  - That `count` is currently forced to 1 (descriptor arrays
    rejected).
  - That all referenced resources must share the same device.
  - That buffer offset + range are bounded by buffer size.

### 14. `VulkanCheckedCast` debug-vs-release behavior

- **File**: `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCheckedCast.h`
- **Function**: `CheckedVulkanCast<T>(...)`
- **Why**: The helper asserts device ownership in debug and silently
  static-casts in release. Misreading this as "always validates"
  leads to hard-to-find bugs.
- **What the comment should explain**:
  - That the assertion is debug-only; release builds elide it.
  - That this replaces `dynamic_cast<VulkanX*>(rhiX*)`.

### 15. `VulkanSwapchain::Recreate` semaphore-array lifetime

- **File**: `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp:821-867`
- **Function**: `VulkanDevice::RecreateSwapchain`
- **Why**: `VulkanFrameResources` are recreated only when the
  render-finished-sempahore count differs from the new swapchain
  image count. If the count matches, the FencesResource array is
  reused even though the swapchain was rebuilt.
- **What the comment should explain**:
  - That a `vkDeviceWaitIdle` is the only sync barrier before the
    rebuild.
  - That the render-finished semaphores are reused across
    recreations when image count matches.

### 16. `Engine::Initialize` subsystem creation order

- **File**: `Engine/Source/Runtime/Engine/Private/Engine.cpp:48-71`
- **Function**: `Engine::Initialize`
- **Why**: The fixed subsystem order has hidden dependencies (e.g.
  `RHISystem` needs `PlatformSystem` for the native window).
- **What the comment should explain**:
  - That the order is significant.
  - That adding a subsystem must come at the correct point in this
    list.

## Severity: Low

Documentation / discoverability improvements; not load-bearing.

### 17. `ShadowDepthPass::Execute` per-cascade transition

- **File**: `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp`
- **Function**: `AddShadowDepthPass` (the per-cascade lambda)
- **Why**: Each cascade ends with
  `commandList->TransitionTextureToShaderRead(depthView)`; without
  this the forward pass's shadow sampling reads from an unexpected
  layout and the validation layer complains. Today this is documented
  inline in the `.cpp`; the contract is easy to lose in a future
  refactor.
- **What the comment should explain**:
  - That the transition is required for dynamic rendering.
  - That the next pass' sampler is `SHADER_READ_ONLY_OPTIMAL`.

### 18. `ForwardOpaquePass::Execute` set-output rebuild

- **File**: `Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp`
- **Function**: `AddForwardOpaquePass` (the execute lambda)
- **Why**: The pass resets the render output to
  `frameContext.Output` after `ShadowDepthPass` switched it to
  per-cascade depth-only views.
- **What the comment should explain**:
  - That this restore is mandatory for dynamic rendering.
  - That it always reads `frameContext.Output` from the previous
    pass' setup, never its own recorded output.

### 19. `Math/Pipeline/RenderProjection.h`

- **File**: `Engine/Source/Runtime/Renderer/Private/Pipeline/RenderProjection.h`
- **Function**: `ApplyRHIClipSpaceConvention`
- **Why**: The Y-flip is the single point where the engine handles
  Vulkan's flipped-Y clip-space. Callers may be tempted to "fix Y
  themselves" and double-flip.
- **What the comment should explain**:
  - That callers must NOT also flip Y on their own.
  - That this is called once per frame, not per object.

### 20. `Math::BuildViewMatrixLH_XForward` rationale

- **File**: `Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h`
- **Why**: The function name encodes three conventions
  (left-handed, X-forward, specific orientation). New helpers often
  copy the name without understanding which knob to flip.
- **What the comment should explain**:
  - That the orientation matches the world convention.
  - That this is the only view-matrix helper the Renderer should call.

## Items Explicitly Out of Scope

These are tempting but should NOT receive comments:

- Plain getters / setters (`GetPosition`, `SetColor`).
- Loops over collections with single-statement bodies.
- Trivial constructor/destructor bodies.
- `enum class` declarations where the names are self-documenting.
- Macro definitions whose name already carries the meaning.

## Final Notes

This file is documentation only. Apply these comments in a separate
change after the docs-only phase completes. Touch the comments last:
- the code's behavior is the source of truth.
- comments explain *why*, not *what*.
- older commit messages may provide historical context, but commit
  messages must not be cited inside source code.

The set of items here is intentionally short. Many places in the
codebase are already well-commented; this file focuses on the spots where
clarity remains a barrier to safe change.
