# 05 CSM / Shadow System 审计

## 1. 审计范围

- `Runtime/Renderer/Private/Shadows/`：`RenderShadowManager.{h,cpp}`、`ShadowResourceCache.{h,cpp}`、`DirectionalShadowPlanner.{h,cpp}`、`RenderShadowType.h`
- `Runtime/Renderer/Private/Passes/ShadowDepthPass.{h,cpp}`
- `Runtime/Renderer/Private/Passes/ForwardOpaquePass.{h,cpp}`（shadow sampling 集成）
- `Runtime/Renderer/Public/XEngine/Renderer/RendererSettings.h`、`RendererDebugSettings.h`
- `Runtime/Renderer/Private/ShaderInterop/GPUShadowTypes.h`
- `Engine/Shaders/Lighting/`：`ShadowTypes.slang`、`ShadowSampling.slang`、`Lighting.slang`
- 与之相关的：`ShadowResourceCache` ↔ `RenderFrameResources` ↔ `ForwardOpaquePass` ↔ `ShadowSampling.slang` 的整条链路
- LightComponent → RenderLight extraction（参见 `RenderExtraction.cpp`）

---

## 2. 当前优点

- **职责分层清晰**：
  - `ShadowResourceCache` 只拥有 shadow resources（texture + sampled view + per-layer depth views + sampler）；
  - `DirectionalShadowPlanner` 只做纯 CSM 数学（split / basis / projection / texel-snap）；
  - `RenderShadowManager` 编排：选 light → 取/建 resource → planner → fill frame data → fill GPU data；
  - `ShadowDepthPass` 只做 GPU 命令录制。
  - **这与提示文档期望方向完全一致**。架构上对未来扩展（spot shadow / point shadow / atlas）非常友好。
- **persistent Texture2DArray（推荐模式）**：`ShadowResourceCache::GetOrCreate...` 在 shape 不变时复用同一 texture / sampled view / layer views / sampler；无 per-frame-in-flight 复制 shadow texture。这是提示文档 §3 "Stage 9 V0 only supports Texture2DArray" 的最佳实现。
- **shader interop 类型严格**：
  - `GPUCascadeShadowData { LightViewProjection, Params(splitFar, depthBias, normalBias, texelSize) }`；
  - `GPUShadowData { ShadowParams, Cascades[4] }`；
  - `static_assert(sizeof == ...)` 强制与 Slang 端 `MAX_SHADOW_CASCADES = 4` 同步。
- **Float-friendly bias 字段**：cascade 自身 bias 参数在 shader 端用 `params.y + max(0,1-NdotL)*params.z`，slope-scaled bias 已经留位。
- **freeze / visualize 调试开关已经预留**：通过 `RendererDebugSettings` + `ShadowDebugSettings` 控制。
- **handed-light basis 自洽**：`Math::BuildLightBasis` 用 `Cross(worldUp, Forward)` 得到 right, up = Cross(right, forward)（LH + Cross(Right, Up) = Forward 的约定）；光 forward 通过 `forward`（+X 旋转后）拉出，与 `DirectionToLight = -forward` 在 `RenderExtraction` 中的表达吻合。

---

## 3. 发现的问题

### 3.1 [Critical] `RenderShadowManager::PrepareDirectionalShadow` 第 124 行 `=` 而不是 `==`

- **相关文件**：[`RenderShadowManager.cpp:124`](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp)
- **问题描述**：
  ```cpp
  if (shadowLight = nullptr) { return; }
  ```
  此行把 `shadowLight` 设为 null 后立即判 true，**永远 return**。CSM 实际永远没机会准备。这是审计 CSM 部分最严重的一行 silent bug。
- **推荐修复方式**：改为 `if (shadowLight == nullptr) { return; }`。
- **建议**：P0，立刻修。

### 3.2 [Critical] `RenderFrameResources::Initialize` 中引用未声明 `shadowSampledView` / `shadowSampler`

