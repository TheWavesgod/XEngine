# Stage 9 CSM Implementation Plan

**目标**: 实现渲染器端的阴影框架与 Cascaded Shadow Maps V0。
**范围**: 仅 C++/Slang 实现细节，不修改 `.xscene` 格式，不引入 RenderGraph V1，不破坏 Stage 8 已通过的 PBR 路径。
**方向**: CascadeCount=1 退化为普通方向光阴影；CascadeCount>1 走 CSM 路径，但共享同一份 RenderShadowManager / ShadowResourceCache / ShadowDepthPass 框架。

> 本文档是 AI 助手根据当前 XEngine 代码库状态生成的实施蓝图。**不会**直接修改任何源文件，使用者应据此文档手动实现并验证。

---

## 1. Current Code Audit（代码审计）

### 1.1 已存在的相关文件

| 路径 | 状态 | 备注 |
|------|------|------|
| [Project_Cache.md](../Project_Cache.md) | 现有 | 项目结构 / 绑定约定 / 转换规则的事实来源 |
| [RendererSettings.h](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererSettings.h) | 现有 | 已定义 `DirectionalShadowTechnique / ShadowMapStorageMode / ShadowFilterMode / DirectionalShadowSettings / ShadowSettings / RendererSettings`。**结构与期望完全一致** |
| [RendererDebugSettings.h](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RendererDebugSettings.h) | 现有 | 已定义 `ShadowDebugSettings`（`VisualizeCascades / FreezeShadowMatrices / ShowShadowMap / DebugCascadeLayer`）以及 `RendererDebugSettings` 中的 `VisualizeCascades / FreezeShadowMatrices` 直通字段 |
| [RenderShadowType.h](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h) | 现有 | 已定义 `MaxShadowCascades / RenderShadowCascade / RenderDirectionalShadowFrameData / RenderShadowFrameData` |
| [RenderShadowManager.h](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.h) | 现有 | 仅有声明 + `m_FrameData / m_DirectionalPlanner / m_ResourceCache / m_HasFrozenData / m_FrozenFrameData` 字段，**`.cpp` 为空** |
| [DirectionalShadowPlanner.h](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.h) | 现有 | 完整声明 `DirectionalShadowPlanDesc` 与 `BuildPlan()` |
| [DirectionalShadowPlanner.cpp](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp) | 半实现 | 已写 `ComputeCascadeSplits / GetCameraFrustumCornersWorldSpace / GetCascadeFrustumCornersWorldSpace / ComputeAverageCenter / ComputeBoundingSphereRadius / QuantizeRadius`；`BuildPlan` 内层循环体只剩一行 `// TODO`，**未填入 LightView / LightProjection / 视锥包围盒 / texel snap** |
| [ShadowResourceCache.h](../../Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h) | 半实现 | 声明齐全；`DirectionalShadowResources` 注释里把 `SampledView` 和 `LayerDepthViews` 标了 TODO，**未实现** |
| [ShadowResourceCache.cpp](../../Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp) | 现有 | **整文件为空**（仅文件头注释），需要从零实现 `Initialize / Shutdown / GetOrCreateDirectionalShadowResources` |
| [ShadowDepthPass.h](../../Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.h) | 现有 | **整文件只有 `#pragma once`**，没有 `AddShadowDepthPass()` 声明；对应 `.cpp` 不存在 |
| [GPUShadowTypes.h](../../Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h) | 现有 | `MaxShadowCascades / GPUCascadeShadowData / GPUShadowData` 已有，命名与期望完全一致 |
| [GPUFrameTypes.h](../../Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPUFrameTypes.h) | 现有 | `GPUFrameData` 已包含 `Shadows` 字段，但 `BuildGPUFrameData` **尚未填充** `data.Shadows` |
| [RenderFrameResources.h](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.h) | 现有 | 类声明存在；尚无 shadow 资源 / 阴影相关 API |
| [RenderFrameResources.cpp](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp) | 半实现 | 当前只创建 Set 0 binding 0（仅 `UniformBuffer` 描述符）。`BuildGPUFrameData` 未填 `Shadows` 字段，未持有 shadow texture/sampler 句柄，未做 image-layout 过渡 |
| [ForwardRenderPipeline.cpp](../../Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp) | 现有 | 当前只 `AddClearPass → AddForwardOpaquePass → AddPresentPass`；TODO 行明确写出 "Stage 8B/8C/8D: add lighting and shadow passes" |
| [ForwardOpaquePass.cpp](../../Engine/Source/Runtime/Renderer/Private/Passes/ForwardOpaquePass.cpp) | 现有 | 已经在用 Set 0 (`frameBindGroup`) + Set 1 (`bindGroup`)；未绑定 shadow texture/sampler。TODO 行写有 "Stage 9: Declare HDR color/depth graph resources" |
| [RenderResourceContext.h](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderResourceContext.h) | 现有 | 包含 `Textures / Meshes / Materials / Shaders / PipelineStates / FrameResources`；**尚未挂入 `Shadows`**（`RenderResourceContext` 注释里有 "Stage 9 and later: RenderShadowManager*" 但字段未加） |
| [RenderSystem.cpp](../../Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp) | 现有 | 当前 `Update(frame, SceneData)` 后直接调 `ActivePipeline->Render`；未持有 `RenderShadowManager`、未调用 `PrepareFrame`、未把设置传入 |
| [RenderExtraction.cpp](../../Engine/Source/Runtime/Renderer/Private/Scene/RenderExtraction.cpp) | 现有 | 已经在把 `LightComponent` 转换成 `RenderLight`，并写入 `RenderObject::CastShadow/ReceiveShadow`。`RenderLight::CastShadow` 字段已存在 |
| [LightComponent.h](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Components/LightComponent.h) | 现有 | 只包含 `Enabled / CastShadow / Color / Intensity / Type / Range / Cone*`；**不包含** CSM 配置（符合期望） |
| [MeshRendererComponent.h](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Components/MeshRendererComponent.h) | 现有 | 已有 `Visible / CastShadow / ReceiveShadow` 字段（与期望完全一致） |
| [RenderScene.h](../../Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderScene.h) | 现有 | `RenderObject` 已包含 `CastShadow/ReceiveShadow`；`RenderLight` 已有 `CastShadow/Enabled/DirectionToLight` |
| [Editor/RendererDebugPanel.cpp](../../Engine/Source/Editor/Private/Panels/RendererDebugPanel.cpp) | 现有 | 已有 `VisualizeCascades / FreezeShadowMatrices` 两个 checkbox（与期望一致），但只有 UI 占位，无实际效果 |
| [Renderer/CMakeLists.txt](../../Engine/Source/Runtime/Renderer/CMakeLists.txt) | 现有 | 已 `GLOB_RECURSE` 包含 `Private/Shadows/*.h / *.cpp` 以及 `Private/Passes/*.h / *.cpp`，新文件无需改构建脚本 |
| [RHITexture.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITexture.h) | 现有 | 描述符里已有 `Width / Height / MipLevels / ArrayLayers / Format / Dimension / Usage`；**关键观察**: `ArrayLayers` 已存在，但 **没有 `RHITextureView` 抽象类**，需要新增 |
| [RHITypes.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h) | 现有 | 已有 `RHITextureDimension::Texture2DArray`、`RHITextureUsageFlags::DepthStencilAttachment / Sampled` |
| [RHICommandList.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICommandList.h) | 现有 | 已有 `SetRenderOutput / SetGraphicsPipeline / SetRenderViewport / TransitionTextureToShaderRead / SetBindGroup / PushConstants / Draw / DrawIndexed`。**注意**: `SetRenderOutput` 当前只接受单个 `RHIRenderOutputDesc`（仅支持 1 个 color target + 1 个 depth target），shadow pass 需要的是"零 color target + 单 layer depth"，**需要扩展或者绕过** |
| [RHIPipeline.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h) | 现有 | `RHIGraphicsPipelineDesc` 只支持单个 `ColorFormat` + `DepthFormat`，**没有"无 color attachment"的标志**。Vulkan 后端 `VulkanPipeline.cpp` 中 `VkPipelineRenderingCreateInfo` 也写死 `colorAttachmentCount = 1`，需要改造为可选 |
| [RHIBindGroup.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBindGroup.h) | 现有 | 已有 `RHIBindGroupLayoutEntry` 描述符以及 `RHIBindingResource { Texture / Sampler / Buffer }` 联合体 |
| [VulkanTexture.cpp](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp) | 半实现 | `GetImageViewType` 把 `Texture2DArray` 视为 2D（缺失分支），需要补 `VK_IMAGE_VIEW_TYPE_2D_ARRAY`；其它创建流程（`vmaCreateImage` + 一次性 `vkCreateImageView`）本身可复用 |
| [VulkanDescriptor.cpp](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp) | 半实现 | `VulkanBindGroup::Create` 已能处理 `CombinedImageSampler` 与 `UniformBuffer`，**但没有 `SampledTexture`（去耦 texture+sampler）/ `Sampler` 单独类型**的分支，shadow 绑定要"texture array + sampler"会落到 `CombinedImageSampler` 但 shadow 通常需要独立 sampler。需要视 Stage 9 决定是否扩张 descriptor 池 |
| [ForwardPBR.slang](../../Engine/Shaders/Passes/ForwardPBR.slang) | 现有 | 当前在用 `g_FrameData.Lighting` 调 `EvaluateSceneLighting`；未读取 shadow 数据；未声明 `Texture2DArray` shadow 绑定 |
| [Common/Types.slang](../../Engine/Shaders/Common/Types.slang) | 现有 | `GPUFrameData` 中**没有** `GPUShadowData` 字段（与 C++ 端 `GPUShadowData` 已存在不同步），需要补 |
| [Lighting/LightingTypes.slang](../../Engine/Shaders/Lighting/LightingTypes.slang) | 现有 | `GPULightingData` / `GPULight` 已存在；没有 `GPUShadowData` / `GPUCascadeShadowData`，需要新增 |
| [Passes/DepthOnly.slang](../../Engine/Shaders/Passes/DepthOnly.slang) | 现有 | **整文件仅一个 TODO 注释**，shadow depth pass 需要这份 shader，需要从头写 |
| [Passes/ImGui.slang](../../Engine/Shaders/Passes/ImGui.slang) | 现有 | 仅 TODO 注释；本次不需要 |

