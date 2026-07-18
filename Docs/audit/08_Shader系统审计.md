# 08 Shader System 审计

## 1. 审计范围

- `Runtime/Shader/Public/XEngine/Shader/`：`ShaderSystem.h`、`ShaderCompiler.h`、`ShaderReflection.h`、`ShaderTypes.h`、`ShaderModule.h`
- `Runtime/Shader/Private/`：`ShaderSystem.cpp`、`ShaderCompiler.cpp`、`ShaderCache.cpp`、`Slang/SlangCompiler.{h,cpp}`、`SlangReflection.cpp`
- `Engine/Shaders/`：`Common/*.slang`、`Lighting/*.slang`、`Materials/*.slang`、`Passes/*.slang`
- C++ ↔ Slang 共享的 interop 类型：`GPUFrameTypes.h`、`GPULightingTypes.h`、`GPUShadowTypes.h`、`GPUFrameTypes.h`、`RenderShaderTypes.h`

---

## 2. 当前优点

- **Slang 一致**：所有 shader 使用 Slang 语法 → 编译到 SPIR-V (Vulkan)。Slang module + entry point 设计成熟。
- **公共 interop 类型严格对齐**：`GPUFrameData`、`GPULightingData`、`GPUShadowData`、`GPUCascadeShadowData` 在 C++ (`*.h`) 与 Slang (`Types.slang`, `LightingTypes.slang`, `ShadowTypes.slang`) 都有镜像 struct，用 `alignas(16)` + `static_assert` 强制大小。
- **Push constants 类型独立管理**：`RenderShaderTypes.h` 集中所有 push constant (`PBRPushConstants`, `MeshPushConstants`, `ShadowDepthPushConstants`) 与 sizeof/offset 静态断言。这是 stage 9 调试利器。
- **shader include 链浅**：`Common/Math.slang` → `Common/Types.slang` → `Lighting/LightingTypes.slang` → `Lighting/ShadowTypes.slang` → `Lighting/ShadowSampling.slang`；层级清楚。
- **`#pragma once`** + 命名清晰：`#pragma once` 在所有 .slang 头使用；modular block `[[vk::binding(set, bind)]]` 显式可见。
- **每 pass shader 独立**：DepthOnly.slang / ForwardPBR.slang / Triangle.slang / UnlitTextured.slang 各自分离，便于 cache & recompile 控制。
- **Stage 9 CSM shader 已实现**：`ShadowSampling.slang` 提供 `SampleShadow1Tap` / `SampleShadowPCF3x3` / `ComputeShadowFactor` —— 9-tap manual PCF。

---

## 3. 发现的问题

### 3.1 [Critical] `ShadowSampling.slang:62 / 94` manual PCF；硬件 PCF 与 border color 未启用

- **相关文件**：[`Engine/Shaders/Lighting/ShadowSampling.slang`](../../Engine/Shaders/Lighting/ShadowSampling.slang)、[`ShadowResourceCache.cpp:156-162`](../../Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp)
- **问题描述**：
  - 当前 `g_ShadowSampler` 是普通 comparison-less sampler（`VulkanSampler.cpp:38-41` 硬编码），故必须手动 PCF 比较 shadowMapDepth 与 refDepth；
  - borderColor 是 `VK_BORDER_COLOR_INT_OPAQUE_BLACK` —— 对 shadow filter 的 clamp border 行为是 "深度读为 0"，但在 saturation 过的 `saturate(uv)` 后，已经 clamp；border 极少触发；但 PCF 多个 sample 在 texel 边界做 `saturate(uv + offset)` 是为避开 border。
- **推荐修复方式**：
  - 加 `RHISamplerDesc::CompareEnable/CompareOp/BorderColor`；
  - `VulkanSampler` 翻译到 `compareEnable = VK_TRUE`、`compareOp = VK_COMPARE_OP_LESS_OR_EQUAL`、`borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE`；
  - shader 用 `g_ShadowMap.SampleCmp(g_ShadowSamplerCompare, float3(uv, layer), refDepth)` 一行硬件 PCF。
- **建议**：P0-1，下一批 CSM 修。

