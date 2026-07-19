# 04 Renderer Architecture 审计

## 1. 审计范围

- `Runtime/Renderer/Private/RenderSystem.cpp/.h`
- `Runtime/Renderer/Private/Pipeline/`：`ForwardRenderPipeline.{h,cpp}`、`RenderPipeline.{h,cpp}`、`RenderFrameContext.h`、`RenderProjection.h`
- `Runtime/Renderer/Private/Resources/`：`RenderFrameResources.{h,cpp}`、`RenderMeshManager.{h,cpp}`、`RenderMaterialSystem.{h,cpp}`、`RenderTextureManager.{h,cpp}`、`RenderShaderLibrary.{h,cpp}`、`RenderPipelineStateCache.{h,cpp}`、`RenderResourceContext.h`、`RenderResourceManager.{h,cpp}`、`MeshManager.{h,cpp}`、`GraphicsPipelineStateKey.h`、`RenderShaderKey.h`、`RenderShaderTypes.h`、`BindlessResourceManager.{h,cpp}`、`RenderTextureManager.{h,cpp}`
- `Runtime/Renderer/Private/RenderGraph/`：`RenderGraph.{h,cpp}`、`RenderGraphBuilder.{h,cpp}`、`RenderGraphCompiler.{h,cpp}`、`RenderGraphExecutor.{h,cpp}`、`RenderGraphContext.{h,cpp}`、`RenderGraphPass.h`、`RenderGraphResource.{h,cpp}`
- `Runtime/Renderer/Private/Passes/`：`ShadowDepthPass.{h,cpp}`、`ForwardOpaquePass.{h,cpp}`、`ForwardPass.{h,cpp}`、`ForwardMeshPass.{h,cpp}`、`DepthPrePass.{h,cpp}`、`ClearPass.{h,cpp}`、`PresentPass.{h,cpp}`、`SkyboxPass.{h,cpp}`、`TonemapPass.{h,cpp}`、`TrianglePass.{h,cpp}`
- `Runtime/Renderer/Private/Scene/`：`RenderExtraction.{h,cpp}`、`RenderScene.cpp`
- `Runtime/Renderer/Private/Shadows/`：`RenderShadowManager.{h,cpp}`、`ShadowResourceCache.{h,cpp}`、`DirectionalShadowPlanner.{h,cpp}`、`RenderShadowType.h`
- `Runtime/Renderer/Private/Materials/`、`Private/Mesh/`、`Private/ShaderInterop/`、`Private/GPUScene/`
- `Runtime/Renderer/Public/XEngine/Renderer/`：`RenderTypes.h`、`RenderScene.h`、`RenderView.h`、`MaterialTypes.h`、`Material.h`、`Mesh.h`、`Texture.h`、`CameraData.h`、`RendererSettings.h`、`RendererDebugSettings.h`、`RenderSystem.h`

---

## 2. 当前优点

- **RHI 抽象清晰**：`Renderer` 不直接 include Vulkan 头，仅消费 `RHI/*`。CSM 与 Set 0 数据通过 `RHIBindGroup` 流入 Pipeline，没有与 backend 耦合。
- **Pass/Resource/Frame 三层划分**：Passes 仅 record command；RenderFrameResources 仅管理 per-frame buffer / bind group；ShadowResourceCache 拥有 shadow resources。**期望方向正确**。
- **FrameContext 单一载体**：CPU 端 `RenderFrameContext` 是一帧逻辑的 transport object（Device, CommandList, ViewMatrix, Projection, Viewport, FrameIndex, CameraWorldPosition, Output）。这有助于测试和 debug 帧记录。
- **RenderPipeline 是抽象层**：`ForwardRenderPipeline` 继承 `RenderPipeline`，将来 Defer / Compute 路径接入只需新增子类。
- **PipelineStateCache 收敛**：`RenderPipelineStateCache` 用 `GraphicsPipelineStateKey` 做 hash，pipeline 不会重复创建。
- **Manager / Cache split**：`RenderTextureManager` 与 `RenderShaderLibrary` 都是独立模块，初始化由 RenderSystem 编排，运行时各管一摊。
- **ShadowResourceCache 单套纹理持久化**：与提示文档期望方向完全吻合。
- **`RenderFrameResources` 不创建 shadow maps，仅消费 shadow bind group 的 raw pointer**——这正好是 "Set 0 bind groups 包含 shadow sampled view / sampler，但不创建 shadow textures"。
- **GPU struct layout 严格 fixed**：`GPUFrameData`、`GPULightingData`、`GPUShadowData`、`GPUCascadeShadowData`、`PBRPushConstants`、`ShadowDepthPushConstants` 都用 `alignas(16)` + `static_assert(sizeof)`，对 Slang 端保持 layout。
- **ImGui/CSM 调试**：`RendererDebugPanel` 接通 `RendererDebugSettings`，并预先留 slot 给未来 CSM 调试工具。