### 1.2 已完成 / 部分完成的工作

* 数据结构骨架**齐全**：`DirectionalShadowSettings / ShadowSettings / RendererSettings`、CPU 侧 `RenderShadowCascade / RenderDirectionalShadowFrameData / RenderShadowFrameData`、GPU 侧 `GPUCascadeShadowData / GPUShadowData` 命名与字段都对得上期望。
* `DirectionalShadowPlanner` 的数学工具（`ComputeCascadeSplits / GetCameraFrustumCornersWorldSpace / GetCascadeFrustumCornersWorldSpace / ComputeAverageCenter / ComputeBoundingSphereRadius / QuantizeRadius`）已经写好。
* 调试面板与调试设置结构已经就绪（`VisualizeCascades / FreezeShadowMatrices`）。
* `RenderObject::CastShadow / ReceiveShadow`、`RenderLight::CastShadow`、`LightComponent::CastShadow / Enabled` 字段齐全。
* `RenderResourceContext` 注释里已经标注 "Stage 9 and later: RenderShadowManager*" 字段，**实际未加**。
* Renderer `CMakeLists.txt` 已经把 `Private/Shadows/*.cpp` 和 `Private/Passes/*.cpp` 包含在 GLOB 里。

### 1.3 缺失 / 不完整的工作

* **`.cpp` 实现层几乎全空**：`RenderShadowManager.cpp / ShadowResourceCache.cpp` 整个文件无函数体；`ShadowDepthPass.cpp` 不存在。
* **`DirectionalShadowPlanner::BuildPlan` 主循环未完成**（只剩 `// TODO`）。
* **`RHITextureView` 抽象类不存在**——ShadowResourceCache 期望产出 `RHITextureView*`（per-cascade depth view + whole-array sampled view），需要新增一个 RHI 抽象以及 Vulkan 实现。
* **RHICommandList `SetRenderOutput` 不支持"零 color attachment + 任意 layer"** 的 depth-only 输出，需要扩展。
* **RHI 流水线描述符不支持 0 个 color attachment**，`VulkanPipeline.cpp` 写死 `colorAttachmentCount = 1`，需要为 depth-only pipeline 增加 `ColorFormat = Undefined` 语义。
* **绑定约定中 `Set 0` 目前只有 binding 0（GPUFrameData）**，需要补 binding 1（shadow texture array）和 binding 2（shadow sampler）。
* **Descriptor 类型只支持 `CombinedImageSampler / UniformBuffer / StorageBuffer`**，独立 shadow sampler 需要从 `CombinedImageSampler` 路径走（shadow texture + shadow sampler 放同一条 `CombinedImageSampler` 即可，但需要先确认）。
* **`VulkanTexture::GetImageViewType` 把 `Texture2DArray` 错为 2D**。
* **Shader 侧 `GPUShadowData` 未声明**（`Types.slang` 与 `LightingTypes.slang` 都需要新增）。
* **`ForwardPBR.slang` 未声明 shadow binding、未调用 shadow helper**。
* **没有 `ShadowDepth.slang`**，且 `DepthOnly.slang` 也是空壳。
* **`ForwardRenderPipeline` 没有把 `RenderShadowManager` 串进来**，没有调用 `PrepareFrame`、没有添加 `AddShadowDepthPass`。
* **`RenderResourceContext` 未持有 `RenderShadowManager*`**（`Shadows` 字段缺失）。
* **`RenderFrameResources` 未持有 `GPUShadowData` 上传路径、未持有 shadow texture/sampler 句柄**。
* **没有编辑器 UI 控制** `RendererSettings`（`RendererSettings` 已经定义但 `RendererSystem` 还未提供 getter/setter；`RendererDebugPanel` 还没有任何写 `RendererSettings` 的入口；`RendererDebugSettings` 中 `VisualizeCascades` / `FreezeShadowMatrices` 虽已在，但功能没接）。
* **`ShadowValidation.xscene`**（在 `Assets/Scenes/`）尚未被 Sandbox/Editor 加载过（待确认），需要在 Stage 9 验证流程中加载它。

### 1.4 架构风险 / 现状隐患

1. **CPU/GPU 结构不同步**: `GPUShadowData` 在 C++ 端已经存在，但 `Common/Types.slang` 的 `GPUFrameData` 里没有 `Shadows` 字段；C++ 端 `data.Shadows` 也未被填充。这意味着只要 `BuildGPUFrameData` 一旦填上 `Shadows`，C++ struct size 与 Slang struct size 就会错位（`static_assert` 也会炸）。必须在任何上传前先补 Slang 端 layout。
2. **GPUFrameData bind group layout 与阴影不匹配**: 当前 layout 只有 binding 0 = `UniformBuffer`。Shadow 要求 `Set 0: binding 0 = GPUFrameData / binding 1 = shadow texture array / binding 2 = shadow sampler`。增加 binding 1 / 2 会让所有 `ForwardOpaquePass`/`ShadowDepthPass` 的 pipeline layout 同时改变。
3. **RHI 没有 `RHITextureView` 抽象**: 这是一个**架构缺口**，Shadow 一定需要 per-layer depth view + whole-array sampled view。建议把 `RHITextureView` 设计成只持有 native handle + 描述信息（base layer / layer count / aspect / format），不要让 RHI 知道"cascades"。
4. **RHI 不支持 depth-only 渲染**: 现有 `RHICommandList::SetRenderOutput` 强制要求一个 swapchain/render target，shadow pass 期望"零 color target + 单 layer depth"。需要扩展 `RHIRenderOutputDesc` 或新增 `BeginDepthOnlyRendering` API。
5. **`RHIGraphicsPipelineDesc` 写死 color attachment**: `colorAttachmentCount = 1` 硬编码在 `VulkanPipeline.cpp`。Depth-only pipeline 需要走 `colorAttachmentCount = 0`。
6. **`VulkanTexture::GetImageViewType` 缺少 `Texture2DArray` 分支**: 当前默认走 `VK_IMAGE_VIEW_TYPE_2D`，会导致 shadow 数组视图错误。
7. **`RenderScene::Lights` 顺序未文档化**: `RenderShadowManager` 准备从 `RenderScene.Lights` 找"第一个"有阴影的方向光，但 `RenderExtraction` 的写入顺序没有契约。建议在 `RenderShadowManager::PrepareDirectionalShadow` 文档里写明 "First enabled directional with `CastShadow = true` wins"；同时也把"光排序"留作 Stage 10+。
8. **`RenderFrameContext` 没有 camera near/far 字段**: 当前 `RenderSystem` 通过 `camera->NearPlane / FarPlane` 计算投影，但 `RenderShadowManager::PrepareFrame` 接收的 `RenderFrameContext` 里没有这两个字段。需要新增 `CameraNear / CameraFar`。
9. **`RendererSettings` 没有"活动实例"**: 现有代码没有 `RenderSystem::GetRendererSettings()`，也没有向 `RenderShadowManager` 注入 settings 的路径。Stage 9 必须建立这条链路。
10. **`RHISampler` 没有 comparison sampler 字段**: Stage 9 V0 仅 PCF/Hard，可暂时用普通 `Linear + ClampToEdge` 即可，但需要在 plan 里留出扩展点（`RHISamplerDesc` 增加 `CompareEnable` / `CompareOp`）以便 Stage 10+。

---

## 2. Target Architecture（目标架构）

### 2.1 数据流（最终形态）