### 3.2 [Critical] C++ 与 Slang 字段名不一致：`GPUFrameData.Shadows` vs `Shadow`

- **相关文件**：[`Engine/Shaders/Common/Types.slang:38-44`](../../Engine/Shaders/Common/Types.slang)、[`GPUFrameTypes.h:26-32`](../../Engine/Source/Renderer/Private/ShaderInterop/GPUFrameTypes.h)
- **问题描述**：shader 端字段是 `Shadow`，C++ 端是 `Shadows`。虽然 vec 类型一致，Reflection-based pipeline layout 推导会因字段名 mismatch 出错（如果 Slang reflection 用 names 做 cross-validation）。
- **推荐修复方式**：把 C++ 改成 `Shadow`，与 Slang 同名；保持类型 `GPUShadowData`。
- **建议**：Stage 10 之前修。

### 3.3 [High] 阴影 cascade 选择没有 smooth blend

- **相关文件**：[`ShadowSampling.slang:18-34`](../../Engine/Shaders/Lighting/ShadowSampling.slang)
- **问题描述**：`SelectCascadeIndex` 是 strict-boundary；cascade 边界可能闪烁。CPU 端 `cascade[i].Params.x = SplitFar`，相邻 cascade 区间比较。
- **推荐修复方式**：实现 `SelectCascadeBlend(viewSpaceDepth, shadow)` 返回 vec2 `(cascadeA, blend)`；CPU 数据不变。
- **建议**：Stage 12+。

### 3.4 [High] `ForwardPBR.slang:44-46` 法线计算可能不正确

- **相关文件**：[`Engine/Shaders/Passes/ForwardPBR.slang`](../../Engine/Shaders/Passes/ForwardPBR.slang)
- **问题描述**：
  - `output.normal = normalize(mul((float3x3)g_PushConstants.model, input.normal));` —— 这是 `N_object * M` 风格，等价于 `M^T * N_obj` 当输入是 column vector 时；
  - GLM 列主序下 `Mat4 * Vec4` 是 `M * v`；`Mat3(M)` 抽出 `(M^T)^(-1)` 还是 `M`？取决于 GLM 实现。Slang 端通常 `(float3x3)m` 是直接取左上角，等价于 column-major top-left 3x3。
  - 正确法线变换应该是 `(M^-T) * N_obj`；当 model 仅含 uniform scale 与 rotation 时 `(float3x3)M` 已经正交，等价于 `(M^T)` —— 因此用 `(float3x3)M * N` 实际是 `N_obj` 经过 "transposed & non-uniform-scale-incorrect" 的变换。**如果 model 含 non-uniform scale 或 negative scale，结果错**。
- **推荐修复方式**：先转 `inverse(transpose(mat3(model)))` 然后 normalize，CPU 端如果预设 uniform scale 可用 `mat3(model)` 优化。
- **建议**：Stage 11 PBR Test 修。

### 3.5 [High] `ForwardPBR.slang:66` normal map 解码后与 vertex normal 加权过小

- **相关文件**：[`ForwardPBR.slang:65-66`](../../Engine/Shaders/Passes/ForwardPBR.slang)
- **问题描述**：
  - `float3 sampledNormal = normalTexture.Sample(input.uv).xyz * 2.0 - 1.0;`
  - `float3 N = normalize(input.normal + sampledNormal * 0.0001);`
  - 0.0001 权重基本让 normal map 失效 —— 这是 placeholder（注释 "Stage 7 用了 loaded/generated tangents"）。
- **推荐修复方式**：完整 tangent-space normal mapping 接入，需要：
  - vertex shader 输出 `tangent` + `bitangent`；
  - TBN 矩阵构造；
  - normal sample 在 TBN 空间转到 world。
- **建议**：Stage 11 PBR Test。

### 3.6 [High] `ForwardPBR.slang:78` viewSpaceDepth 取 `viewPos4.x` 是 +X forward 约定

