# 01 Core 与 Math 子系统审计

## 1. 审计范围

- `Engine/Source/Foundation/Core/Public/XEngine/Core/`：`Types.h`、`Base.h`、`Assert.h`、`Handle.h`、`NonCopyable.h`、`Result.h`、`Colors.h`、`UUID.h`、`ProjectPaths.h`、`Defines.h`
- `Engine/Source/Foundation/Core/Private/`：`Assert.cpp`、`UUID.cpp`、`ProjectPaths.cpp`
- `Engine/Source/Foundation/Diagnostics/Public/XEngine/Diagnostics/`：`DebugMarker.h`、`Profiler.h`、`ScopedTimer.h`
- `Engine/Source/Foundation/Logging/Public/XEngine/Logging/Log.h`
- `Engine/Source/Foundation/Math/Public/XEngine/Math/`：`MathTypes.h`、`Math.h`、`MathFunctions.h`、`CameraMatrices.h`、`CoordinateSystem.h`、`CoordinateConversion.h`、`AABB.h`、`Frustum.h`、`Rotator.h`、`Transform.h`
- `Engine/Source/Foundation/Math/Private/Frustum.cpp`
- 命名空间 `XEngine::CoordinateSystem`、`XEngine::Math` 内的全局/内联实现
- `Engine/CMakeLists.txt` 中关于 Foundation 部分的注册方式

整体确认了 Core 类型、Logging、Assert、Handle、数学别名、矩阵约定、AABB、Transform、相机矩阵、坐标系转换等基础设施。

---

## 2. 当前优点

- **类型别名统一**：[`MathTypes.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/MathTypes.h) 使用 `using Vec3 = glm::vec3;` 等显式别名，避免直接裸用 `glm::vec3`，这让 "替换为自研 math" 成为可能。
- **左手 +X forward 约定显式建模**：[`CoordinateSystem.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateSystem.h) 同时提供 `CoordinateSystem::Forward / Right / Up` 常量、`Math::GetForwardVector(quat)` 等内联工具，README 与 Scene/Renderer/Shader 的对接都依赖这套约定。
- **左手 LH 矩阵构造函数命名清晰**：`Math::LookAtLH_XForward`、`Math::BuildViewMatrixLH_XForward`、`Math::PerspectiveLH_ZO`、`Math::OrthographicLH_ZO` 在名字上强制 +X Forward + ZO 约定，未来一旦混淆会立刻在编译/调用处暴露。
- **基础结构上对齐 GPU**：`GPUFrameData`、`GPUCascadeShadowData`、`PBRPushConstants`、`ShadowDepthPushConstants` 等都用 `alignas(16)` 和 `static_assert` 强制 Mat4/Vec4 与 sizeof(float) 一致；这是 GPU 侧的稳定保证。
- **AABB 工具覆盖 TransformAABB / CombineAABB**：命中最近邻的阴影/CSM/Renderer 需求，复用广。
- **`std::is_standard_layout_v` 在 shader interop 全部就位**：可以尽早防止 reorder 或 vtable 变动带来的 silent breakage。
- **`Units` 命名空间虽然小但已就位**：`MetersPerUnit`、`CentimetersPerUnit` 已经预留，与未来美术 pipeline 单位口径挂钩。
- **`Math.h` 是一个 umbrella header**：包含所有 math 头，目前阶段对调用方友好，符合 "一个 include 进入数学" 的预期。

---

## 3. 发现的问题

### 3.1 [High] `Math::Math.h` 与 `XEngine::Math` 命名空间分裂定义

- **相关文件**：[`Math.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/Math.h)、[`MathFunctions.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/MathFunctions.h)、[`CameraMatrices.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h)、[`CoordinateSystem.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateSystem.h)
- **问题描述**：
  - 有些函数定义在 `XEngine::Math`（`MathFunctions.h`、`CameraMatrices.h`），然后通过 `using XEngine::Math::Foo;` 提到全局 `XEngine::` 命名空间。
  - `Math::PerspectiveLH_ZO` 在 `CameraMatrices.h` 中，`Math::Perspective`（叫它也是 LH_ZO）在 `MathFunctions.h` 中，**两者签名一致但名字不同**。`RenderSystem.cpp` 既用 `Math::PerspectiveLH_ZO`，又用 `Math::Perspective`（fallback 路径），依赖两套 alias 对齐。
  - `Math::LookAt` 包裹 `LookAtLH_XForward`，但 Scene/Editor 中部分代码用 `glm::lookAt`，部分用 `Math::LookAt`，命名分裂会让以后排查 "为什么 light 朝相反方向" 时浪费大量时间。