```text
Scene LightComponent
  │  (RenderExtraction)
  ▼
RenderScene.Lights
  │
  ▼
RenderShadowManager::PrepareFrame(device, scene, frame, settings, debugSettings)
  ├── DirectionalShadowPlanner::BuildPlan(desc, outData)
  │     └── 填 RenderDirectionalShadowFrameData.Cascades[]
  ├── ShadowResourceCache::GetOrCreateDirectionalShadowResources(device, desc)
  │     └── 产出 RHITexture* + RHITextureView*(sampled) + array<RHITextureView*, 4>(depth)
  │     └── 产出 RHISampler*(compare)
  ├── 若 debugSettings.FreezeShadowMatrices && m_HasFrozenData：使用 m_FrozenFrameData
  ├── 否则填 m_FrameData；debug 开启则复制到 m_FrozenFrameData
  └── FillGPUShadowData(gpuShadowData) 填充 GPUShadowData
  │
  ▼
RenderSystem 在 Update(frame, scene) 之前调用 RenderShadowManager::PrepareFrame(...)
  │
  ▼
RenderFrameResources::Update(frame, scene, shadowData)
  ├── 上传 GPUFrameData (含 Shadows)
  └── 若 shadowData.Enabled：更新 frame bind group 的 binding 1 / binding 2
  │
  ▼
ForwardRenderPipeline::Render
  ├── AddClearPass
  ├── AddShadowDepthPass(graph, frame, scene, resources, shadowManager)
  ├── AddForwardOpaquePass(graph, frame, scene, resources)  // 读 shadow
  └── AddPresentPass
```

### 2.2 所有权

```text
RenderSystem
  ├── RenderShadowManager (unique_ptr) ── 持 ShadowResourceCache / DirectionalShadowPlanner / frozen data
  ├── RenderFrameResources (现有)
  └── RendererSettings (struct, not owned)

ForwardRenderPipeline
  ├── m_Graph
  └── 无 shadow 资源所有权

ShadowResourceCache
  ├── std::shared_ptr<RHITexture>           // Texture2DArray
  ├── std::shared_ptr<RHITextureView>       // sampled view (whole array)
  ├── std::array<std::shared_ptr<RHITextureView>, MaxShadowCascades>  // per-layer depth view
  └── std::shared_ptr<RHISampler>           // compare sampler (or linear + clamp to edge for V0)

DirectionalShadowPlanner
  └── 仅 CPU 数学；无 RHI 资源

ShadowDepthPass
  └── 无状态；通过 lambda 捕获外部数据

RenderFrameResources
  └── 持有 shadow bind group（可选），指向 ShadowResourceCache 的资源
```

### 2.3 绑定约定（Stage 9 之后）

```text
Set 0 = per-frame global data
  binding 0 : GPUFrameData uniform buffer
  binding 1 : shadow texture (Texture2DArray, combined image sampler)
  binding 2 : shadow sampler (CombinedImageSampler 的 sampler component，
              实际 V0 可由 binding 1 同时承载)
```

> **实施简化**: 第一版（V0）把 `Set 0: binding 1` 设为 `CombinedImageSampler`（texture array + sampler），**不要**额外拆分 binding 2。Stage 10+ 再评估是否需要独立 sampler 用于无 sampler 的 PCF。

---

## 3. File-by-File Plan（按文件）

### 3.1 `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h`

* **状态**: 现有
* **职责**: CPU 侧 shadow 帧数据布局（保持不变即可，已经匹配期望）
* **变更**: 确认 `BiasParams` 字段语义（建议 `x = depth bias / y = normal bias / z = slope-scaled bias factor / w = reserved`），与 GPU 端 `Params` 字段对齐
* **依赖**: 仅 `<XEngine/Core/Types.h>` 与 `<XEngine/Math/Math.h>`

### 3.2 `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.h`

* **状态**: 现有
* **职责**: 声明 CSM 数学的输入描述与 `BuildPlan()`
* **变更**:
  * 给 `DirectionalShadowPlanDesc` 增字段：
    * `bool ReverseZ = true;`（Vulkan 反向 Z 适配）
    * `AABB WorldBounds`（已经在，但需补注释说明它来自 RenderExtraction 聚合的 `OpaqueObjects[i].WorldBounds`）
  * `BuildPlan` 签名保持不变
* **依赖**: `RenderShadowType.h` / `RendererSettings.h`

### 3.3 `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp`

* **状态**: 半实现（主循环缺主体）
* **变更**:
  * 完成 `BuildPlan` 的 for-loop：
    1. 用 `GetCascadeFrustumCornersWorldSpace` 拿 8 个世界空间角点
    2. `ComputeAverageCenter` + `ComputeBoundingSphereRadius` 算世界空间包围球
    3. 用 `QuantizeRadius` 稳定半径
    4. `Math::LookAtLH_XForward(center - lightDir * radius, center, CoordinateSystem::Up)` 构造 light view
    5. `Math::OrthographicLH_ZO(-r, r, -r, r, -r - bias, r + bias)` 构造 light proj（深度扩展一个 bias 避免遮挡）
    6. 若 `desc.StabilizeCascades`：`Math::SnapToTexel(LightViewProj, resolution)`（V0 简单用 floor/cell）
    7. 写 `outData.Cascades[i]`：`LightView / LightProjection / LightViewProjection / SplitNear / SplitFar / LayerIndex = i / Resolution / ShadowMapSize / BiasParams / WorldBounds / LightSpaceBounds`
    8. 准备下一级 `previousSplit = splitFar`
* **依赖**: `Math/MathFunctions.h`、`Math/CoordinateSystem.h`、`Math/CameraMatrices.h`

> 现有代码中 `ComputeCascadeSplits` **没有把结果写进 `outSplits`**——属于 bug，Stage 9 实现时必须修复。

### 3.4 `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h`

* **状态**: 半实现
* **变更**:
  * 取消 `DirectionalShadowResources` 中 `SampledView / LayerDepthViews` 的 TODO 注释，改为正式字段：
    ```cpp
    struct DirectionalShadowResources
    {
        std::shared_ptr<RHITexture>       Texture;
        std::shared_ptr<RHITextureView>   SampledView;          // Texture2DArray, 整 array
        std::array<std::shared_ptr<RHITextureView>, MaxShadowCascades> LayerDepthViews {};
        std::shared_ptr<RHISampler>       Sampler;
        u32 Resolution = 0;
        u32 CascadeCount = 0;
        RHIFormat Format = RHIFormat::Undefined;
    };
    ```
  * 新增 `RHITextureViewDesc`（在 RHI 层定义）并在 `GetOrCreateDirectionalShadowResources` 内创建
* **依赖**: RHI 扩展（见 §3.13、§3.14）

### 3.5 `Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp`

* **状态**: 空
* **变更**:
  * `Initialize(RHIDevice&)`：创建默认 `RHISamplerDesc{ Filter::Linear, AddressU/V/W::ClampToBorder, MaxAnisotropy=1 }` 作为比较 fallback（V0 不开 compare）
  * `Shutdown(RHIDevice&)`：清空所有 `shared_ptr`
  * `GetOrCreateDirectionalShadowResources(device, desc)`：
    1. 若当前资源 `m_Directional` 与 `desc` 匹配（`Resolution / CascadeCount / Format / StorageMode`），直接返回
    2. 否则销毁旧资源：
       * `RHITexture*` 由 `shared_ptr` 自动析构
       * 但 sampler / views 需要显式 reset
    3. 创建 `RHITextureDesc{ Width=Resolution / Height=Resolution / MipLevels=1 / ArrayLayers=CascadeCount / Format=D32Float / Dimension=Texture2DArray / Usage=Sampled | DepthStencilAttachment }`
    4. 创建 `SampledView`：`RHITextureViewDesc{ Texture, ViewType=Texture2DArray, BaseMipLevel=0, MipCount=1, BaseArrayLayer=0, ArrayLayerCount=CascadeCount, Aspect=Depth }`
    5. 对每个 cascade 创建一个 `LayerDepthView`：`BaseArrayLayer=i, ArrayLayerCount=1, Aspect=Depth`，用于 depth attachment
    6. 写回 `m_Directional`，返回引用
* **依赖**: `RHITextureView`（新增，见 §3.14）

### 3.6 `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.h`

* **状态**: 现有
* **变更**: 无需改动（接口已匹配）
* **依赖**: `RenderShadowType.h` / `DirectionalShadowPlanner.h` / `ShadowResourceCache.h`

### 3.7 `Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp`