---

## 3. 发现的问题

### 3.1 [Critical] `RenderFrameResources` 直接引用未定义变量 `shadowSampledView`、`shadowSampler`

- **相关文件**：[`RenderFrameResources.cpp:80-86`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：
  - `RenderFrameResources::Initialize` 在创建 bind group 时，把 `RHIBindingResource { 1, ..., shadowSampledView, nullptr, nullptr, 0, 0 }` 传进去 —— `shadowSampledView`、`shadowSampler` 这两个名字在函数作用域内并未声明。任何使用 Set 0 的 shader（如 ShadowSampling）都会拿到一个 null sampled view -> 着色失败。
  - 当前能编是因为某些调用路径没用 `frameBindGroup`（仍走 `RenderFrameResources::Update`），但 shader 端要 `g_ShadowMap.Sample(...)` 就会 segfault / 静默。
- **为什么重要**：CSM Set 1 / binding 2 是 ShadowMap 的核心契约，目前实现是 broken。
- **推荐修复方式**：
  - 把 Initialize 多接两个参数：`RHITextureView* shadowSampledView, RHISampler* shadowSampler`；或
  - 提供 `SetShadowBindings(...)`，由 `RenderSystem` 在 `RenderShadowManager` 初始化后调用；或
  - 把 `m_FrameBindGroups` 在 `Initialize` 创建时不放 shadow binding，而是在第一次 `Update` 拿到 shadow resource 后再 write descriptor（`RHIBindGroup::Update`）。
- **建议**：P0，立刻修。

### 3.2 [Critical] `RenderFrameResources::Initialize` 创建 bind group 时 `shadowSampledView` / `shadowSampler` 未提供，但 Shader 端要读取 ShadowMap

- **相关文件**：[`ForwardOpaquePass.cpp:104-107`](../../Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp)
- **问题描述**：`ForwardOpaquePass` 通过 `SetBindGroup(0, frameBindGroup)` 把 Set 0 绑到 GPU；如果 `frameBindGroup` 内的 shadow texture/sampler 为 null，shader 在 `ShadowSampling.slang:62` 会触发 GPU 异常。
- **为什么重要**：这是 shadow 必须能"动起来"的核心路径，目前 broken。
- **推荐修复方式**：与 3.1 配套修。

### 3.3 [Critical] `RenderShadowManager::PrepareFrame` 第 124 行空指针赋值

- **相关文件**：[`RenderShadowManager.cpp:124`](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp)
- **问题描述**：
  ```cpp
  if (shadowLight = nullptr) { return; }
  ```
  这是 assignment，不是比较！永远 return false，跳出函数。这意味着 **当前即使有 directional shadow light，也永远不进入规划器**。CSM 实际未生效。
- **推荐修复方式**：改为 `if (shadowLight == nullptr) { return; }`，外加 `RendererDebugPanel` 调试输出确认。
- **建议**：P0，立刻修。这是审计范围内最容易修复且效益最大的 silent bug。

### 3.4 [Critical] `ShadowDepthPass.cpp` 写入 command 时只用 `RHIPipeline* depthPipeline` 缺 fallback

- **相关文件**：[`ShadowDepthPass.cpp:46-53`](../../Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp)
- **问题描述**：
  - `GetOrCreateShadowDepthPipeline(RHIFormat::Undefined, RHIFormat::D32Float)` 在 `RenderPipelineStateCache` 中按当前签名只匹配 ForwardOpaque pipeline（见 CreateGraphicsPipeline 中 `key.PassKind != RenderPassKind::ForwardOpaque` 直接 return null）。
  - shadow depth 是单独的 PassKind，应当有专用 PassKind，但当前 `GraphicsPipelineStateKey` 与 `RenderPipelineStateCache::CreateGraphicsPipeline` 都不接受它。
- **为什么重要**：ShadowDepthPass 即使修完 3.3 也不会真正画到 shadow map，因为 pipeline 永远 null。
- **推荐修复方式**：
  - 引入 `RenderPassKind::ShadowDepth` 与对应的阴影 depth pipeline 构造（独立 RenderShaderKey 路径：`Passes/DepthOnly.slang`，vertex function 为 `vertexMain`，fragment 不需要）；
  - `RenderPipelineStateCache::CreateGraphicsPipeline` 接受该 PassKind 并写专门 desc（depth-only：EnableDepthTest=true, EnableDepthWrite=true, HasColorAttachment=false）；
  - 在 `ShadowDepthPass.cpp` 中改成读 `RenderPassKind::ShadowDepth` pipeline key；
  - `depthPipeline->DepthBias` 写入 cascade 的 depth bias（已经准备好 `EnableDepthBias = true` 与 `DepthBiasConstantFactor`）。
- **建议**：P0-1，立刻修。

### 3.5 [Critical] `Resources.FrameResources->Update(frame, SceneData)` 在每帧重建 shadow bind group 时 race

- **相关文件**：[`RenderSystem.cpp:252-256`](../../Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp)、[`RenderFrameResources.cpp:119-132`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：
  - `Update` 只用 `BuildGPUFrameData` 填充 camera/lighting，没有把当前 frame 的 `ShadowData` 填进去。CSM 的 `LightViewProjection` 等 GPU 数据根本就没有传到 shadow bind group / GPUFrameData 的 `Shadows` 字段。
  - 即便 shader 端把 `GPUFrameData.Shadows` 绑定在了哪个 buffer 上，C++ 端 `BuildGPUFrameData` 也没 `data.Shadows = ...`。这是个空缺。
- **为什么重要**：CSM 的 GPU 部分数据全是 0 矩阵，shader 拿到的 `LightViewProjection` 是空矩阵。CSM 整体 broken。
- **推荐修复方式**：
  - `BuildGPUFrameData` 增加参数 `const RenderShadowManager& shadowManager`，并 fill `data.Shadows = ...` 通过 `shadowManager.FillGPUShadowData(...)`；
  - `RenderSystem::Render` 调用前已经把 ShadowManager 准备完，传递过去即可。
- **建议**：P0-1。

### 3.6 [Critical] `DirectionalShadowPlanner::BuildPlan` 在 `ReverseZ=true` 的分支里调用两次同名 `Math::OrthographicLH_ZO`

- **相关文件**：[`DirectionalShadowPlanner.cpp:311-323`](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp)
- **问题描述**：
  - `tmpProj = Math::OrthographicLH_ZO(..., -radius - bias*4, radius + bias*4)` 然后立刻又覆盖。
  - 同时 `lightProjection` 在 line 339-353 再写一次相同内容。看上去是为了让深度测试 reverse-Z，但代码可读性差且容易写错。
  - 在 `desc.ReverseZ=true` 时，传 `(radius+bias*4, -radius-bias*4)` 顺序即可；目前做了 swap 但随后又赋值覆盖。
- **为什么重要**：
  - Reverse-Z 是 shadow 比较通过 `>=` 的关键约定。一旦代码逻辑分层出错，shadow filter 表现是"全亮"或"全暗"。
  - 当前写入 depth buffer 的范围不确定时，比较必然错。
- **推荐修复方式**：抽出 helper `ComputeCascadeProjection(radius, bias, reverseZ)`，让 reverse-Z 与正向 Z 在一处。
- **建议**：本批 CSM 修（P0-1）。

### 3.7 [Critical] `ForwardOpaquePass` 在 Shadow S0 资源未绑定前调用 `SetBindGroup(0, frameBindGroup)`

- **相关文件**：[`ForwardOpaquePass.cpp:104-107`](../../Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp)
- **问题描述**：见 3.2。这是一旦 `m_FrameBindGroups` 内 shadow texture/sampler 被正确注入，必须确保 shadow manager 在 ForwardOpaquePass 之前完成。
- **推荐修复方式**：在 `RenderSystem::Render` 中：
  ```cpp
  ShadowManager->PrepareFrame(...)
  FrameResources->Update(frame, SceneData, *ShadowManager);
  ```
- **建议**：P0-1 配套。

### 3.8 [High] `RendererMaxFramesInFlight = 3` 与 RHI 的帧数应同步

- **相关文件**：[`RenderFrameResources.h:19`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.h)
- **问题描述**：
  - RHI/Renderer 双方独立硬编码帧数；未来 backend 改 1 或 2 帧都会和 Renderer 错位。
  - 已在 RHI 公共层审计 3.2 提过。
- **推荐修复方式**：从 `RHIDevice::GetMaxFramesInFlight()` 拿。

### 3.9 [High] `RenderFrameResources::Initialize` 在某些 error path 上 Shutdown 半成品

- **相关文件**：[`RenderFrameResources.cpp:42-97`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：
  - 如果 `CreateBindGroup` 在第 `n` 个 frame-in-flight 失败，函数 `return false`，但已经创建的 `m_FrameBuffers[0..n)` 与 `m_FrameBindGroupLayout` 都仍在。调用方接着 `Shutdown()` 没问题，但 `Shutdown()` 没检查 `m_FrameBindGroups[i]` 是否为 null。
  - 这是 safe 的（`reset()` 对 null 是好的）；但显式 check + log 更友好。
- **推荐修复方式**：保持现状即可，**但加注释：每个 `m_FrameBuffers[i]` 与 `m_FrameBindGroups[i]` 是可选**。

### 3.10 [High] `ForwardRenderPipeline::Render` 是 God object 雏形

- **相关文件**：[`ForwardRenderPipeline.cpp/.h`](../../Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp)
- **问题描述**：
  - 当前已经接管：ShadowDepthPass + ForwardOpaquePass + 其他可选 pass；
  - 没有 RenderGraph 显式的 builder API，已是直接 AddPass；
  - `ActivePipeline->Render(frame, SceneData, Resources)` 这个调用承担"调度 → 录制 → 提交"全部责任。
  - 未来 Defer / Compute / Async 路径接入时，`ForwardRenderPipeline` 会膨胀。建议把 RenderGraph API 提为 RenderPipeline 公共部分，`ForwardRenderPipeline` 只剩 setup+compile。
- **推荐修复方式**：
  - RenderGraph 设计前先在 `RenderPipeline` base 留出 `Build(RenderResourceContext&)` virtual；各 pipeline 子类负责构建 graph；
  - base class 负责 `graph.Compile()` 与 `Execute(RenderGraphContext)`；
  - 这样 CSM、ForwardOpaque 等成为 graph 的 content。
- **建议**：RenderGraph V1 前修。

### 3.11 [High] `GPUScene` 已搭框架但未消费

- **相关文件**：[`Renderer/Private/GPUScene/`](../../Engine/Source/Runtime/Renderer/Private/GPUScene)
- **问题描述**：当前看到 `GPUScene.{h,cpp}` 与 `GPUSceneUploader.{h,cpp}` 已存在，但是 ForwardOpaquePass 仍走 CPU `RenderScene` 索引，没有真正消费 SSBO / GPU-driven path。
- **推荐修复方式**：保持，标 TODO Stage 13。

### 3.12 [High] `RenderFrameResources::Update` 不重置 shadow 数据

- **相关文件**：[`RenderFrameResources.cpp:119-132`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：
  - `BuildGPUFrameData` 应当包括 ShadowData；
  - 当前 `data` 在 `BuildGPUFrameData` 中只填 `Camera` 与 `Lighting`，`data.Shadows` 保持零值。
- **推荐修复方式**：在 `BuildGPUFrameData` 中加入 `shadowManager.FillGPUShadowData(data.Shadows)` 调用。

### 3.13 [High] `RenderShadowManager::FreezeShadowMatrices` 行为分裂

- **相关文件**：[`RenderShadowManager.cpp:42-67`](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp)
- **问题描述**：
  - 当 FreezeShadowMatrices=true 时，第一次进入 PrepareFrame 会用 `m_HasFrozenData` 标记并备份到 `m_FrozenFrameData`；之后保持冻结。
  - 但**`m_FrozenFrameData.Directional.ShadowTexture / SampledView / Sampler / CascadeDepthViews`** 这些 GPU 资源都是 raw 指针，如果 shadow resource 在冻结期间被重建（用户改分辨率），指针会悬挂。
- **推荐修复方式**：
  - 把冻结数据改为 handle / weak-ptr；
  - 或在 ShadowResourceCache `GetOrCreate...` 中：
    ```cpp
    if (m_Directional.Texture was reset due to recreate) -> invalidate all shadow bind groups. 
    ```
- **建议**：ShadowResourceCache 重建修复时一并处理。

### 3.14 [High] `DirectionalShadowPlanner::BuildPlan` 在 `desc.ReverseZ = true` 时深度范围混乱

- **相关文件**：[`DirectionalShadowPlanner.cpp:308-353`](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp)
- **问题描述**：
  - 见 3.6 详细注释。代码末尾生成的 `lightProjection` 实际传入的是 `(radius+bias*4, -radius-bias*4)` 的 near/far；这与 `Math::OrthographicLH_ZO(near, far)` 的语义存在不一致，需要逐行核对。
- **推荐修复方式**：单步替换为 helper，确认测试样例可解释。

### 3.15 [High] `ShadowResourceCache::GetOrCreateDirectionalShadowResources` 在重建路径上 invalidates frame bind group，但 frame bind group 仍引用旧 view

- **相关文件**：[`ShadowResourceCache.cpp:58-90`](../../Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp)
- **问题描述**：
  - `m_Directional = {}` 直接释放旧 `Texture` / `SampledView` / `LayerDepthViews`，但 `RenderFrameResources::m_FrameBindGroups` 内 Set 0 的 binding 1（SampledTexture）继续引用旧 SampledView。
  - shadow resource 重建后，shadow_map 采样会采样已经被释放的 view。这对 vulkan 来说是 dangling descriptor set。
- **推荐修复方式**：
  - 在 `RenderFrameResources` 持有 `RHITextureView*` 改为 `std::shared_ptr<RHITextureView>`，且 shadow cache 重建时通知 frameResources；
  - 或 `RenderFrameResources` 与 Shadow 共享一个"shadow resource version"计数，每帧 check，若变化则 rebind。
- **建议**：P0-1。

### 3.16 [Medium] `DirectionalShadowPlanDesc` 默认 `Resolution = 2048`, `DepthBias = 0.003f` 是 "magic default"

- **相关文件**：[`DirectionalShadowPlanner.h`](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.h)、[`RendererSettings.h`](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererSettings.h)
- **问题描述**：shadow 资源 cache 与 frame data 一致性主要靠这两个 magic；最好改成 `DirectionalShadowSettings` 上读 `settings.Resolution` / `settings.DepthBias`（事实上 PrepareDirectionalShadow 已经在做），但 planner 内部 default 不应再用。
- **推荐修复方式**：保持 default 在 settings。

### 3.17 [Medium] Pass 接口签名暴露 `RHIPipeline*` 作为 arg 是不一致的

- **相关文件**：[`ShadowDepthPass.cpp:46-50`](../../Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp)、[`ForwardOpaquePass.cpp:94`](../../Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp)
- **问题描述**：每个 pass 通过 `PipelineStates->GetOrCreate*(...)` 取 pipeline，再传给 lambda；统一改走 `RenderPipelineStateCache::GetOrCreate*` 是好的；但重复模式应该抽象成 helper：`auto depthPipeline = resources.PipelineStates->GetOrCreateShadowDepthPipeline(...)` 并 error-out。
- **推荐修复方式**：加 `PipelineView` wrapper，或者使用 `RHIPipeline::IsValid()` 检查。

### 3.18 [Medium] `RenderFrameContext` 字段过宽

- **相关文件**：[`RenderFrameContext.h`](../../Engine/Source/Runtime/Renderer/Private/Pipeline/RenderFrameContext.h)
- **问题描述**：包含 `Device`、`CommandList`、`ViewMatrix`、`ProjectionMatrix`、`ViewProjectionMatrix`、`CameraWorldPosition`、`Output`、`FrameIndex`、`TimeSeconds`、`DeltaTime`、`SwapchainWidth`、`SwapchainHeight`。未来要把 `RenderShadowManager::RenderShadowFrameData shadowSnapshot` 注入，可一次性 compose。
- **推荐修复方式**：保持现状，但加 `FrameSnapshot` 字段为以后用。

### 3.19 [Medium] `ShadowResourceCache::Initialize` 与 `Shutdown` 不对称

- **相关文件**：[`ShadowResourceCache.cpp:15-26`](../../Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp)
- **问题描述**：`Initialize` 只是 store device；`Shutdown` 直接清空。`m_Device` 没置 null，使 re-init 不干净。
- **推荐修复方式**：`Shutdown` 中把 `m_Device = nullptr;`。

### 3.20 [Medium] `RenderFrameResources` 没有 destroy Vulkan 后 writeable cursor 的"rollback"

- **相关文件**：[`RenderFrameResources.cpp:100-117`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：`Shutdown` 没按 1..N 反序释放；但 shared_ptr 会自动处理，可忽略。

### 3.21 [Medium] Renderer 公共 header 暴露私有信息

- **相关文件**：[`Runtime/Renderer/Public/XEngine/Renderer/Mesh.h`](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/Mesh.h)、[`Material.h`](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/Material.h)
- **问题描述**：需要扫描这两个 file（未读完）以确认不暴露 RHI / Vulkan / Slang / stb。
- **推荐修复方式**：先验证。

### 3.22 [Low] 渲染器的 `DirectionToLight` 与 scene 的 forward 关系

- **相关文件**：[`RenderExtraction.cpp:110-111`](../../Engine/Source/Runtime/Renderer/Private/Scene/RenderExtraction.cpp)
- **问题描述**：
  - `renderLight.DirectionToLight = Math::Normalize(-forward);`
  - `forward` 取自 transform.GetWorldRotation()；
  - GPU `GPULight.DirectionType.x = normalized(DirectionToLight);`，shader PBR 用作 "to light" 方向。
- **为什么重要**：XEngine 约定 Light rays travel along +X forward，则 `direction from surface to light = -light forward`。代码逻辑正确（用了 negative forward）。但要注意 transform 默认 forward +X；如果 entity 没有 transform 但有 LightComponent，forward = +X，那 `DirectionToLight = -X`。这是 +X forward 的 light，如果 transform 旋转 90度 yaw 后 forward 是 +Y，`DirectionToLight = -Y`。这个事实 OK，但要 unit test。
- **推荐修复方式**：写一组 unit test，验证 forward 与 DirectionToLight 关系。

### 3.23 [Low] `RendererDebugSettings` 与 `ShadowDebugSettings` 字段重叠

- **相关文件**：[`RendererDebugSettings.h:6-29`](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererDebugSettings.h)
- **问题描述**：
  - `RendererDebugSettings` 内同时有 `VisualizeCascades`、`FreezeShadowMatrices` 与 `ShadowDebugSettings` 嵌套结构；其中 `ShadowDebugSettings` 内部又重复这两个字段。
  - 这意味着 UI 端要在 `VisualizeCascades` 上做二选一：当通过 `context.RendererDebug->VisualizeCascades` 还是 `context.RendererDebug->Shadows.VisualizeCascades`？ImGui 实际只连了前者。
- **推荐修复方式**：把 `ShadowDebugSettings` 内嵌的字段删除，并加注释 "all shadow debug settings now live on RendererDebugSettings for UI unification"。

### 3.24 [Low] `GPUFrameData::Shadows` 字段命名 vs `GPUShadowData` 类型

- **相关文件**：[`GPUFrameTypes.h`](../../Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUFrameTypes.h)
- **问题描述**：Shader 端 `struct GPUFrameData { GPULightingData Lighting; GPUShadowData Shadow; }` 字段名是 `Shadow`；C++ 端是 `Shadows`。**slang 与 C++ 字段名不一致**但 vec 类型相同，因为 Slang 在跨 struct 时用字段索引而非名字。但未来 Reflection 跨 ABI 验证时会出错。
- **推荐修复方式**：把 C++ 改成 `Shadow`，与 Slang 同名。

---

## 4. 架构边界问题

- **`RenderExtraction.cpp`** 引用 `Asset/AssetSystem.h`、`Asset/MeshAsset.h`、`Asset/MaterialAsset.h`、`Scene/Scene.h`，没有 RHI 头。这是对的。
- **`RenderSystem.cpp`** 引用 RHI、Scene、Asset、Shader，没有任何 Editor 或 Sandbox 头。这是对的。
- **`ShadowDepthPass.cpp`** 引用 `RHICommandList`、`RHIDevice`，RHI 公共层。不依赖 Vulkan ✓。
- **`ImGuiVulkanBackend.h`** 私有（Editor），完整依赖 Vulkan —— OK，因为这是 Vulkan adapter。
- **`BindlessResourceManager.{h,cpp}`** 应确认未被实际引用（否则违反 "本期不做 bindless"）。从 import 列表看似乎未被引用，但是存放位置合理（Renderer 私有）。
- **公共 Renderer header**：`Mesh.h`、`Material.h` 需扫描确认不暴露 RHI / Vulkan / Slang / stb。

---

## 5. 性能 / 生命周期 / 同步问题

- `GPUFrameData` 每帧重新 upload CPU->GPU，CPU 端 `Math::Inverse`（仅在 fallback camera path 用）— 每帧一两帧时 OK。
- `DirectionalShadowPlanner::BuildPlan` 每帧 cascade 重建 (`Math::Inverse(lightViewProj)`) —— 4 次/frame。目前场景规模小，无关紧要，但 Stage 12+ 多个 directional light 时要谨慎。
- `RenderShadowManager` 资源是 per-device 持久的，shadow 的 sampled view 在每次 recreate 之间复用同 shape —— 这正是 "persistent Texture2DArray"。
- `ShadowResourceCache` 在分辨率 / cascade count 改变时通过 `m_Directional = {};` 重置 —— 没有 `RHIBindGroup::Reset()` 通知；frame resources 内 Set 0 bind group 仍引用旧 view；即将 dangling。

---

## 6. 坐标 / 数学问题

- Render 系统使用 `Math::BuildViewMatrixLH_XForward` 直接生成 view matrix（见 RenderSystem.cpp:214），与 LightComponent forward 旋转自然兼容。
- 投影：`Math::PerspectiveLH_ZO` 然后 `ApplyRHIClipSpaceConvention(projection, device->GetClipSpaceConvention())` 这是对的。
- Shadow cascade projection 用了 `Math::OrthographicLH_ZO` + Reverse-Z via 参数 swap —— 但代码可读性差 (见 3.6 / 3.14)。
- Shadow 的 `ProjectToCascadeClip` + `ClipToShadowUV` 在 shader 中做 `mul(lightViewProj, worldPos)`，然后 `uv = clip.xy/clip.w * float2(0.5, -0.5) + 0.5` ——**Y 轴 flip 是 Vulkan 一致**。这条线与 C++ 端的 light 矩阵构建必须吻合（GLM 列主序，shader 也是列主序）。

---

## 7. 推荐修改

- 现在就修：
  - **3.3** `shadowLight = nullptr` 改 `==`；
  - **3.1 / 3.2 / 3.5** shadow bind group 显式注入；
  - **3.4** ShadowDepth pipeline kind；
  - **3.7** shadow bind group 顺序（PrepareFrame → FrameResources::Update → ForwardOpaque）；
  - **3.15** shadow resource 重建时通知 frame resources rebind。
- Stage 10 DebugDraw 期间修：
  - 3.6 / 3.14：cascade projection helper；
  - 3.24：Slang / C++ 字段同名；
- RenderGraph V1 前修：
  - 3.10：RenderGraph API 提到 base；
  - 3.11：GPUScene TODO Stage 13；
  - 3.13：FreezeShadowMatrices handle-based。
- 长期清理：
  - 3.16 / 3.17 / 3.18：planner / pipeline / frame context 抽象；
  - 3.22 / 3.23：debug settings 去重 + unit tests。

---

## 8. 可拆给 Claude Code 的具体任务

1. 修 `RenderShadowManager.cpp:124` 的 `=` 改为 `==`；同时在 `ShadowResourceCache.cpp:88` 之上加一行：`/ FIXME: notify renderer of shadow resource recreation (frame bind group rebind).`，**只注释 + 单字符修改**。
2. 在 `RenderFrameResources::Initialize` 把 `RHITextureView* shadowSampledView, RHISampler* shadowSampler` 作为新参数；`RenderSystem::Render` 在 ShadowManager 准备好后调用初始化函数；请保持所有现存 create 行为，仅添加。**单步 PR**。
3. 在 `BuildGPUFrameData` 签名加 `const RenderShadowManager& shadowMgr`，并填充 `data.Shadows = ...`；调用方 `RenderSystem.cpp` 与 `ShadowManager->FillGPUShadowData(data.Shadows)` 配对。
4. 在 `GraphicsPipelineStateKey` 与 `RenderPipelineStateCache::CreateGraphicsPipeline` 加 `RenderPassKind::ShadowDepth`，并写对应 DepthOnly 路径；**仅添加，不删除**。同时在 `ShadowDepthPass.cpp:46` 用新 PassKind 调用。