- **为什么重要**：
  - 引擎的核心约定是 +X forward、左手。一旦用户代码绕过 namespace alias 直接调用 `glm::lookAt` 或 `glm::perspective`，默认就是 -Z forward + 右手，结果就是 silent bug。
  - 等到 Editor、Camera、Shader 调试时再发现这种 bug 排查极费力。
- **推荐修复方式**：
  - 强制所有 `glm::lookAt / perspective / ortho / rotate` 等 API 调用都包装到 `Math::` 之下；
  - 在 CI 阶段加一个 grep 检查："在 Runtime/Renderer/Source 中禁止出现 `glm::lookAt|glm::perspective|glm::ortho`"；
  - 移除 `using XEngine::Math::Foo;` 顶层 using 指令，统一要么 `XEngine::Math::Foo`、要么 `XEngine::Foo` 二选一。
- **建议**：现在修（属于 P0 的一类）。

### 3.2 [Critical] GLM 列主序与 `XEngine` "Row-major 视图" 的混用

- **相关文件**：[`CameraMatrices.h:9-29`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h)、[`CoordinateConversion.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateConversion.h)、[`MathFunctions.h:112-118`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/MathFunctions.h)、[`Transform.h:13-16`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/Transform.h)
- **问题描述**：
  - GLM 默认是 column-major，`Mat4 m; m[0] = Vec4(right.x, right.y, right.z, 0)` 实际把 `right` 写到第 0 列。
  - `LookAtLH_XForward` 直接用 `view[0][0] = right.x; view[1][0] = right.y; ...` 这种 row-literal 写法。从 CPU 计算结果看是 "按行写" —— 即在 GLM column-major 表达下这是把 right 写到了第一列。这其实是 GLM 中 View 矩阵的标准列主序写法。
  - `Math::ComposeTRS` 通过 `Translate * Rotate * Scale` 复合，依赖 GLM 默认列主序乘法约定。
  - 但 `TransformPoint` 用 `Vec3(transform * Vec4(point, 1))` —— 对列主序来说是把 `point` 视作列向量右乘，结果是 `M * P`；这与 `ComposeTRS = T * R * S` 的复合顺序一致（列主序下 MV = T·R·S·v）。
  - **问题在于命名**：很多代码、注释会反复出现 "view matrix 行" 这种叙述（"view[0] 表示 X 行"），未来若有人误以为 row-major，会写 `view[3][0] = ...` 这类按行矩阵读的代码，与 GLM 相反。
  - `CoordinateConversion.h` 中的 `GltfPositionToXEngine` 是 `(-z, x, y)`，从 glTF 的 +Y up / -Z forward / RH 转 XEngine 的 +Z up / +X forward / LH，**做了坐标变换**。但矩阵列方向与坐标轴朝向是两个正交问题；同时 `TransformAABB` 直接对 corners 做 `transform * Vec4(corner, 1)`（GPU 端列主序下 `M * v`），是正确的。
- **为什么重要**：
  - 把 GLM 当成 row-major 看会导致矩阵相乘顺序完全反。如果有人在某个 `LookAt` 之外的地方直接写 `view * world` 而实际应该是 `world * view`，结果会反向，并且 cursor 方向、camera roll、light 朝向、shadow direction 全部错位。
  - CSM 的 `lightView * Vec4(corner, 1.0f)` 写法和 `view * corner` 与 "projection * view" 组合，如果哪天有人按 row-major 思维改一行就会立刻烂掉。
- **推荐修复方式**：
  - 在 [`MathTypes.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/MathTypes.h) 顶部加 `// XEngine math is column-major (GLM default). Do NOT write row-major code.` 一行注释；
  - 提供 `Mat4 Transpose(const Mat4&)` 显式使用，避免歧义；
  - 引入一组 helper：`Mat4 Mul(const Mat4&, const Mat4&)` 等显式表达 "M1 * M2"，命名上提示乘法方向。
