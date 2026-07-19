# 07 Editor 与 Sandbox 审计

## 1. 审计范围

- `Engine/Source/Editor/`：所有 `Private/Panels`、`Private/ImGui`、`Private/Viewport`、`Private/EditorApplication`、`Private/EditorSystem`、`Private/EditorCamera`、`Private/EditorContext`、`Private/FreeCameraController`
- `Apps/Sandbox/Source/main.cpp`
- `Apps/EditorApp/Source/main.cpp`（未读完）
- 与之相关的 Renderer interface：RenderSystem 的 `SetOverlayCallback/SetViewProvider/SetOutputProvider` 三个回调

---

## 2. 当前优点

- **Editor 明确作为可选目标**：`XENGINE_ENABLE_EDITOR` CMake 选项，独立 `XEngineEditor` library；Sandbox 不含 editor 部分（`Apps/Sandbox/Source/main.cpp` 仅 include Runtime，不含 Editor）。**这是上一份提示文件希望的"Editor 与 Sandbox 分离"的实现**。
- **Editor ↔ Renderer 通过回调注入**：`RenderSystem::SetOverlayCallback/SetViewProvider/SetOutputProvider` 让 Editor 不直接 override Renderer Pipeline：Editor 提供 "我想画这一帧" 的 view 与 output provider，Renderer 接管 command 录制。这是好的边界。
- **ImGui backend 与 RHI 解耦**：`ImGuiVulkanBackend.{h,cpp}` 是 Editor 内部，明确私有于 Editor 模块，可独立替换为 ImGui-D3D12。
- **Viewport Panel 提供 camera-capture input mode**：能切换 `ViewportInputMode::CameraCapture` 与 `UI`，**允许 ImGui 不阻塞 debug camera**（见 `ViewportPanel.cpp:55-71`）。
- **Panels 模块各自独立**：`InspectorPanel`、`SceneHierarchyPanel`、`AssetBrowserPanel`、`RendererDebugPanel`、`RenderGraphPanel`、`ProfilerPanel`、`MainMenuBar`、`ViewportPanel` 各有 `Draw(EditorContext&)` 入口。
- **Sandbox 不依赖 Editor**：`main.cpp` 只 include Runtime + Scene + Asset。**这是对的**。
- **EditorContext 集中跨面板状态**：`Show*` flags、`ViewportTextureId`、`CurrentScenePath`、`UseEditorCamera`、`ViewportInputMode` 都通过 `EditorContext` 传，避免 panel 互依赖。

---

## 3. 发现的问题

### 3.1 [High] `RenderSystem` 的 `SetOutputProvider` 改名后与 `OutputProvider` 内部存储对齐

- **相关文件**：[`RenderSystem.cpp:411-417`](../../Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp)
- **问题描述**：
  - `RenderSystem::SetOutputProvider(std::function<bool(RHIRenderOutputDesc&)>)` 实现是 `m_Impl->OutputProvider = std::move(provider);`。
  - 这与 comment "if OutputProvider returns true with valid viewport, override" 一致。
  - **但是 EditorSystem 必须每帧调 `SetOutputProvider`，否则保持 fallback**。
- **推荐修复方式**：把 `SetOutputProvider` 的 signature 改为 `(callback, lifetime)` 让 Renderer 持有 context；这在现有接口上是 OK 的。**当前是 minor**。

### 3.2 [High] `EditorContext` 的 `ViewportTextureId` 是 uint64 / ImTextureID，但模型不一致

- **相关文件**：`Editor/Private/Panels/ViewportPanel.cpp:43`
- **问题描述**：
  - `ImGui::Image(static_cast<ImTextureID>(context.ViewportTextureId), viewportSize)`；
  - ImGui 在 Vulkan build 上 `ImTextureID = ImTextureID` 类型，但项目当前用 `uint64_t` 存 Vulkan descriptor set handle；
  - 没有强制要求 textureId 必须 alive。
- **为什么重要**：Editor 关闭 panel 会用到 `context.ShowViewport = false`；EditorSystem/ViewportPanel 配合删除。OK 但加 guard。
- **推荐修复方式**：加 `bool HasValidViewportTexture() const` 在 `EditorContext`，每次访问前检查。

### 3.3 [High] `EditorCamera` 与 `FreeCameraController` 重复

- **相关文件**：[`Editor/Private/FreeCameraController.{h,cpp}`](../../Engine/Source/Editor/Private/FreeCameraController.h)
- **问题描述**：`Scene/Private/DebugCameraController.cpp` 与 Editor `FreeCameraController` 都做"按 WASD 移动 + mouse look"。两份实现将 double maintain。
- **推荐修复方式**：把 `FreeCameraController` 移到 Runtime 或 Editor 公共 base，让 `DebugCameraController` 与 `FreeCameraController` 都派生。
- **建议**：Stage 10 DebugDraw 之前合并。