* **状态**: 空
* **变更**:
  * `Initialize(RHIDevice&)`：调用 `m_ResourceCache.Initialize(device)`
  * `Shutdown(RHIDevice&)`：调用 `m_ResourceCache.Shutdown(device)`，清空 `m_FrameData / m_FrozenFrameData / m_HasFrozenData`
  * `PrepareFrame(device, scene, frame, settings, debugSettings)`：
    1. 若 `debugSettings.FreezeShadowMatrices && m_HasFrozenData`：`m_FrameData = m_FrozenFrameData; return;`
    2. 重置 `m_FrameData`（`Directional.Enabled = false; CascadeCount = 0;`）
    3. 找到第一盏 `Type == Directional && Enabled && CastShadow` 的 `RenderLight`：
       * 没有则直接返回（阴影关）
    4. 构造 `DirectionalShadowResourceDesc{ Resolution=settings.Resolution, CascadeCount=settings.CascadeCount, DepthFormat=D32Float, StorageMode=settings.StorageMode }`
    5. `auto& res = m_ResourceCache.GetOrCreateDirectionalShadowResources(device, desc)`
    6. 拷贝 `res.Texture / SampledView / Sampler / LayerDepthViews` 到 `m_FrameData.Directional`
    7. 聚合 `OpaqueObjects` 的 `WorldBounds` 为 `sceneBounds`（若空，用一个巨大盒子兜底）
    8. 构造 `DirectionalShadowPlanDesc`：
       ```cpp
       DirectionalShadowPlanDesc planDesc;
       planDesc.Light              = &light;
       planDesc.CameraView         = frame.ViewMatrix;
       planDesc.CameraProjection   = frame.ProjectionMatrix;
       planDesc.CameraNear         = frame.CameraNear;
       planDesc.CameraFar          = frame.CameraFar;
       planDesc.CameraPosition     = frame.CameraWorldPosition;
       planDesc.SceneBounds        = sceneBounds;
       planDesc.CascadeCount       = settings.CascadeCount;
       planDesc.Resolution         = settings.Resolution;
       planDesc.SplitLambda        = settings.SplitLambda;
       planDesc.DepthBias          = settings.DepthBias;
       planDesc.NormalBias         = settings.NormalBias;
       planDesc.StabilizeCascades  = settings.StabilizeCascades;
       ```
    9. `m_DirectionalPlanner.BuildPlan(planDesc, m_FrameData.Directional)`
    10. 写 `Directional.Enabled` 由 `BuildPlan` 决定
    11. 若 `debugSettings.FreezeShadowMatrices`：`m_FrozenFrameData = m_FrameData; m_HasFrozenData = true;`
  * `FillGPUShadowData(GPUShadowData& out)`：
    1. `out.ShadowParams = Vec4{ Enabled?1:0, (float)CascadeCount, (float)Resolution, debug.VisualizeCascades?1:0 }`
    2. 对每个 cascade：`out.Cascades[i] = { LightViewProjection, Vec4{ SplitFar, DepthBias, NormalBias, 1.0f/Resolution } }`
* **依赖**: `RenderScene.h` / `RenderExtraction.h`（只用 OpaqueObjects.WorldBounds 聚合）

### 3.8 `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.h`

* **状态**: 仅有占位
* **变更**:
  ```cpp
  #pragma once
  #include "../../../Public/XEngine/Renderer/RenderScene.h"

  namespace XEngine
  {
      class RenderGraph;
      struct RenderFrameContext;
      struct RenderResourceContext;
      class  RenderShadowManager;
      class  RenderPipelineStateCache;

      void AddShadowDepthPass(
          RenderGraph& graph,
          const RenderFrameContext& frameContext,
          const RenderScene& renderScene,
          RenderResourceContext& resources,
          RenderShadowManager& shadowManager,
          RenderPipelineStateCache& pipelineStates);
  }
  ```
* **依赖**: `RenderScene.h`

### 3.9 `Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp`

* **状态**: 不存在
* **变更**:
  * `AddShadowDepthPass` 注册一个 lambda 闭包，遍历 `0..CascadeCount-1`：
    1. 调 `commandList->SetRenderOutput(RHIRenderOutputDesc{ ColorTarget=nullptr, DepthTarget=m_FrameData.Directional.CascadeDepthViews[i], Viewport={0,0,Resolution,Resolution}, ColorFormat=Undefined, DepthFormat=D32Float, RenderToSwapchain=false })`
    2. `commandList->SetRenderViewport({0,0,res,res})`
    3. `commandList->SetGraphicsPipeline(shadowDepthPipeline)`（通过 `RenderPipelineStateCache` 拿 `PassKind::ShadowDepth`）
    4. `commandList->SetBindGroup(0, frameBindGroup)`（注意：shadow pass 也用 frame bind group，因为 `Set 0` 包含 `GPUFrameData`，shadow shader 需要 `Shadows.LightViewProjection[i]`）
    5. 遍历 `renderScene.OpaqueObjects`，过滤 `object.CastShadow`：
       * `commandList->SetVertexBuffer / SetIndexBuffer`
       * 通过 push constant 写入 `WorldMatrix`
       * `commandList->DrawIndexed(...)`
  * 关键点：shadow pass 一次只渲染一个 cascade，所以每个 cascade 是一次 `SetRenderOutput` 调用
* **依赖**: `RHICommandList` 扩展（见 §3.15）、`RenderPipelineStateCache` 扩展（见 §3.11）

### 3.10 `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.h`

* **状态**: 现有
* **变更**:
  * `Update` 签名改为：
    ```cpp
    void Update(
        const RenderFrameContext& frame,
        const RenderScene& scene,
        const GPUShadowData& shadowData,
        const RenderShadowFrameData& shadowResources,
        const ShadowDebugSettings& shadowDebug);
    ```
  * 新增 getter：`GetFrameBindGroupLayout()` 已存在；新增 `GetFrameBuffer(frameIndex)` 已存在
  * 新增私有：`BuildGPUShadowData(...)` 改为 inline `Update` 接收外部填充好的 `GPUShadowData`
* **依赖**: `GPUShadowTypes.h`、`RendererDebugSettings.h`

### 3.11 `Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp`

* **状态**: 半实现
* **变更**:
  * **绑定 layout 扩展**：当前只有 `binding 0 = UniformBuffer`；新增 `binding 1 = CombinedImageSampler (count=1)`、`binding 2 = CombinedImageSampler (count=1)`：
    ```cpp
    layoutDesc.Entries.push_back({ 1, RHIBindingType::CombinedImageSampler, RHIShaderStageFlags::Fragment, 1 });
    ```
    *Stage 9 V0 把 sampler 跟 texture 合并到 binding 1 上就够了；binding 2 留空*

    > **实施选择（推荐）**:  只扩展 `binding 1` 为 `CombinedImageSampler`（Texture2DArray + Sampler），**不开 binding 2**。Stage 10+ 需要独立 sampler 时再补 binding 2。
  * **每帧重建 vs 缓存**:
    * Stage 9 V0：frame bind group 创建时就绑定空 placeholder，每帧更新时如果 shadow texture 改变，调用 `vkUpdateDescriptorSets` 直接更新；**不要每帧重新分配 bind group**
    * RenderFrameResources 需要持有一个"可写"的 frame bind group 实现：当前 `RHIBindGroup` 是只读抽象。建议增加一个 `UpdateSampledTexture(u32 binding, RHITexture*, RHISampler*)` 的 RHI API。
  * `BuildGPUFrameData` 内部改为：
    ```cpp
    data.Camera    = BuildGPUCameraData(frame);
    data.Lighting  = BuildGPULightingData(scene);
    data.Shadows   = shadowData;  // 直接拷贝
    ```
  * `Update` 内部：上传 `GPUFrameData` 到 uniform buffer；调用 `UpdateSampledTexture(1, shadowResources.Directional.ShadowTexture, shadowResources.Directional.Sampler)` 当 `Enabled`，否则传 null placeholder
* **依赖**: `RHICommandList` / `RHIBindGroup` 扩展

### 3.12 `Engine/Source/Runtime/Renderer/Private/Resources/RenderResourceContext.h`

* **状态**: 现有
* **变更**:
  * 取消 "Stage 9 and later" 注释，新增实际字段：
    ```cpp
    class RenderShadowManager;
    struct RenderResourceContext
    {
        RenderTextureManager*   Textures      = nullptr;
        RenderMeshManager*      Meshes        = nullptr;
        RenderMaterialSystem*   Materials     = nullptr;
        RenderShaderLibrary*    Shaders       = nullptr;
        RenderPipelineStateCache* PipelineStates = nullptr;
        RenderFrameResources*   FrameResources = nullptr;
        RenderShadowManager*    Shadows       = nullptr;     // NEW
        ...
    };
    ```
  * `IsValid()` 暂时**不**强制 `Shadows != nullptr`（让 Editor/Sandbox 可以独立于 shadow 启动）；但在 ForwardRenderPipeline 中要显式 `if (resources.Shadows)` 才加 shadow pass
* **依赖**: 无

### 3.13 `Engine/Source/Runtime/Renderer/Private/Resources/GraphicsPipelineStateKey.h`

* **状态**: 现有
* **变更**:
  * 新增 `PipelineLayoutVersion` 递增（`1 → 2`）以让现有的 ForwardOpaque pipeline 重新创建（因为 frame bind group layout 变了）
  * 无需新枚举
* **依赖**: 无

### 3.14 `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITextureView.h`（**新文件**）