- **相关文件**：[`ForwardPBR.slang:76-77`](../../Engine/Shaders/Passes/ForwardPBR.slang)
- **问题描述**：注释说 `// XEngine: camera forward is +X in view space.`，所以取 `viewPos4.x` 作为 view-space depth。**这是对的**，与文档期望一致。
- **为什么列为问题**：这条是 "current design is correct, but heavily depends on convention understanding. Add a unit-like test or sanity check helper".
- **推荐修复方式**：在 shader 顶部加 `#define XENGINE_VIEW_SPACE_DEPTH(viewPos) (viewPos.x)`；CPU 端提供同名宏同步。
- **建议**：Stage 10 期间补，使约定显式。

### 3.7 [High] `ShadowDepthPushConstants._pad0..2` 占位但 Slang 端要求 push constant size

- **相关文件**：[`Engine/Shaders/Passes/DepthOnly.slang:3-12`](../../Engine/Shaders/Passes/DepthOnly.slang)、[`RenderShaderTypes.h:25-33`](../../Engine/Source/Runtime/Renderer/Private/Resources/RenderShaderTypes.h)
- **问题描述**：Slang 用 `ConstantBuffer<ShadowDepthPushConstants>`；padding 字段被 explicit 命名，但 SPIR-V layout computation 可能需要 16-byte alignment；如果 Vulkan push constant range size = 96B（C++ 端有 pad 占满 96），shader 要求 16-byte 边界；当前 C++ sizeof = ?
- **推荐修复方式**：把 `_pad0..2` 替换为 `float4 pad` 单字段，并 `static_assert(sizeof % 16 == 0)` 在 RenderShaderTypes.h 强制。

### 3.8 [Medium] `ShadowSampling.slang` 与 `Lighting.slang` 重复 import

- **相关文件**：`ShadowSampling.slang:1-5`, `Lighting.slang`
- **问题描述**：多次 include `Common/Math.slang` / `Common/Types.slang`，slang 应去重；但 `#pragma once` 已经有保护。
- **建议**：保持。

### 3.9 [Medium] `ShadowSampling.slang` 没用 NdotL

- **相关文件**：[`ShadowSampling.slang:55-99`](../../Engine/Shaders/Lighting/ShadowSampling.slang)
- **问题描述**：`SampleShadow1Tap(refDepth, NdotL)` 接 `NdotL` 但未用，仅 bias 计入 `params.z`。这是 silent 中性 bias。
- **推荐修复方式**：用 NdotL 调 bias —— 这是正确的；CPU 端传给 shader 的 `params.z = normalBias`，shader 用作最大 slope bias；这就是"slope-scaled bias"。
- **建议**：保持，但要注释。

### 3.10 [Medium] Set 0 binding 1 是 SampledTexture，binding 2 是 Sampler；shader binding 必须一致

- **相关文件**：`ShadowSampling.slang:8-13` 与 C++ `RenderFrameResources.cpp:80-86`
- **问题描述**：见 Renderer 审计 3.1。C++ 未传 shadowSampledView / shadowSampler，shader 必须用 `null` —— `Sample` 调用对 null descriptor 在 Vulkan 是 UB。
- **建议**：与 Renderer 修复同时。

### 3.11 [Medium] `Common/Math.slang` 与 C++ `Math` 命名分散

- **相关文件**：`Common/Math.slang`
- **问题描述**：Slang 提供 cross / normalize / lerp 等，但命名不应与 `Math::` 重叠。
- **建议**：保持，但保持 Math.slang 内仅函数不引入 namespace。

### 3.12 [Medium] `ForwardPBR.slang` 用 `mul(g_FrameData.Camera.View, ...)` 取 viewPos 而不取 projection；潜在 stale Z

- **相关文件**：[`ForwardPBR.slang:76-77`](../../Engine/Shaders/Passes/ForwardPBR.slang)
- **问题描述**：取 view-space position 后用 `viewPos4.x` 作为 view-space depth。**如果 camera view matrix 不是 ±1 forward direction 会错**。
- **建议**：加 sanity invariant `assert(viewMatrix[0][2] == ±1)`。

### 3.13 [Low] `Lighting.slang` 未审

- **相关文件**：`Engine/Shaders/Lighting/Lighting.slang`
- **建议**：Stage 10 期间审。

### 3.14 [Low] `Materials/PBRMaterial.slang` 等未读