- **建议**：现在修（属于 P0）。

### 3.3 [High] `Math::Perspective` 与 `Math::PerspectiveLH_ZO` 名字不一致

- **相关文件**：[`MathFunctions.h:87-95`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/MathFunctions.h)、[`CameraMatrices.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CameraMatrices.h)
- **问题描述**：`MathFunctions.h` 里 `Perspective` 包裹 `PerspectiveLH_ZO`，但 `CameraMatrices.h` 自己又暴露了 `PerspectiveLH_ZO`。意味着一个函数有两套 alias。RenderSystem 在 fallback 路径用 `Perspective`，正常路径用 `PerspectiveLH_ZO`。
- **推荐修复方式**：只保留 `PerspectiveLH_ZO` 作为唯一入口 + `Math::OrthographicLH_ZO`；`Math::Perspective` 改成别名或删除。
- **建议**：现在修（小重构）。

### 3.4 [High] `CoordinateConversion.h` 注释和实现不一致

- **相关文件**：[`CoordinateConversion.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateConversion.h)
- **问题描述**：
  - `GltfPositionToXEngine({-z, x, y})` 与 `GltfDirectionToXEngine({-z, x, y})`：注释 "GltfToXEngineFlipsHandedness = true" 是对的，但 `GltfTangentToXEngine` 注释说 "basis conversion changes handedness, so tangent-space handedness changes too"，结果是 `direction, -tangent.w`。这其实是手势（w）翻转方向是反的：在 glTF 中 `tangent.w` 决定 tangent 空间的手势是 +1 还是 -1；坐标轴 swap 后会切到反手势。
  - 但 `GltfTangentToXEngine` **没有任何验证/单元测试**，仍然写着 "Validate this convention with additional normal-mapped assets as the material system grows."。这意味着 normal map 取舍的 parity 还没确定，就会进入 forward PBR shader。
- **为什么重要**：NORMAL 偏移在 tangent 空间的手势错了，整个 PBR 法线方向整体反。这是 silent 的视觉错误。
- **推荐修复方式**：引入 glTF reference consumer（Blender、gltf-validator、`cgltf`）做对账；为 `GltfTangentToXEngine` 写一组 unit test，构造已知正方体 + 平整 normal map 比对。
- **建议**：Stage 11 PBR Test 修，但应该现在补一组 sanity unit test。

### 3.5 [Medium] `Rotator` 单位的含混