### 3.4 [High] `EditorContext::CurrentScenePath` 编辑保存路径与 Sandbox 加载路径不对称

- **相关文件**：`Editor/Private/EditorSystem.cpp`、`Editor/Public/XEngine/Editor/EditorContext.h`
- **问题描述**：
  - Sandbox `main.cpp:34` 走 `serializer.LoadFromFile(*scene, "asset://Scenes/Default.xscene")`；
  - Editor 同样可以通过 SceneHierarchyPanel 加载，但当前主菜单 / 按钮触发 save/load 仍待确认。
- **建议**：保持，但写一组 round-trip 测试。

### 3.5 [High] ImGui Vulkan Backend 与 RHI 直接耦合

- **相关文件**：[`Editor/Private/ImGui/ImGuiVulkanBackend.{h,cpp}`](../../Engine/Source/Editor/Private/ImGui/ImGuiVulkanBackend.h)
- **问题描述**：直接使用 `vulkan.h` 与 `volk.h`；不走 RHI public。这意味着 Editor 不能换成 Metal/D3D12。**但在沙盒接受范围内**（因为 Editor 是 optional + Vulkan-first）。
- **推荐修复方式**：未来加 D3D12 Editor 时加 `ImGuiD3D12Backend`，把 `ImGuiBackend` 接口抽象。

### 3.6 [High] `EditorSystem::OnCreate` 失败路径是否正确 Shutdown 不明

- **相关文件**：[`EditorSystem.cpp:61-...`](../../Engine/Source/Editor/Private/EditorSystem.cpp)
- **问题描述**：未审计完整。OnCreate 在依赖项缺失时 `XENGINE_LOG_ERROR` 后返回，但资源已经创建的（如 `m_ImGuiLayer`）的清理路径必须由 `OnDestroy` 兜底。
- **建议**：Stage 10 之前审 `EditorSystem.cpp` 完整实现。

### 3.7 [Medium] `ProfilerPanel` 与 `Diagnostics/Profiler` 的关系未读

- **相关文件**：[`Editor/Private/Panels/ProfilerPanel.{h,cpp}`](../../Engine/Source/Editor/Private/Panels/ProfilerPanel.cpp)
- **问题描述**：Tracy 集成的开关是 `XENGINE_ENABLE_TRACY`。当前面板用什么 backend 是 stale。
- **建议**：Stage 14 性能调优期间审。

### 3.8 [Medium] `RenderGraphPanel` 显示当前 graph，但没有真正的 graph introspection

- **相关文件**：[`Editor/Private/Panels/RenderGraphPanel.cpp`](../../Engine/Source/Editor/Private/Panels/RenderGraphPanel.cpp)
- **问题描述**：未审。当前 RenderGraphV0 不是真正的 graph，仅 `std::vector<RenderGraphPass>`。很可能 panel 是 stub。
- **建议**：RenderGraph V1 之前确认 panel 与 graph 同步。

### 3.9 [Medium] `AssetBrowserPanel` 依赖 `Assets/MaterialAsset.h`、`MeshAsset.h`、`TextureAsset.h`，走 Asset 公共路径；Editor 不应该绕过 AssetSystem

- **相关文件**：[`AssetBrowserPanel.cpp`](../../Engine/Source/Editor/Private/Panels/AssetBrowserPanel.cpp)
- **问题描述**：未审。需要确保 panel 通过 `AssetSystem` API（不是 `AssetRegistry`）取 asset list。
- **建议**：Stage 11。

### 3.10 [Medium] `InspectorPanel` / `SceneHierarchyPanel` 与 `EditorContext` 双向绑定

- **相关文件**：[`InspectorPanel.cpp`](../../Engine/Source/Editor/Private/Panels/InspectorPanel.cpp)、[`SceneHierarchyPanel.cpp`](../../Engine/Source/Editor/Private/Panels/SceneHierarchyPanel.cpp)
- **问题描述**：未审完。需要确认 panel 不会绕过 Scene system 直接改 EntityComponent。
- **建议**：Stage 11。

### 3.11 [Medium] `EditorViewportRenderTarget` 与 renderer 的 "OutputProvider" 协调

- **相关文件**：[`EditorViewportRenderTarget.{h,cpp}`](../../Engine/Source/Editor/Private/Viewport/EditorViewportRenderTarget.cpp)
- **问题描述**：未审。需要确认 viewpoint resize 时 RHI Texture 也重建，并且不会破坏 frames-in-flight。
- **建议**：Stage 10 DebugDraw 期间审。

### 3.12 [Medium] Sandbox 启动时直接 `LoadFromFile`，失败时只 log