- **建议**：Stage 11 PBR Test。

### 3.15 [Low] `Tonemap.slang`, `Skybox.slang`, `Triangle.slang`, `UnlitTextured.slang` 未读

- **建议**：Stage 11。

---

## 4. 架构边界问题

- C++ ↔ Slang interop 类型分别在 `Runtime/Renderer/Private/ShaderInterop/` 与 `Engine/Shaders/Common/Types.slang`、`Lighting/LightingTypes.slang`、`Lighting/ShadowTypes.slang`。**这是合理的双源真理**：C++ 与 Slang 是两套语言，必须独立写。
- Slang header 的 `#pragma once` 防 multiple inclusion，OK。
- `RenderShaderKey` 与 `RenderShaderLibrary` 集中管理 source path/entry point/target，避免散在多个 pipeline desc。
- Shader `*LightingTypes.slang*` 引用 `Common/Types.slang`，后者引用 `Lighting/*` —— **circular ?** 实际 `Common/Types.slang` 已 include `Lighting/LightingTypes.slang` 与 `Lighting/ShadowTypes.slang`；后者不引用 `Common/Types.slang`。OK 不循环。

---

## 5. 性能 / 生命周期 / 同步问题

- **shader cache**：当前 `ShaderCache.cpp`、`ShaderSystem.cpp` 未审完；编译缓存键是否包含 stage / target / entry / hash。
- **shader reload** 是否支持 runtime edit？未审完。
- **Push constant size**：Vulkan 限制 `[128, 256]` 字节；当前 `ShadowDepthPushConstants` 96 B，OK。`PBRPushConstants` 96 B，OK。
- **GPU struct alignment**：每个 alignas(16) + static_assert 已就位。

---

## 6. 坐标 / 数学问题

- **+X Forward**：`ForwardPBR.slang:77` 用 `viewPos4.x` 是约定正确。
- **Reverse-Z**：ShadowDepthPass 反向 depth write（与 camera DepthZeroToOne 一致）；shader 端 `>=` bias 比较正确。
- **Y flip in shadow UV**：`ShadowSampling.slang:46` `clip.xy/clip.w * float2(0.5, -0.5) + 0.5`，Vulkan 专属。OK。
- **GLM column-major 与 Slang mul 顺序**：shader `mul(M, v)` 标准语义，与 GLM `M * v` 一致。OK。

---

## 7. 推荐修改

- 现在就修：
  - **3.1** hardware PCF；
  - **3.10** shadow bind group 注入（同步 RHI / Renderer）；
  - **3.2** C++ / Slang 字段名同；
- Stage 10 DebugDraw 期间修：
  - **3.6** `XENGINE_VIEW_SPACE_DEPTH` 宏；
  - **3.7** push constant padding 简化；
  - **3.12** view matrix sanity；
- Stage 11 PBR Test 修：
  - **3.4 / 3.5** 法线变换与 normal map 完整；
- 长期清理：
  - 3.13 / 3.14 / 3.15：剩余 shader 逐一审计。

---

## 8. 可拆给 Claude Code 的具体任务

1. 把 `Engine/Shaders/Common/Types.slang` 字段名 `Shadow` 改成 `Shadow`，与 C++ 端对齐。**slang 端单 line 修改**；`GPUFrameTypes.h` C++ 端 `Shadows` → `Shadow`；同步更新测试。
2. 在 `ShadowSampling.slang` 添加 `#define XENGINE_VIEW_SPACE_DEPTH(viewPos) ((viewPos).x)`，并在 `ForwardPBR.slang:77` 改用宏，附注释。**仅 shader 端**。
3. 在 `PBRPushConstants` / `ShadowDepthPushConstants` 把 `_pad0..2` 替换为 `float4 pad`，并 `static_assert(sizeof % 16 == 0)`。**仅 struct layout 调整**。
4. 在 `ForwardPBR.slang` 把法线变换改为 `N = normalize(mul(inverse(transpose((float3x3)g_PushConstants.model)), input.normal));`。注意：single precision inverse 精度，可加 uniform-scale check。**仅 shader 端**。