- **相关文件**：[`Rotator.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/Rotator.h)
- **问题描述**：Scene 公开了 `SetWorldRotationDegrees(... Rotator)`、`LightComponent::InnerConeAngleDegree` 等角度单位用 `Degree`/`Radians` 在命名上能区分；但 `Rotator` 自己只声明 `Roll/Pitch/Yaw` 默认值，没有写明是 degree 还是 radian。
- **推荐修复方式**：在 `Rotator` 上加 `explicit RotatorDeg(...)` / `RotatorRad(...)` 工厂，并在字段注释里写 `// stored as radians`。
- **建议**：Stage 10 DebugDraw 时修。

### 3.6 [Medium] `Frustum` 没有真正被消费

- **相关文件**：[`Frustum.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/Frustum.h)、`Frustum.cpp`
- **问题描述**：实现了 `Frustum::ExtractFromMatrix` 等 API，但 Renderer / CSM 完全没看到 frustum culling，仅靠 `AABB` 跑全量。Frustum 目前是 dead weight。
- **为什么重要**：未来要做 frustum culling（Stage ~12 之前就要做），它要么现在就在主路径上跑测试，要么会被忘记。
- **推荐修复方式**：
  - 把它纳入未来 culling lane；
  - 在 header 上加 `// TODO Frustum culling: not wired into RenderExtraction yet.`；
  - 写一组 unit test（合成四个角的 frustum，验证 contains/intersects）。
- **建议**：Stage 12 Culling 之前修。

### 3.7 [Medium] `AABB::TransformAABB` 没有处理负缩放

- **相关文件**：[`AABB.h:52-73`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/AABB.h)
- **问题描述**：`TransformAABB` 通过 8 角点 + Encapsulate 间接处理一般 affine，但当 matrix 行列式 < 0（带 negative scale / mirror）时能正确给出世界 AABB；同时也没有处理 matrix 不是纯 rigid 的情况（混有 skew 时 Encapsulate 给出的是 axis-aligned 范围，对后续 CSM 计算来说可能过紧）。
- **为什么重要**：CSM 现在用 `WorldBounds = TransformAABB(meshAsset->Bounds, transform)`，但 `RenderExtraction` 里只用 `Math::TransformAABB` 一次就到 `AABB` 后被两个路径消费（RenderObject.WorldBounds、RenderShadowManager.ComputeShadowCasterBounds），如果 `MeshAsset::Bounds` 表达的是 model-space AABB，那么：
  - 模型带 -1 缩放/镜像/非正交 basis，CSM 使用 AABB 拟合 sphere 半径可能偏大或偏小；
  - 这是 "shadow acne" 难以诊断时的常见嫌疑点。
- **推荐修复方式**：明确文档 `TransformAABB` 的前提条件（默认 model space AABB 是 axis-aligned 且无负缩放）；如要支持任意 affine，可考虑切到 `EPARunner` 或 OBB-to-AABB 拟合并提供测试覆盖。
- **建议**：Stage 12 Culling 前修。

### 3.8 [Low] `Units` 命名空间的应用缺失

- **相关文件**：[`CoordinateSystem.h`](../../Engine/Source/Foundation/Math/Public/XEngine/Math/CoordinateSystem.h)
- **问题描述**：定义了 `MetersPerUnit`、`CentimetersPerUnit`，但全引擎没有任何地方使用它。这意味着物理、import 边界、Lua/蓝图仍按 "裸数值" 传递。
- **推荐修复方式**：选一个 project-side convention：要么永远 1 unit = 1 m，要么让 Asset import 边界暴露一个 `FromSourceUnit(float)` 调用 `MetersPerUnit` 换算到 `XEngine` 单位。
- **建议**：Stage 13 物理之前讨论。

### 3.9 [Low] Logging 命名 LOG_WARN/INFO/ERROR 与 NDEBUG

- **相关文件**：[`Log.h`](../../Engine/Source/Foundation/Logging/Public/XEngine/Logging/Log.h)
- **问题描述**：`XENGINE_LOG_INFO` / `XENGINE_LOG_WARN` / `XENGINE_LOG_ERROR` 都是无条件的，所有 production build 都打。这对 release 不是问题但开销被忽略。
- **推荐修复方式**：使用 `XENGINE_LOG_LEVEL` 宏切换 INFO/WARN/ERROR，或者用 `[[gnu::cold]]` 标注。Stage 15 shipping 时再考虑。
- **建议**：长期清理。

### 3.10 [Low] `Handle<T>` 的使用稀疏

- **相关文件**：[`Handle.h`](../../Engine/Source/Foundation/Core/Public/XEngine/Core/Handle.h)
- **问题描述**：定义了 handle，但目前 `MeshHandle`、`MaterialHandle`、`AssetHandle` 等是 typedef `u64` 而非 `Handle<T>`；意味着 handle 不是一个受类型保护的强类型。
- **为什么重要**：一旦未来拆分为 `MeshHandle*` 与 `MaterialHandle*` 重载，自由切换会非常容易出错。
- **推荐修复方式**：把 `MeshHandle = Handle<class MeshHandleTag>;` 化，后续编译器会阻止偶发混用。
- **建议**：Stage 12 Culling 之前修（属于小重构，不影响功能）。

---

## 4. 架构边界问题

| 关注点 | 观察 | 影响 |
|---|---|---|
| Math 上行依赖 | 当前 Math/Foundation 不依赖任何 Renderer/RHI/Scene/Asset | 正确，未发现逆向依赖 |
| Transform 表达式 | `XEngine::Transform` 已被定义但 Renderer 仍直接用 `TransformComponent::GetWorldMatrix()` | `Transform` 不被消费，重叠 |
| AABB/Frustum 暴露 | 在 `Public/XEngine/Math` 下，Renderer/Scene 都消费 | OK |
| `CoordinateConversion` | 在 `Foundation/Math` 下，`Asset/GltfImporter` 引入 | OK；该转换语义与 GF import 边界贴合 |

---

## 5. 性能 / 生命周期 / 同步问题

- 当前 Math/Foundation 是 header-only inline 函数。会带到所有 TU 体积膨胀。
- GLM 是模板化的。`-O2` 可以 inline，但如果未来加入新函数或明显高负载函数（如 `Math::ComposeTRS` 被 Renderer 每对象调用）会需要 hot path 评估；这是 P3 关注项。
- 没有 thread-affinity 问题（数学不存在状态）。
- `Math::Inverse` 调用 `glm::inverse` —— 4x4 inverse 是 ~80 mul + 1 det + 部分除法；目前每帧 cascade 重建会调用多次（每 cascade 一次）。见 CSM 审计。

---

## 6. 坐标 / 数学问题

- **+X forward / +Z up / LH** 在 `CoordinateSystem.h` 已固化；CameraMatrices 与 transform 全部用 LH。
- **Reverse Z**：在 `RHI/RHIClipSpace.h` 中以 `DepthZeroToOne = true` 暴露，配合 `ApplyRHIClipSpaceConvention` 在 RHI / projection 边界上切换。
  - **隐患**：camera side 直接用 `Math::PerspectiveLH_ZO` 生成的是 `[1, 0]` 正向 depth，但 `DirectionalShadowPlanner` 显式倒置了 cascade 的 near/far；这意味着 CSM 与 camera depth range 不一致 —— 需要在 GPU 端 shader 里也做 reverse-Z 比较，避免 "[1, 0]" 范围在正向 Z 里反而比较错。具体看 CSM 审计（影子 Z 范围与 sampling 一致性问题）。
- **glTF import**：基础轴变换是 `(-z, x, y)`，经验上正确。但 tangent handedness 注释自承认 "validate"，是非常现实的风险。

---

## 7. 推荐修改

- 现在就修：
  - 强制所有 glm 数学调用走 `Math::` 命名空间（设立 grep 防线）；
  - 移除 `Math::Perspective` 多 alias；
  - 文档：`MathTypes.h` 顶部 column-major 注释；
  - 单元测试：`GltfTangentToXEngine`、`AABB::TransformAABB`、`Math::OrthographicLH_ZO`；
- Stage 10 DebugDraw 期间修：
  - `Rotator` 显式单位；
  - `Handle<T>` 强类型 handle；
- RenderGraph V1 前修：
  - `Frustum::ExtractFromMatrix` 接入 `RenderExtraction` 或显式标注 TODO；
- 长期清理：
  - `Units` 命名空间统一物理 import 边界；
  - `Log` 按级别编译期切换；
  - 把 `XEngine::Math` 与 `XEngine::` namespace using 策略二选一。

---

## 8. 可拆给 Claude Code 的具体任务

1. 修一处 CMake 触发的 grep 任务：在 `Engine/Source/Runtime/Renderer` 与 `Engine/Source/Runtime/Scene/Private` 中检测 `glm::(lookAt|perspective|ortho|rotate|scale|translate|inverse|transpose)`，失败返回非零。**只是 CI 任务，不改 C++ 源码**。
2. 在 `Docs/audit/CHECKLIST_math.md` 中列出每个 `Math::*` API 的 (+X Forward、LH、列主序、ZO) 约束清单，先文档不改代码。
3. 写一组不依赖渲染的 unit test harness：`Frustum::ExtractFromMatrix`、`Math::OrthographicLH_ZO` (与 GLM 比较)、`Math::PerspectiveLH_ZO` (与 GLM 比较)。放在 `Engine/Source/Tests/MathTests.cpp`，等待未来 CTest 注册。
4. 把 `MeshHandle`/`MaterialHandle`/`AssetHandle` 从 `using FooHandle = u64;` 改成 `Handle<FooHandleTag>`，请保留显式比较与 cast，**仅作 small refactor**。