* **状态**: 新增
* **职责**: 通用 RHI 抽象，描述一个具体 image view（不暴露 Vulkan）
* **建议定义**:
  ```cpp
  namespace XEngine
  {
      enum class RHITextureViewDimension : u8
      {
          Texture2D,
          Texture2DArray,
          TextureCube,
      };

      enum class RHITextureAspectFlags : u8
      {
          Color  = 1 << 0,
          Depth  = 1 << 1,
          Stencil= 1 << 2,
      };

      struct RHITextureViewDesc
      {
          RHITexture*                 Texture = nullptr;
          RHITextureViewDimension     ViewDimension = RHITextureViewDimension::Texture2D;
          RHIFormat                   Format = RHIFormat::Undefined;
          u32                         BaseMipLevel = 0;
          u32                         MipCount = 1;
          u32                         BaseArrayLayer = 0;
          u32                         ArrayLayerCount = 1;
          RHITextureAspectFlags       Aspect = RHITextureAspectFlags::Color;
          const char*                 DebugName = nullptr;
      };

      class RHITextureView
      {
      public:
          virtual ~RHITextureView() = default;
          virtual const RHITextureViewDesc& GetDesc() const = 0;
          virtual void* GetNativeImageView(RHIBackend backend) const { (void)backend; return nullptr; }
      };
  }
  ```
* **依赖**: `RHITypes.h`、`RHITexture.h`

### 3.15 `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h`

* **状态**: 现有
* **变更**:
  * 新增 `CreateTextureView(const RHITextureViewDesc& desc)` 工厂方法
  * 新增 `UpdateBindGroupSampledTexture(RHIBindGroup* bg, u32 binding, RHITextureView* view, RHISampler* sampler)` 让 frame bind group 每帧更新 shadow binding 而不重建
* **依赖**: `RHITextureView.h`

### 3.16 `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICommandList.h`

* **状态**: 现有
* **变更**:
  * 现有 `SetRenderOutput(const RHIRenderOutputDesc& output)` 保持不变，但需要支持 `ColorTarget == nullptr + ColorFormat == Undefined` 作为"depth-only rendering"的合法输入
  * 配套 `RHIRenderOutputDesc` 在 `RHITypes.h` 中：
    ```cpp
    struct RHIRenderOutputDesc
    {
        RHITexture*     ColorTarget   = nullptr;   // shadow pass: nullptr
        RHITextureView* DepthTarget   = nullptr;   // shadow pass: per-layer depth view
        RHIRect2D       Viewport      {};
        RHIFormat       ColorFormat    = RHIFormat::BGRA8Unorm;
        RHIFormat       DepthFormat    = RHIFormat::D32Float;
        bool            RenderToSwapchain = true;
    };
    ```
    **建议把 `DepthTarget` 从 `RHITexture*` 升级为 `RHITextureView*`**，否则 shadow pass 无法指定 per-layer depth view
* **依赖**: `RHITextureView.h`

### 3.17 `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h`

* **状态**: 现有
* **变更**:
  * `RHIGraphicsPipelineDesc` 新增 `bool HasColorAttachment = true;`（默认 true 保持兼容；shadow pipeline 设 false）
  * `RHIFormat ColorFormat = RHIFormat::Undefined` 已经合法；只在新加的字段里区分"显式 0 attachment"与"color attachment 写 0 字节"
* **依赖**: 无

### 3.18 `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp`

* **状态**: 半实现
* **变更**:
  * 新增 `CreateTextureView(const RHITextureViewDesc& desc)`：
    * 把 `RHITextureViewDimension` 映射到 `VkImageViewType`：
      ```cpp
      VK_IMAGE_VIEW_TYPE_2D / _2D_ARRAY / _CUBE
      ```
    * 修补 `VulkanTexture::GetImageViewType` 的缺漏（独立 helper）
  * 修补 `VulkanPipeline` 构造：当 `desc.HasColorAttachment == false` 时，`VkPipelineRenderingCreateInfo.colorAttachmentCount = 0; pColorAttachmentFormats = nullptr;`
  * 修补 `VkPipelineColorBlendStateCreateInfo`：当 0 color attachment 时整个 color blend state 仍需要合法结构（`attachmentCount = 0` 即可）
  * 修补 `descriptor pool`：在 `CreateDescriptorPool` 中把 `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` 的容量从 1024 增大到 2048（shadow array 占 3 * N 个 frame-in-flight）
  * 新增 `UpdateBindGroupSampledTexture`：直接 `vkUpdateDescriptorSets`（参考 `VulkanDescriptor.cpp` 现有 `VulkanBindGroup::Create` 的 write 流程）
* **依赖**: 新增 `VulkanTextureView`（对应 RHITextureView 的 Vulkan 实现）

### 3.19 `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.h/.cpp`（**新文件**）

* **状态**: 新增
* **职责**: Vulkan 实现 `RHITextureView`
* **建议**:
  ```cpp
  class VulkanTextureView final : public RHITextureView
  {
      VkDevice m_Device = VK_NULL_HANDLE;
      VkImageView m_ImageView = VK_NULL_HANDLE;
      RHITextureViewDesc m_Desc {};
  };
  ```
  * `Create` 把 desc 翻译成 `VkImageViewCreateInfo`，其中 `subresourceRange.aspectMask` 由 `RHITextureAspectFlags` 翻译成 `VK_IMAGE_ASPECT_DEPTH_BIT` / `VK_IMAGE_ASPECT_COLOR_BIT`
* **依赖**: `RHITextureView.h`

### 3.20 `Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.h`

* **状态**: 现有
* **变更**:
  * `Render(...)` 内部使用 `resources.Shadows` 决定是否插入 ShadowDepthPass
* **依赖**: 无

### 3.21 `Engine/Source/Runtime/Renderer/Private/Pipeline/ForwardRenderPipeline.cpp`

* **状态**: 现有
* **变更**:
  * `Render` 内部：
    1. `AddClearPass`（保持）
    2. **新增**: `if (resources.Shadows != nullptr && resources.Shadows->HasDirectionalShadow()) { AddShadowDepthPass(m_Graph, frame, scene, resources, *resources.Shadows, *resources.PipelineStates); }`
    3. `AddForwardOpaquePass`（保持；只是 shader 多了 shadow 采样）
    4. `AddPresentPass`（保持）
  * 去掉 "TODO Stage 8B/8C/8D: add lighting and shadow passes" 注释
* **依赖**: `ShadowDepthPass.h`

### 3.22 `Engine/Source/Runtime/Renderer/Private/Pipeline/RenderFrameContext.h`

* **状态**: 现有
* **变更**:
  * 新增字段：
    ```cpp
    float CameraNear  = 0.1f;
    float CameraFar   = 1000.0f;
    ```
  * 这两个值与 `RenderView.NearPlane / FarPlane` / `CameraComponent.NearPlane / FarPlane` 对应
* **依赖**: 无

### 3.23 `Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp`

* **状态**: 现有
* **变更**:
  * 持有 `std::unique_ptr<RenderShadowManager> m_Shadows;`
  * `OnCreate`：
    * 创建 `m_Shadows = std::make_unique<RenderShadowManager>(); m_Shadows->Initialize(*device);`
    * 把 `impl.Resources.Shadows = m_Shadows.get();`
  * `OnDestroy` / `Shutdown`：先 `m_Shadows->Shutdown(device); m_Shadows.reset();`
  * `Render(deltaTime)`：
    1. 计算 `frame.CameraNear / frame.CameraFar`
    2. `m_Shadows->PrepareFrame(*device, SceneData, frame, m_ActiveSettings.Shadows, DebugSettings.Shadows);`
    3. 取出 `GPUShadowData` 与 `RenderShadowFrameData`
    4. `Resources.FrameResources->Update(frame, SceneData, shadowGPU, m_Shadows->GetFrameData(), DebugSettings.Shadows);`
    5. `ActivePipeline->Render(frame, SceneData, Resources);`
  * 新增 getter：`RendererSettings& GetSettings(); const RendererSettings& GetSettings() const;` 方便 Editor 改
  * 持 `m_ActiveSettings`（`RendererSettings`）
* **依赖**: `RenderShadowManager.h`、`RendererSettings.h`

### 3.24 `Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderSystem.h`

* **状态**: 现有
* **变更**:
  * 新增 `RendererSettings& GetSettings(); const RendererSettings& GetSettings() const;`
  * 内部 Impl 持 `RendererSettings m_Settings;`
* **依赖**: `RendererSettings.h`

### 3.25 Shader 侧

#### `Engine/Shaders/Common/Types.slang`

* **变更**:
  * 包含 `LightingTypes.slang`（已有）
  * `GPUFrameData` 增加 `GPUShadowData Shadows;` 字段
  * 务必保持与 C++ `GPUFrameData` 字段顺序一致：`Camera / Lighting / Shadows`
* **依赖**: `Lighting/ShadowTypes.slang`（新增，见下）

#### `Engine/Shaders/Lighting/ShadowTypes.slang`（**新文件**）

* **状态**: 新增
* **职责**: `GPUShadowData / GPUCascadeShadowData` 的 Slang 版
* **建议**:
  ```hlsl
  #pragma once
  static const uint MAX_SHADOW_CASCADES = 4;

  struct GPUCascadeShadowData
  {
      float4x4 LightViewProjection;
      // x = split far, y = depth bias, z = normal bias, w = 1/resolution
      float4 Params;
  };

  struct GPUShadowData
  {
      // x = enabled, y = cascade count, z = resolution, w = visualize cascades
      float4 ShadowParams;
      GPUCascadeShadowData Cascades[MAX_SHADOW_CASCADES];
  };
  ```
* **依赖**: 无