- **相关文件**：[`RenderFrameResources.cpp:80-86`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：见 Renderer 架构审计 3.1。当前 Initialize 把命名上的 `shadowSampledView`、`shadowSampler` 传给 `RHIBindingResource`，但这两个变量在函数体内未定义。
- **为什么重要**：Set 0 binding 1/2 的 contract 必须在 Initialize 阶段就被绑定；当前是 broken。
- **推荐修复方式**：
  - Initialize 多接 `RHITextureView* shadowTex, RHISampler* shadowSamp`；
  - 或在第一次 `Update` 中写 descriptor 到 bind group（`RHIBindGroup::Update` 抽象需要先存在）；
  - RenderSystem 调用顺序调成：ShadowManager->PrepareFrame → 拿到 shadowSampled/Sampler → FrameResources->SetShadowBindings → FrameResources->Update。
- **建议**：P0，立刻修。

### 3.3 [Critical] `RenderFrameResources::BuildGPUFrameData` 没有写入 ShadowData

- **相关文件**：[`RenderFrameResources.cpp:149-160`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderFrameResources.cpp)
- **问题描述**：GPUFrameData.Shadows 在 C++ 端永远是 0 值矩阵 → shader 采样时拿到 0 矩阵 → cascade 选择 / projection 全部失败。
- **推荐修复方式**：
  - `BuildGPUFrameData` 增加 `const RenderShadowManager&` 参数；
  - `data.Shadows = GPUShadowData{}; shadowManager.FillGPUShadowData(data.Shadows);`。
- **建议**：P0-1 配套。

### 3.4 [Critical] `ShadowDepthPass` 的 pipeline 是 ForwardOpaque pipeline 取出来的

- **相关文件**：[`ShadowDepthPass.cpp:46-49`](../../Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp)
- **问题描述**：`GetOrCreateShadowDepthPipeline(RHIFormat::Undefined, RHIFormat::D32Float)` 在 `RenderPipelineStateCache::CreateGraphicsPipeline` 内部仅在 `key.PassKind != RenderPassKind::ForwardOpaque` 时返回 null。当前 key 把 PassKind 默认是 ForwardOpaque，所以这个函数实际是借 ForwardOpaque 路径取 pipeline，但 PipelineStateCache 看不到 ShadowDepth 这种 PassKind，于是要么 fallback 返回 ForwardOpaque pipeline（在 `GetOrCreateGraphicsPipeline(key)` 上），要么返回 null。
- **为什么重要**：shadow depth 必须有独立 PassKind + 独立 pipeline（depth bias 通常与 forward 不同，shader 不需要 fragment，RasterizerState 应 CullFront）。
- **推荐修复方式**：在 `GraphicsPipelineStateKey` 加 `RenderPassKind::ShadowDepth` 分支；`CreateGraphicsPipeline` 写 DepthOnly.slang vs ForwardPBR.slang 不同入口；shadow 端的 depth bias 也读 `BiasParams` 切到 pipeline depthBias。
- **建议**：P0，立刻修。

### 3.5 [Critical] `DirectionalShadowPlanner::BuildPlan` 内 ReverseZ 路径混乱

- **相关文件**：[`DirectionalShadowPlanner.cpp:308-353`](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp)
- **问题描述**：
  - `desc.ReverseZ == true` 时，`tmpProj` 第一行写 `(near=-radius-bias*4, far=radius+bias*4)`，紧接 `(near=radius+bias*4, far=-radius-bias*4)` 覆盖第二次。
  - 而 `lightProjection` 在 `if (desc.ReverseZ)` 块内又用 `(near=radius+bias*4, far=-radius-bias*4)` 写一遍。
  - 三处都有 magic 数 `bias*4`，系数应该抽常量。
- **为什么重要**：同一函数三处写相同 projection 参数，未来若有人想统一调整 reverse-Z，必须同步改；事实上当前 ReverseZ 逻辑正确（last write wins），但可读性差，且如果哪天有人 commit 改动一处，shadow 立即挂。
- **推荐修复方式**：
  ```cpp
  static Mat4 MakeCascadeProjection(float radius, float depthBias, bool reverseZ) {
      const float half = radius + depthBias * DepthBiasSlackFactor; // = 4
      return reverseZ
        ? Math::OrthographicLH_ZO(-half, half, -half, half,  half, -half)
        : Math::OrthographicLH_ZO(-half, half, -half, half, -half,  half);
  }
  ```
- **建议**：P0-1 配套。

### 3.6 [Critical] texel snap 后再写 `lightProjection`，导致 snap 与 proj 不一致

- **相关文件**：[`DirectionalShadowPlanner.cpp:307-336`](../../Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp)
- **问题描述**：
  - texel snap 路径使用 `tmpProj` 计算 `ComputeTexelSnapOffset(center, lightView, tmpProj, texelSize)`，并把 offset 加到 `lightView` 的 translation。
  - 然后 `lightView[3] -= Vec4(snap, 0.0f);`。
  - 但 `lightProjection` 在 line 339 之后**以新 lightView 重新生成**，且 `lightViewProj = lightProjection * lightView`。
  - `ComputeTexelSnapOffset` 假设 lightView + lightProj 已知；但 `tmpProj` 与最终 `lightProjection` 在 reverse-z 分支中是两套 near/far 顺序，传给 `Math::OrthographicLH_ZO` 的语义不同（near 在前、far 在后）。Snap 算法的"反向 w"行为敏感，这会让 snap offset 与真实 projection 的 NDC 偏移不一致。
- **推荐修复方式**：
  - 在 texel-snap 之前先把 lightProjection 求出来，再以 (lightView, lightProj) 计算 snap；
  - lightView 应用 snap 后不再重建 projection；
  - 把 snap 抽象为 helper，统一存到 cascade struct 中（`SnappedLightViewProjection`）。

### 3.7 [High] `ShadowResourceCache::GetOrCreate` 不通知 FrameResources 的 shadow bind group

- **相关文件**：[`ShadowResourceCache.cpp:58-90`](../../Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp)
- **问题描述**：当分辨率/cascade 数变化时 `m_Directional = {};` 释放旧资源，但 `RenderFrameResources::m_FrameBindGroups[]` 内 Set 0 binding 1 仍指旧 `SampledView`，绑定到当前 command buffer 时会 dangling（即便按同 shape 复用，view 也是新的；但如果 cascade count 改变，DescriptorSet Layout 与实际 binding 数量不一致）。
- **推荐修复方式**：
  - `ShadowResourceCache` 暴露观察者接口 `INotifyShadowResourceRecreated*`；
  - `RenderFrameResources` 实现该接口；
  - 重新绑定 Set 0 内的 sampled view / sampler；
  - 或更简单：在 `RenderFrameResources::Update` 每帧比对 shadow resource version（用 atomic counter）—— 不一致则 `RebuildBindGroups()`。

### 3.8 [High] `SceneBounds` 不参与 cascade 半径估算

- **相关文件**：[`RenderShadowManager.cpp:70-93`](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp)
- **问题描述**：
  - 当前 cascade 半径使用 frustum corners 的 bounding sphere（cascade sub-frustum 8 corners）;
  - `SceneBounds` (`DirectionalShadowPlanDesc`) 已传入但 **planner 完全没用**。
  - 当 cascade 半径仅包住 frustum 时，物体超出 frustum 的部分可能被截掉（shadow acne），即便 scene bounds 更大。
  - 这是 directional shadow 中常见的"shadow caster 被截掉"现象。
- **推荐修复方式**：
  - 在 radius 计算时 `radius = std::max(radius_from_frustum, sceneBounds_in_light_sphere_radius);`；
  - 测试场景：把 cube 放在 frustum 外侧，验证 shadow 仍投影。

### 3.9 [High] `ShadowDepthPass` 不实现 cascade 间 smooth blend boundary

- **相关文件**：`ShadowSampling.slang`、`ShadowDepthPass`
- **问题描述**：当前 shader `SelectCascadeIndex` 是 strict-boundary 选择；cascade 分界处会有硬边。
- **推荐修复方式**：写 `SmoothCascadeBlend(viewSpaceDepth, shadow)`，在 shader 端 lerp 两个 cascade 采样；CPU 端数据无需改，**仅 shader 改动**。
- **建议**：Stage 12+ 质量优化。

### 3.10 [High] light shader data 未区分 directional & spot & point 的 CompareOp

- **相关文件**：[`GPULightingTypes.h`](../../Engine/Source/Runtime/Renderer/Private/ShaderInterop/GPULightingTypes.h)、[`Lighting.slang`](../../Engine/Shaders/Lighting/Lighting.slang)
- **问题描述**：CSM 只对 directional 走 shadow sampling；当前 lighting shader 端如果 `GPULightType::Directional` 走 cascade，否则跳过。当前 ShadowData 仅在一个 light 上建模，没有 "shadow light per cascade" 思想。
- **推荐修复方式**：先接受 directional only，标 TODO Stage 13。

### 3.11 [High] ShadowDepth shader 与 C++ push constants layout 需对齐

- **相关文件**：[`RenderShaderTypes.h:25-33`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderTypes.h)、`DepthOnly.slang`
- **问题描述**：`ShadowDepthPushConstants { Mat4 Model, Mat4 LightViewProjection, u32 CascadeIndex, u32 _pad0..2 }`。shader 端 DepthOnly 需要先读：
  - `Model` 直接 transform position；
  - 之后 `lightViewProj` 是指 vp；shader 端通常用 `worldPos = mul(model, pos)`；`clip = mul(lightVP, float4(worldPos, 1))` —— 这是粗略的；但 vertex 端其实只需要 `mul(lightVP, mul(model, pos))`。
  - 理想用法：vertex 输入是 `mul(lightVP, mul(model, pos))`，但 CPU 端已经提供完整 LightViewProjection。
  - 当前 enum 看是 `LightViewProjection = cascade.LightViewProjection`；意味着 shader 端可以直接 `clipPos = mul(lightViewProjection, mul(model, pos))`。这个 layout 与 `gl_Position = mul(lightVP, mul(model, float4(pos, 1.0)))` 匹配。
- **为什么重要**：如果 shader 用 `mul(model, pos)` 然后 `mul(lightVP, worldPos)`，layout 一致；如果 shader 用 `mul(lightVP, mul(model, pos))` —— 注意 GLM 是列主序下：`M_model * vec4(pos,1)` 是 v' = M_model * v；`M_lightVP * v'` 是正确顺序。当前 C++ 端按 GLM 列主序上 texture 数据矩阵乘法，正好吻合。

### 3.12 [High] `ShadowResourceCache` 当前不支持冻结场景下 hot-reload

- **相关文件**：`ShadowResourceCache.cpp`
- **问题描述**：当 `RendererSettings.Directional.Resolution` 改变（用户改），cascade 重绑 OK；但如果 sampler / borderColor 改变（当前不能改），不会重置。
- **建议**：标 Stage 13。

### 3.13 [Medium] `ComputeShadowCasterBounds` fallback 是固定 100m 立方体

- **相关文件**：[`RenderShadowManager.cpp:88-91`](../../Engine/Source/Renderer/Private/Shadows/RenderShadowManager.cpp)
- **问题描述**：当 `scene.OpaqueObjects` 中没有 `CastShadow` object 时，shadow bounds fallback 是 (-50, -50, -50) - (50, 50, 50)，这意味着 cascade 会建模一个 100m 的固定空间，与 scene 真实尺寸无关。
- **推荐修复方式**：fallback 用 `computeCameraBoundingFrustum(...)` 或基于 camera position + farPlane 推一个合理默认。
- **建议**：Stage 12 改进。

### 3.14 [Medium] `DirectionalShadowPlanner::ComputeCascadeSplits` 在 CPU 端重复计算 camera frustum

- **相关文件**：[`DirectionalShadowPlanner.cpp:51-107`](../../Engine/Source/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp)
- **问题描述**：每帧 `Math::Inverse(cameraProjection * cameraView)`；当 camera 未动时该矩阵不会变。增加 scene 级别 dirty flag 可避免重复 inversion。
- **推荐修复方式**：缓存 `Mat4 InverseViewProj` 与 camera signature (pos, rotation, proj params) 的 hash；signature 不变则复用。
- **建议**：Stage 14 优化。

### 3.15 [Medium] `GPUShadowData::ShadowParams.w` 当前没用

- **相关文件**：[`GPUShadowTypes.h:23-33`](../../Engine/Source/Renderer/Private/ShaderInterop/GPUShadowTypes.h)、`ShadowSampling.slang:18-23`
- **问题描述**：`ShadowParams.w = visualize cascades flag` 预留。当前 `RenderShadowManager::FillGPUShadowData` 第 4 个分量写 0 并注释 "visualize cascades is set by the caller (RenderFrameResources) from debug settings"。
- **推荐修复方式**：在 `BuildGPUFrameData` 接受 `RendererDebugSettings` 参数，把 visualize 标志写入第 4 个分量；shader 接到后用 cascade color 替换 directLight。

### 3.16 [Medium] `ShadowDepthPass` 无 Submesh prebatch

- **相关文件**：[`ShadowDepthPass.cpp:91-126`](../../Engine/Source/Renderer/Private/Passes/ShadowDepthPass.cpp)
- **问题描述**：当前为每个 object 设置一次 push constants / vertex / index binding，再分 submesh draw —— 当子 mesh 不止一个 material 时这会引起过多 state changes。
- **推荐修复方式**：先 batch by Material，然后 batch by Submesh range。Shader 仍用 push constant。
- **建议**：Stage 14 优化。

### 3.17 [Medium] `ShadowDepthPushConstants` 中 `_pad0..2` 是手写对齐

- **相关文件**：[`RenderShaderTypes.h:25-33`](../../Engine/Source/Renderer/Private/Resources/RenderShaderTypes.h)
- **问题描述**：手动 padding 在 shader reflection 中可能引起 push constant range 错位（GPU pipeline layout 计算可能要求 16 字节边界）。
- **推荐修复方式**：使用 `vec4 CascadeIndex_pad` 整体 16 bytes，或者把 `_pad0..2` 用 `float` 或 `uint32_t[3]` 写到一个 `Vec4.pad`。

### 3.18 [Medium] `ShadowResourceCache` 不支持 hot-reload 时主动更新 bind group

- **相关文件**：上一条
- **问题描述**：当前若 RenderSystem 重建 shadow resources 会自动 rebind（前提是帧 bind group reload），否则悬挂。
- **建议**：Stage 13。

### 3.19 [Medium] `GPULight::SpotAnglesShadow` 第三个分量已用作 shadow flag 但方向语义混乱

- **相关文件**：[`GPULightingTypes.h:32-34`](../../Engine/Source/Renderer/Private/ShaderInterop/GPULightingTypes.h)
- **问题描述**：字段定义 `cast shadow flag`，注释 OK。但当前 CSM 仅对第一个 directional light 工作，CPU 端对所有 light 都填这个 flag，GPU shader 端需要正确判断。
- **建议**：标 Stage 13。

### 3.20 [Low] `ShadowDepthPass.cpp:43` 注释 "Optional: declare depth-attachment access for the future RenderGraph resource tracker."

- **相关文件**：[`ShadowDepthPass.cpp:60-69`](../../Engine/Source/Runtime/Renderer/Private/Passes/ShadowDepthPass.cpp)
- **问题描述**：现在 `RenderGraphBuilder` 阶段 `setup` lambda 接住 `depthView` 但 `(void)depthView;` 忽略；同时 `(void)desc.DepthAttachment;`。意味着 RenderGraph 当前并不知道 shadow depth attachment 存在。
- **推荐修复方式**：保留 stub，等 RenderGraph V1 接入。
- **建议**：RenderGraph V1 前修。

### 3.21 [Low] `ShadowFilterMode` 枚举与 shader filterMode 不一致

- **相关文件**：[`RendererSettings.h:19-25`](../../Engine/Source/Renderer/Public/XEngine/Renderer/RendererSettings.h)、[`ShadowSampling.slang:114-122`](../../Engine/Shaders/Lighting/ShadowSampling.slang)
- **问题描述**：
  - `ShadowFilterMode::Hard = 0`, `PCF3x3 = 1`, `PCF5x5 = 2`, `PCSS = 3`；
  - shader 端 `filterMode == 0` 走 Hard，`else` 走 PCF3x3；
  - PCF5x5 / PCSS 还没实现，但 settings 已暴露。
- **推荐修复方式**：先实现 PCF5x5（或合并为 PCF3x3/PCF5x5 选择），再上 PCSS。
- **建议**：Stage 11 PBR Test 期间修。

### 3.22 [Low] `RenderShadowCascade::BiasParams` 仅 `x/y` 有用，z=slope-scaled, w=reserved

- **相关文件**：[`RenderShadowType.h:30-31`](../../Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h)
- **问题描述**：z 与 w 留位，shader 端 `params.z` 用作 normal-bias-slope。CPU 端在 `DirectionalShadowPlanner.cpp` 用 `Vec4(depthBias, normalBias, 0, 0)` 写 —— 没有把 slope-scaled 从 settings 传到 z。
- **推荐修复方式**：扩 `DirectionalShadowSettings { float SlopeScaledBiasFactor = 0.5f; }`，CPU 端写入 z，shader 已能识别。

---

## 4. 架构边界问题

- **`RenderShadowManager::FillGPUShadowData` 产出的是 GPUShadowData（Renderer 私有），不是 RHI 资源** —— 正确，对外只暴露 shader interop 类型。
- **`DirectionalShadowPlanner` 不知道 RHI / Vulkan** —— 正确。但 `BuildLightBasis` 用了 GLM 的 cross，OK。
- **`ShadowResourceCache` 仅用 RHI 公共类型** —— 正确。
- **`ShadowDepthPass` 只 record command，不创建资源** —— 正确，符合 Pass 设计原则。
- **`RendererDebugSettings` 中 `FreezeShadowMatrices` 与 `ShadowDebugSettings::FreezeShadowMatrices` 重复** —— 见 Renderer 审计 3.23。

---

## 5. 性能 / 生命周期 / 同步问题

- **生命周期**：ShadowResourceCache 重建 + frame bind group 重置未联通。`ShadowDepthPass` 在 cascade0 写到 cascade[1] buffer 时是依赖同 commandbuffer 内的顺序；命令之间无显式 barrier，但 RenderPass 自身提供 dependency 链。
- **同步**：ShadowDepth 不需要等外部信号；ForwardOpaque 之后 shadow texture sampling 在同一 command buffer 内完成。
- **每帧**：CPU 端 `BuildPlan` 调用 `Math::Inverse(cameraProjection * cameraView)` 一次 + 4 次 `Math::Inverse(lightView)` —— GLM inverse 不是一个超轻量操作；适合加 camera change cache。
- **persistent**：`ShadowResourceCache::Texture / View / Sampler` 都是 long-lived，只在 size 改变时重建；这是正向设计。
- **freeze**：m_FrozenFrameData 是深拷贝，安全；但 GPU 资源 pointer 仍指向 live，所以重建后失效。

---

## 6. 坐标 / 数学问题

- **+X Forward**：Directional light forward = rotated +X；DirectionToLight = -forward。`BuildLightBasis` 假设 `forward = directionToLight`（即 shader 朝光方向），cross 顺序是 `Cross(worldUp, forward) = right`，`Cross(right, forward) = up` —— 这与 +X forward LH 的右手系叉积在 GLM `glm::cross(a,b) = a × b` 语义下吻合。
- **ReverseZ**：`desc.ReverseZ = true` 写 `tmpProj` 顺序 `(radius+bias*4, -radius-bias*4)`（near > far），随后覆盖；最终 `lightProjection` 用 `(radius+bias*4, -radius-bias*4)` —— 与 `shadow.ShadowParams` 第 4 个分量对 lighting 端 `>(biasedRef)` 配套 OK，前提是 shader 端 `SampleShadow1Tap` 与 `SampleShadowPCF3x3` 都用 `>=`。
- **Left-Hand**：cascade projection 用 `Math::OrthographicLH_ZO`；shader 端 `mul(lightViewProj, worldPos)` 在 GLM column-major 下与 LH 引擎一致。这是正确的。
- **Vulkan Y flip**：`ClipToShadowUV.y * -0.5 + 0.5`；这是 Vulkan 专属。RHIClipSpace 已经管理。

---

## 7. 推荐修改

- 现在就修：
  - **3.1** `=` → `==`；
  - **3.2 / 3.3** shadow bind group 与 GPUFrameData::Shadows 写入；
  - **3.4** ShadowDepth PassKind 独立；
  - **3.5 / 3.6** planner ReverseZ + texel snap 一致性。
- Stage 10 DebugDraw 期间修：
  - **3.7** ShadowResourceCache resource-recreated 通知；
  - **3.15** `ShadowParams.w = visualize cascades` 接线；
  - **3.22** `SlopeScaledBiasFactor` 引入；
  - **3.21** PCF5x5 切到 shader；
- RenderGraph V1 前修：
  - **3.8** SceneBounds 参与 cascade 半径；
  - **3.20** RenderGraph Builder 显式声明 depth-attachment；
- 长期清理：
  - **3.9** cascade blend smooth；
  - **3.14** camera inverse 缓存；
  - **3.16 / 3.17** Submesh batching + push constant 自然对齐；
  - **3.10** 多 light shadow 抽象。

---

## 8. 可拆给 Claude Code 的具体任务

1. 在 `RenderShadowManager.cpp:124` 把 `if (shadowLight = nullptr)` 改为 `if (shadowLight == nullptr)`；**只单行修改**。同时为 `shadowLight = nullptr` 写一行 `static_assert(!std::is_assignable<...>)` 防御。
2. 在 `RenderFrameResources::Initialize` 改为接 `RHITextureView* shadowSampledView, RHISampler* shadowSampler`；`RenderSystem::Render` 顺序：
   ```cpp
   ShadowManager->PrepareFrame(...);
   FrameResources->SetShadowBindings(ShadowManager->GetFrameData().Directional.SampledView, ShadowManager->GetFrameData().Directional.Sampler);
   FrameResources->Update(frame, *ShadowManager, scene);
   ```
   **保持外部 behavior 一致，仅补缺失 wiring**。
3. 在 `BuildGPUFrameData` 加 `const RenderShadowManager&` 参数并 fill `data.Shadows`。
4. 在 `GraphicsPipelineStateKey` 与 `CreateGraphicsPipeline` 中加 `RenderPassKind::ShadowDepth` 与对应 `RenderShaderKey("DepthOnly.slang")` + 深度专属 descriptor（HasColorAttachment=false, EnableDepthTest=true, EnableDepthWrite=true, EnableDepthBias=true）。**仅添加分支，不删既有 PassKind**。
5. 在 `DirectionalShadowPlanner.cpp` 抽 helper `static Mat4 MakeCascadeProjection(radius, depthBias, reverseZ)`，并在 BuildPlan 内 `lightProjection = MakeCascadeProjection(radius, desc.DepthBias*4.0f, desc.ReverseZ);` — texel-snap 用同一 helper 计算 proj。