- **相关文件**：[`Apps/Sandbox/Source/main.cpp:34`](../../Apps/Sandbox/Source/main.cpp)
- **问题描述**：当前 path "asset://Scenes/Default.xscene" 是一个虚拟路径；VirtualFileSystem 是否解析取决于 Init。失败时只 log error，但 `engine.Run()` 仍然进行，导致 empty scene。
- **推荐修复方式**：fallback 到 "constructed in code" 默认场景，确保 sandbox 永远有可渲染内容。
- **建议**：Stage 10 期间修。

### 3.13 [Low] `EditorContext::UseEditorCamera` 默认 false，会让 Renderer 用 primary camera（来自 Scene）

- **相关文件**：[`EditorContext.h`](../../Engine/Source/Editor/Public/XEngine/Editor/EditorContext.h)
- **问题描述**：默认 false 是好选择（"Editor 不主动要 camera"），但用户首次打开 editor viewport 会看到 scene camera 视角，困惑。
- **建议**：Stage 10 期间加 UI 微调。

### 3.14 [Low] `Sandbox` 不显示 Editor UI 是 OK；但 `config.MaxFrames = 0` 可能导致死循环

- **相关文件**：[`main.cpp:18`](../../Apps/Sandbox/Source/main.cpp)
- **问题描述**：`config.MaxFrames = 0` 应该是 "无限"，但 Engine 中可能有"0 = 不退出"。需确认。
- **建议**：Stage 10 期间确认 Engine 行为。

---

## 4. 架构边界问题

- **Sandbox 不依赖 Editor** ✓（`main.cpp` 完全不含 `#include <XEngine/Editor/...>`）。
- **Editor 依赖 Runtime** ✓（Editor 是 reader，Runtime 不应依赖 Editor；CMake 也确保如此）。
- **Editor 通过 RenderSystem 回调注入**（不能 register custom pass）—— 这是限制。允许"开发者写 custom render effect"需未来开 RenderGraph plugin point。
- **ImGui Vulkan backend 是 Editor 私有** ✓
- **EditorContext** 是跨 panel 状态，scope 全局，合适。
- **EditorApp vs Sandbox** 是两个 binary targets；每个 main.cpp 独立 configure subsystems set。OK。

---

## 5. 性能 / 生命周期 / 同步问题

- **ImGui 必须在主线程**：当前 `EditorSystem` 每帧调 `ImGuiLayer::NewFrame` 等；如果 Engine 多线程，ImGui 仍要 main thread。OK。
- **EditorViewportRenderTarget** resize 时与 swapchain 不能 race：要保证 `SetOutputProvider` 返回的 `ColorTargetView` 在当前 backbuffer 完成 usage 才回收。当前未审。
- **Sandbox 的 Scene load 是 CPU-blocking**：异步加载是 Stage 13+。

---

## 6. 坐标 / 数学问题

- `EditorCamera` 推测用 `Math::BuildViewMatrixLH_XForward`；与 Scene camera 同约定。OK。
- `ViewportAxisGizmo` 未审，但应基于 +X forward 绘制（X tip forward, Y right, Z up）。

---

## 7. 推荐修改

- 现在就修：
  - **3.12** Sandbox 默认 fallback scene；
  - **3.3** EditorCamera 与 FreeCameraController 合并 / 抽 base；
  - **3.6** EditorSystem OnCreate 失败 Shutdown path 完整。
- Stage 10 DebugDraw 期间修：
  - **3.5** ImGui backend 多 backend 抽象；
  - **3.11** EditorViewportRenderTarget resize flow 完整；
  - **3.13 / 3.14** Sandbox UX 微调；
- RenderGraph V1 前修：
  - **3.8** RenderGraphPanel 与 RenderGraph introspection 真实；
- 长期清理：
  - **3.7** ProfilerPanel；
  - **3.9 / 3.10** AssetBrowserPanel / InspectorPanel / SceneHierarchyPanel 审。

---

## 8. 可拆给 Claude Code 的具体任务

1. 在 `EditorContext.h` 加 `bool HasValidViewportTexture() const` 与 `void InvalidateViewportTexture()`；`ViewportPanel::Draw` 在每帧检查。**仅添加**。
2. 把 `FreeCameraController` 与 `DebugCameraController` 提到一个公共 base（`Public/Camera/CameraController.h`，移入 Runtime/Input 或独立 Runtime/Camera）；两个 subclass 继承。**仅重构，不改 motion semantics**。
3. Sandbox `main.cpp` 加 fallback：若 `LoadFromFile` 失败则构造 default cube scene。**仅修改 main.cpp**。
4. 在 `EditorSystem` OnCreate 失败路径加 `Shutdown()` 调用，确保已分配 resources 释放。**仅修改 OnCreate / 失败路径**。