#### `Engine/Shaders/Lighting/ShadowSampling.slang`（**新文件**）

* **状态**: 新增
* **职责**: 阴影采样辅助（`SelectCascade / ComputeShadowUV / SampleDirectionalShadow / PCF3x3 / DebugCascadeColor`）
* **依赖**: `ShadowTypes.slang` / `Common/Types.slang`

#### `Engine/Shaders/Passes/ShadowDepth.slang`（**新文件**）

* **状态**: 新增
* **职责**: depth-only 顶点+片段入口；输出深度，无 color
* **建议**:
  * `VSInput / VSOutput` 与 ForwardPBR 一致
  * 顶点：把 `input.position` 变换到 world，再变换到 `g_FrameData.Shadows.Cascades[cascadeIndex].LightViewProjection`
  * push constant 携带 `cascadeIndex`（u32）以及 `model`
  * 片段：直接 `discard` 即可（深度是 SV_Depth 写入）；或用空 fragment shader 也可——Vulkan 允许 fragment-less pipeline
* **依赖**: `Common/Types.slang`、`Lighting/ShadowTypes.slang`

#### `Engine/Shaders/Passes/ForwardPBR.slang`

* **变更**:
  * 包含 `Lighting/ShadowTypes.slang` 与 `Lighting/ShadowSampling.slang`
  * 在 Set 0 之后，**新增** `[[vk::binding(1, 0)]] Texture2DArray g_ShadowTexture; SamplerState g_ShadowSampler;`（Vulkan 描述符可分离，Slang 用 `[[vk::binding(N)]] Texture2DArray` + `[[vk::binding(N+1)]] SamplerState`；具体 binding 取决于 frame bind group layout）
  * 实际上：当前 frame bind group layout 只有一个 `CombinedImageSampler`，所以 Slang 端要写成 `[[vk::binding(1, 0)]] Sampler2DArray g_ShadowTexture;`（combined image sampler，Texture2DArray + Sampler 一起）
  * 在 `EvaluateSceneLighting` 调用前，用 `GPULight.SpotAnglesShadow.z` 决定是否调用 `SampleDirectionalShadow`
  * `EvaluateSceneLighting` 改签名：增加 `GPUShadowData shadows, float3 worldPosition, Texture2DArray shadowTex, SamplerState shadowSampler` 参数
  * 调试：`if (g_FrameData.Shadows.ShadowParams.w > 0.5)` 时把着色器输出替换为 `DebugCascadeColor(cascadeIndex)`
* **依赖**: `ShadowTypes.slang`、`ShadowSampling.slang`

#### `Engine/Shaders/Passes/DepthOnly.slang`

* **变更**:
  * Stage 9 不直接使用 `DepthOnly.slang`（因为新增的 `ShadowDepth.slang` 就是 depth-only pass）；但建议把 `ShadowDepth.slang` 里的最小片段代码搬过来作为 `DepthOnly.slang` 的最小实现（保持一个"通用 depth-only shader"文件存在），便于未来 DepthPrePass 复用
* **依赖**: `Common/Types.slang`

### 3.26 `Engine/Source/Editor/Private/Panels/RendererDebugPanel.cpp`

* **状态**: 半实现
* **变更**:
  * 取消 "Shader debug output is stored here; renderer wiring for these modes is deferred..." 注释
  * 接收 `RenderSystem*` 而非只有 `RendererDebugSettings*`，新增 section "Shadows"：
    * 显示当前 `RendererSettings.Shadows.Directional`（Enabled / CascadeCount / Resolution / FilterMode / SplitLambda / DepthBias）
    * 调试选项：`VisualizeCascades / FreezeShadowMatrices / ShowShadowMap / DebugCascadeLayer`（已有 `VisualizeCascades/FreezeShadowMatrices` 两个 checkbox，需补 `ShowShadowMap / DebugCascadeLayer`）
    * 修改 `RendererSettings` 通过 `RenderSystem::GetSettings()` 写回
  * Editor 通过 `EditorContext.RendererDebug`（`RendererDebugSettings*`）和 `EditorContext.RenderSystem`（新增）访问
* **依赖**: `RenderSystem.h`

### 3.27 `Engine/Source/Editor/Public/XEngine/Editor/EditorContext.h`

* **状态**: 现有
* **变更**:
  * 新增 `class RenderSystem* RenderSystem = nullptr;` 字段（让 EditorContext 能访问）
* **依赖**: 前向声明

### 3.28 `Engine/Source/Editor/Private/EditorSystem.cpp`

* **状态**: 现有
* **变更**:
  * 在 `OnCreate` 中把 `RenderSystem*` 写入 `EditorContext.RenderSystem`（如果 EditorContext 是 EditorSystem 的成员）
* **依赖**: 无

---

## 4. Implementation Order（实施步骤）

每一步给出：文件、目标、写什么、如何验证、常见错误。

### Step 1: 数据 / 设置骨架（已完成，仅做命名一致性检查）

* **文件**: `RenderShadowType.h / DirectionalShadowPlanner.h / RendererSettings.h / RendererDebugSettings.h / GPUShadowTypes.h`
* **目标**: 确认所有 struct 与 Project_Cache / prompts 期望命名一致
* **写什么**: 不需要写新文件；只做 sanity check
* **如何验证**: 对照 §1.1 表
* **常见错误**: 漏掉 `static_assert(std::is_standard_layout_v<...>)`

### Step 2: Scene 侧 shadow flag 确认

* **文件**: `LightComponent.h / MeshRendererComponent.h / RenderScene.h`
* **目标**: 确认 shadow 字段已就位
* **写什么**: 无（已就位）
* **如何验证**: grep `CastShadow` 在 Scene 与 Renderer 都有
* **常见错误**: 误把 CSM 参数（cascade count、resolution）放进 `LightComponent`

### Step 3: `DirectionalShadowPlanner` 主循环补全

* **文件**: `DirectionalShadowPlanner.cpp`
* **目标**: 修 `ComputeCascadeSplits` 没写 `outSplits` 的 bug；完成主循环
* **写什么**: 见 §3.3
* **如何验证**: 单测 / 写一个 sandbox 调用 `BuildPlan` 打印 cascades
* **常见错误**: 用反 Z 时 ortho 深度范围没扩展；LightView 用 forward 而不是 `DirectionToLight`；Y-up vs Y-down

### Step 4: RHI 扩展（**关键**）

* **文件**:
  * `RHITypes.h`（扩展 `RHIRenderOutputDesc`）
  * `RHIPipeline.h`（扩展 `RHIGraphicsPipelineDesc`）
  * `RHICommandList.h`（确保支持 `ColorTarget=nullptr`）
  * `RHIDevice.h`（加 `CreateTextureView`、`UpdateBindGroupSampledTexture`）
  * 新增 `RHITextureView.h`
  * `VulkanDevice.cpp`（实现上述）
  * 新增 `VulkanTextureView.h/.cpp`
  * 修补 `VulkanTexture.cpp` 中 `GetImageViewType` 缺 `Texture2DArray` 分支
  * 修补 `VulkanPipeline.cpp` 中 `colorAttachmentCount = 0` 支持
* **目标**: 让 RHI 具备"per-layer depth view + whole-array sampled view + depth-only pipeline"三个能力
* **如何验证**: 写一个最小测试 sandbox：创建 `Texture2DArray(D32, 4 layers)`，创建 sampled view + 4 个 layer view，验证 Vulkan layer 正确
* **常见错误**:
  * `VkImageViewCreateInfo` 的 `subresourceRange.aspectMask` 没设 `VK_IMAGE_ASPECT_DEPTH_BIT`
  * `VK_IMAGE_VIEW_TYPE_2D_ARRAY` 误用 `VK_IMAGE_VIEW_TYPE_2D`
  * `colorAttachmentCount = 0` 还要保留 `VkPipelineColorBlendStateCreateInfo.attachmentCount = 0`（必须 0，不能为 1）
  * `descriptor pool` 容量没增大导致后续 descriptor 分配失败

### Step 5: `ShadowResourceCache` 实现

* **文件**: `ShadowResourceCache.h / .cpp`
* **目标**: 创建并缓存 shadow Texture2DArray、views、sampler
* **写什么**: 见 §3.5
* **如何验证**: 调用 `GetOrCreateDirectionalShadowResources` 两次（不同 resolution），观察资源被重建
* **常见错误**: 不释放旧 `shared_ptr` 导致旧 VkImage / VkImageView 泄漏；Texture2DArray 的 `ArrayLayers` 没正确传给 `RHITextureDesc`

### Step 6: `RenderFrameResources` Set 0 扩展

* **文件**: `RenderFrameResources.h / .cpp`
* **目标**: frame bind group layout 包含 shadow binding；`Update` 上传 `GPUShadowData`；每帧 `UpdateBindGroupSampledTexture`
* **写什么**: 见 §3.11
* **如何验证**: 启动 sandbox，validation layer 不报 descriptor 错
* **常见错误**: layout 增字段时没把 `PipelineLayoutVersion` 加 1，缓存里仍是旧 pipeline

### Step 7: `ShadowDepth` shader / pipeline

* **文件**:
  * `Engine/Shaders/Passes/ShadowDepth.slang`（新增）
  * `Engine/Shaders/Lighting/ShadowTypes.slang`（新增）
  * `Engine/Shaders/Common/Types.slang`（补 `Shadows` 字段）
  * `RenderPipelineStateCache.cpp`（新增 `PassKind::ShadowDepth` 分支，shader 走 `ShadowDepth.slang`）
  * `RenderPipelineStateCache.h`（无变化，`PassKind` 已枚举）
  * `GraphicsPipelineStateKey.h`（`PipelineLayoutVersion` 改 2）
* **目标**: 写完 shadow pass shader；让 pipeline cache 能产出 `ShadowDepth` 流水线
* **如何验证**: 在 sandbox 跑；用 RenderDoc 看 cascade 0 深度纹理被写入
* **常见错误**: shadow pipeline 仍然有 `colorAttachmentCount = 1`，导致 descriptor 不匹配

### Step 8: 单 cascade shadow pass（CascadeCount=1 路径）

* **文件**:
  * `ShadowDepthPass.h / .cpp`（新增）
  * `RenderFrameContext.h`（加 `CameraNear / CameraFar`）
  * `RenderSystem.cpp`（计算 near/far 并填）
* **目标**: 只渲染 1 个 cascade（用 `CascadeCount = 1` 配置），把深度写入 shadow texture 的 layer 0
* **写什么**: 见 §3.8 / §3.9
* **如何验证**: RenderDoc 看 depth attachment 写入正确；Vulkan validation layer 不报错
* **常见错误**: shadow pass 仍然期望 `ColorTarget != nullptr`；`DepthTarget` 仍用 `RHITexture*` 而非 `RHITextureView*`

### Step 9: Forward 采样 cascade 0

* **文件**:
  * `ForwardPBR.slang`（声明 `Sampler2DArray g_ShadowTexture`、调用 `SampleDirectionalShadow`）
  * `ForwardOpaquePass.cpp`（无需大改；frame bind group 已绑 shadow 资源）
* **目标**: 验证 PBR 着色能正确读 shadow texture layer 0
* **写什么**: 临时把光强与 ambient 都关掉，只保留 directional + shadow
* **如何验证**: sandbox `ValidationScene` 中 `DamagedHelmet` 在光下方有正确的 shadow
* **常见错误**: `Sampler2DArray` 用法错（Slang 中是 `g_ShadowTexture.SampleLevel(s, uv3, 0)` 而不是 `g_ShadowTexture.Sample(s, uv3)`）

### Step 10: `DirectionalShadowPlanner` 多 cascade 路径

* **文件**: `DirectionalShadowPlanner.cpp`
* **目标**: `CascadeCount > 1` 路径下产 N 套 LightView/LightProjection
* **写什么**: 在 Step 3 基础上加 `CascadeCount` 循环
* **如何验证**: 写测试打印 splits 与 `LightViewProjection` 矩阵
* **常见错误**: 上一级 split 漏做；`previousSplit` 没更新

### Step 11: 多 cascade 渲染

* **文件**: `ShadowDepthPass.cpp`
* **目标**: 循环渲染所有 cascade，每次切换 `DepthTarget`（不同 `RHITextureView*`）与 viewport
* **写什么**: 遍历 0..CascadeCount-1，每次 `SetRenderOutput` 用不同 `LayerDepthViews[i]`
* **如何验证**: RenderDoc 看 layer 0/1/2/3 都有合理深度
* **常见错误**: 没切换 viewport；layer view 错位

### Step 12: Shader cascade 选择

* **文件**:
  * `Engine/Shaders/Lighting/ShadowSampling.slang`（新增 `SelectCascade / ComputeShadowUV / SampleDirectionalShadow / PCF3x3 / DebugCascadeColor`）
  * `ForwardPBR.slang`（调用上述）
* **目标**: shader 端根据 view-space depth 选 cascade，PCF 3x3 采样
* **写什么**: 见 §3.25
* **如何验证**: visual 对照——cascades 边界处不应有硬切；`VisualizeCascades` 模式输出 cascade 颜色
* **常见错误**: 选 cascade 的 split 与 CPU 端不同步（必须都用 `Cascades[i].Params.x` 作为 split far）

### Step 13: PCF / bias / debug 控件

* **文件**:
  * `ForwardPBR.slang`（PCF 3x3 / 5x5 / Hard 三档由 `g_FrameData.Shadows` 的 flag 决定；用 `FilterMode` 推一个 shader define 或 runtime branch）
  * `RendererDebugPanel.cpp`（暴露 `VisualizeCascades / FreezeShadowMatrices / ShowShadowMap / DebugCascadeLayer`）
  * `RenderSystem.cpp`（在 `PrepareFrame` 之前把 `RendererSettings.Shadows` 与 `RendererDebugSettings.Shadows` 传给 `RenderShadowManager`）
* **目标**: 把所有 UI 控件接到实际行为
* **写什么**: 详见 §3.26
* **如何验证**: editor 改 `CascadeCount`、`Resolution`、`DepthBias`，sandwich 立刻生效
* **常见错误**: PCF shader 写错偏移；bias 单位不对（`DepthBias` 是光空间 depth offset，应在线性 light space 算）

### Step 14: 收尾与 Stage 11 准备

* **文件**: 全局 sanity
* **目标**:
  * `ComputeCascadeSplits` 的 split 结果真的写进 `outSplits`
  * `RenderFrameContext.CameraNear / CameraFar` 在所有路径都正确填充（RenderView、CameraComponent、Fallback）
  * `RenderFrameResources::Update` 的 placeholder texture 当 shadow disabled 时不污染 descriptor
  * 删除所有 `// TODO Stage 9` 注释
  * 在 `Assets/Scenes/ShadowValidation.xscene` 中放 1 盏方向光 + 1 个 cube + 1 个 plane，验证 shadow 投射
* **如何验证**: 完整跑 `Sandbox` / `Editor`；对照 §6 checklist

---

## 5. Integration Points（集成点）

### 5.1 RenderSystem ↔ RenderShadowManager

* `RenderSystem::OnCreate` 创建 `RenderShadowManager`；`Shutdown` 时 `device->WaitIdle()` 之后释放
* `RenderSystem::Render(deltaTime)` 顺序：
  1. `m_Shadows->PrepareFrame(*device, SceneData, frame, m_Settings.Shadows, DebugSettings.Shadows)`
  2. `Resources.FrameResources->Update(frame, SceneData, m_Shadows->GetShadowGPUData(), m_Shadows->GetFrameData(), DebugSettings.Shadows)`
  3. `ActivePipeline->Render(frame, SceneData, Resources)`

### 5.2 ForwardRenderPipeline ↔ ShadowDepthPass

* `ForwardRenderPipeline::Render`：
  ```cpp
  if (resources.Shadows != nullptr && resources.Shadows->HasDirectionalShadow())
  {
      AddShadowDepthPass(m_Graph, frame, scene, resources, *resources.Shadows, *resources.PipelineStates);
  }
  AddForwardOpaquePass(m_Graph, frame, scene, resources);
  ```

### 5.3 RenderFrameResources ↔ ShadowResourceCache

* `RenderFrameResources::Update` 内部调用 `UpdateBindGroupSampledTexture`：
  * `binding 1`：`shadowData.Directional.Enabled ? shadowData.Directional.ShadowTexture : null` + sampler
  * V0 简化：若 `Enabled == false`，仍然绑定一个空 placeholder（避免 descriptor 不完整）。可以使用 `ShadowResourceCache::GetOrCreateDirectionalShadowResources(...,{Resolution=1, CascadeCount=1})` 兜底

### 5.4 RenderResourceContext

* 增加 `RenderShadowManager* Shadows` 字段；**不**强制 `IsValid()` 要求
* `ForwardRenderPipeline` / `ShadowDepthPass` 显式 nullptr check

### 5.5 RenderPipelineStateCache

* `PassKind::ShadowDepth` 走专属分支：
  * 顶点+片段 shader = `Passes/ShadowDepth.slang`
  * `ColorFormat = RHIFormat::Undefined`
  * `HasColorAttachment = false`
  * `DepthFormat = RHIFormat::D32Float`
  * `EnableDepthTest/Write = true`
  * `BindGroupLayouts = { m_FrameResources->GetFrameBindGroupLayout() }`（只 Set 0；shadow pass 不需要 material）
  * `PushConstantSize = sizeof(ShadowDepthPushConstants)`
  * `PipelineLayoutVersion = 2`

### 5.6 ForwardOpaquePass

* 不需要新代码：frame bind group 已经包含 shadow 资源；shader 通过 `g_FrameData.Shadows` 读
* 但**确保**在 `RenderFrameResources::Update` 之前不要让任何 pass 用旧 bind group

### 5.7 Editor `RendererDebugPanel`

* `EditorContext.RenderSystem` 取出 `RendererSettings& settings = renderSystem.GetSettings();`
* UI 修改 `settings.Shadows.Directional.{CascadeCount / Resolution / FilterMode / SplitLambda / DepthBias / StabilizeCascades / Enabled}`
* `RendererDebugSettings.Shadows.{VisualizeCascades / FreezeShadowMatrices / ShowShadowMap / DebugCascadeLayer}` 已在；新增两个 slider/input

---

## 6. Validation Checklist（验证清单）

构建与基础：

* [ ] `CMake` 重新生成后无新警告
* [ ] Sandbox 与 Editor 都能启动
* [ ] Vulkan validation layer 不报 `VUID-VkImageViewCreateInfo-*` 错

模块边界：

* [ ] `Scene` 仍不依赖 `Renderer/RHI`（grep `Renderer/RHI` in `Engine/Source/Runtime/Scene`）
* [ ] `Asset` 仍不依赖 `Renderer/RHI`
* [ ] `RHI` 仍不包含 `csm / cascade / shadow` 等关键词（`grep -r csm Engine/Source/Runtime/RHI` 应为空）
* [ ] `LightComponent` 不含 cascade count / resolution / depth bias 字段

功能正确性：

* [ ] 现有 PBR（无 shadow）路径仍工作
* [ ] `CascadeCount = 1` + `Enabled = true` 路径：场景里有 shadow 投射
* [ ] `CascadeCount = 4` + `Enabled = true` 路径：4 个 cascade 都写满
* [ ] RenderDoc dump：shadow texture array layer 0/1/2/3 都有非空数据
* [ ] `ForwardPBR` 着色器读取的 shadow layer 与 `SelectCascade` 一致
* [ ] `VisualizeCascades = true`：场景输出按 cascade 索引分色
* [ ] `FreezeShadowMatrices = true`：移动摄像机/光，shadow 仍保持
* [ ] `SplitLambda = 0`：纯线性 split；`SplitLambda = 1`：纯对数 split
* [ ] `StabilizeCascades = true`：camera 移动时 shadow 不抖动

资源生命周期：

* [ ] 修改 `Resolution` 触发 `ShadowResourceCache` 重建
* [ ] 修改 `CascadeCount` 触发 `ShadowResourceCache` 重建
* [ ] Editor 关闭后 `device->WaitIdle` 后所有 `RHITexture/RHITextureView/RHISampler` 都 release
* [ ] frame-in-flight 多帧下没有 descriptor 复用导致的 use-after-free

性能：

* [ ] `CascadeCount = 1` 与 `Enabled = false` 时无 shadow pass 注册到 graph
* [ ] `FreezeShadowMatrices` 开启时 `RenderShadowManager::PrepareFrame` 早退（看 log）

---

## 7. Things Not to Implement in Stage 9（明确不做）

按提示要求，下述功能**不**在 Stage 9 范围内：

* RenderFeature framework
* RenderGraph V1（资源声明 + 自动 pass 排序 + barrier 推断）
* Shadow atlas（同一张 texture 内放多个光）——本阶段只支持 1 盏方向光
* Spot shadow
* Point shadow（含 cubemap shadow）
* VSM / EVSM
* PCSS 完整实现（仅留 enum 占位）
* Shadow map viewer（ImGui overlay 显示 shadow texture）——`ShowShadowMap` 只读 GPU 资源，UI 显示留给 Stage 10+
* Ray tracing
* Cascade blend（V0 在 cascade 边界硬切；Stage 10 再做 blend）
* 高分辨率阴影的 Hi-Z / roughness-aware sampling
* 接触硬化阴影（contact-hardening shadows）
* Subsurface / translucent shadow

---

## 8. Open Questions / Risks（已知风险与待定项）

1. **RHI 扩展量**: 引入 `RHITextureView` 抽象是一次较大的 RHI 改动；如果想最小化 Stage 9 diff，可以**只在 `RHI/Vulkan` 内部**新增 `RHITextureView`，让 RHI 公共头不增加抽象类（用 `void*` 句柄 + 描述符）。但建议不要这样做——未来 D3D12/Metal 也会需要 image view 抽象。
2. **Descriptor pool 扩容**: 每帧 `CombinedImageSampler` 描述符 1 个 + sampler 1 个（共享）= 影响不大，但需要确认 `CreateDescriptorPool` 的 1024 容量足够。
3. **`RenderFrameContext.CameraNear / CameraFar`**: 现有 `RenderSystem.cpp` 里通过 `CameraComponent.NearPlane / FarPlane` 取；需要保证 fallback 路径也填。`RenderView` 已经有 `NearPlane / FarPlane`，但 `RenderFrameContext` 当前没暴露这俩，需要补（§3.22）。
4. **光排序**: `RenderShadowManager` 只取"第一盏方向光"；如果场景里有多盏方向光有 shadow，Stage 9 默默忽略后续。Stage 10+ 引入"shadow caster 选择策略"时再处理。
5. **阴影 caster 过滤**: 当前 `ShadowDepthPass` 遍历 `renderScene.OpaqueObjects` 并检查 `object.CastShadow`；`RenderExtraction` 当前**没有**把 `MeshRendererComponent::CastShadow` 复制到 `RenderObject::CastShadow`（需要补——见 §3.22 之后；或者 Stage 9 实施时直接改 `RenderExtraction.cpp`）。
6. **shadow texture 的 image layout 过渡**: Vulkan 中 shadow texture 在被采样前需要 `VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL`；现有 `VulkanCommandList` 只处理 swapchain / color target / 单 depth。需要扩展或由 `ShadowDepthPass` 自己加 barrier。**建议在 `AddShadowDepthPass` 末尾加一个 `TransitionTextureToShaderRead(shadowTex)`**（现有 API 已经支持任意 `RHITexture*`）。
7. **`Sampler2DArray` 的 slang 写法**: 取决于 shadow binding 是 `CombinedImageSampler` 还是 `SampledTexture + Sampler` 拆分；按 V0 简化，统一用 combined，slang 端写 `Sampler2DArray`。

---

## 9. 推荐的最小实现顺序（再次总结）

按以下顺序提交 commit / patch：

1. RHI 扩展（`RHITextureView / RHIRenderOutputDesc 升级 / depth-only pipeline`）
2. Vulkan 实现（`VulkanTextureView / 修补 VulkanTexture / VulkanPipeline / VulkanDevice`）
3. `DirectionalShadowPlanner` 主循环 + `ComputeCascadeSplits` bug
4. `ShadowResourceCache` 实现
5. `RenderShadowManager` 实现
6. `RenderFrameResources` Set 0 扩展 + shadow upload
7. `RenderFrameContext` CameraNear / CameraFar + `RenderSystem` 串联
8. Shader 侧（`ShadowTypes.slang / ShadowDepth.slang / ShadowSampling.slang` + 修补 `ForwardPBR.slang / Types.slang`）
9. `ShadowDepthPass` 实现
10. `ForwardRenderPipeline` 集成
11. `RenderPipelineStateCache` 增 `PassKind::ShadowDepth` 分支
12. `RenderResourceContext` 增 `Shadows` 字段
13. `RendererDebugPanel` 暴露 shadow 设置
14. 验证 §6 checklist

---

## 10. 命名与文件路径速查

| 期望路径 | 当前实际 | 备注 |
|----------|----------|------|
| `Renderer/Public/XEngine/Renderer/RendererSettings.h` | ✅ 存在 | 命名一致 |
| `Renderer/Public/XEngine/Renderer/RendererDebugSettings.h` | ✅ 存在 | 命名一致 |
| `Renderer/Private/Shadows/RenderShadowTypes.h` | 实际是 `RenderShadowType.h`（单数） | **建议改名**为 `RenderShadowTypes.h` 以匹配期望，但保留旧名也可 |
| `Renderer/Private/Shadows/RenderShadowManager.h/.cpp` | ✅ 存在 | |
| `Renderer/Private/Shadows/DirectionalShadowPlanner.h/.cpp` | ✅ 存在 | |
| `Renderer/Private/Shadows/ShadowResourceCache.h/.cpp` | ✅ 存在 | |
| `Renderer/Private/Passes/ShadowDepthPass.h/.cpp` | header 仅有占位，cpp 不存在 | |
| `Renderer/Private/Resources/RenderGPUData.h` | 内容已合并到 `ShaderInterop/GPUShadowTypes.h` | **建议保留现有结构**（不要新建 `RenderGPUData.h`，除非想统一） |
| `Renderer/Private/Resources/RenderFrameResources.h/.cpp` | ✅ 存在 | |
| `Assets/Shaders/Passes/ShadowDepth.slang` | 实际目录是 `Engine/Shaders/Passes/ShadowDepth.slang` | **按实际引擎目录新增** |
| `Assets/Shaders/Lighting/ShadowTypes.slang` | 实际 `Engine/Shaders/Lighting/ShadowTypes.slang` | 同上 |
| `Assets/Shaders/Lighting/ShadowSampling.slang` | 实际 `Engine/Shaders/Lighting/ShadowSampling.slang` | 同上 |

---

**实施者应先阅读本计划全文，再按 §4 的顺序逐项实现。完成每一步时建议做小提交，便于 review 与回退。**
