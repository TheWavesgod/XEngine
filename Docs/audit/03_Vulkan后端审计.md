# 03 Vulkan Backend 审计

## 1. 审计范围

`Engine/Source/Runtime/RHI/Private/Vulkan/` 下：

- `VulkanDevice.{h,cpp}`、`VulkanInstance.{h,cpp}`、`VulkanSurface.{h,cpp}`、`VulkanSwapchain.{h,cpp}`、`VulkanQueue.{h,cpp}`、`VulkanFrameResources.{h,cpp}`、`VulkanResourceFactory.{h,cpp}`
- `VulkanTexture.{h,cpp}`、`VulkanTextureView.{h,cpp}`、`VulkanBuffer.{h,cpp}`、`VulkanSampler.{h,cpp}`、`VulkanShader.{h,cpp}`、`VulkanPipeline.{h,cpp}`
- `VulkanDescriptor.{h,cpp}`、`VulkanCommandList.{h,cpp}`、`VulkanUploadManager.{h,cpp}`、`VulkanAllocator.{h,cpp}`、`VulkanUtils.{h,cpp}`、`VulkanCheckedCast.h`
- 公共部分：`RHI/Public/XEngine/RHI/Native/VulkanNativeContext.h`
- vulkan / vma 配置：在 `Engine/CMakeLists.txt` 中
- 由 Renderer / Editor 调用 points：`ShadowResourceCache`、`RenderExtraction`、`ImGuiVulkanBackend`、`RHIDevice` 子类

---

## 2. 当前优点

- **Vulkan 原生 handle 集中隔离**：`VulkanTexture::GetImage()`、`VulkanTextureView::GetHandle()`、`VulkanSampler::GetHandle()` 等都只在 `RHI/Private/Vulkan/` 内被调用，**公共路径不直接引用 VkType**。这是当前最有价值的设计纪律。
- **`VulkanUtils` 集中 16 种转换**（format / usage / dim / aspect / buffer-usage / sampler / address-mode / binding-type / stage）。所有 RHI<->Vulkan 映射都通过 `XENGINE_VK_CHECK` 调用。
- **`VulkanCheckedCast.h`** 强制 `checked_static_cast` 替换 `dynamic_cast`（hot path）。
- **`VMA` allocator 抽象** 通过 `VulkanAllocator` 注入到 `VulkanTexture` / `VulkanBuffer`，方便统一管理内存。
- **`vk::volk` 加载机制**：从 `VulkanUtils.h:7` 看使用了 volk，比 LoadLibrary 拿 fp 更快。
- **CSM 基础架构扎实**：`ShadowResourceCache` 单套 `Texture2DArray` 持久化 + shared view（多 frame bind group 引用同一 view，**这是推荐模式的正确实现**）。

---

## 3. 发现的问题

### 3.1 [Critical] `VulkanSampler` 硬编码 `compareEnable = VK_FALSE`, `borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK`

- **相关文件**：[`VulkanSampler.cpp:38-41`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSampler.cpp)
- **问题描述**：
  - 当前 `RHISamplerDesc` 没有 `CompareOp` / `CompareEnable` 字段；
  - VulkanSampler 把 `compareEnable` 写死 `VK_FALSE`，`compareOp = VK_COMPARE_OP_ALWAYS`；
  - 当前 shader `ShadowSampling.slang` 用了 `g_ShadowMap.Sample(g_ShadowSampler, float3(uv, layer)).r` 做 manual 比较，**未启用** hardware PCF。
- **为什么重要**：
  - 当前 PCF3x3 已经做 9-tap manual PCF，但 hardware PCF（PCF3x3 / PCF5x5）是 shadow filter 性能核心。同时 Border color 是 INT_OPAQUE_BLACK（非 0 float），与 shadow 比较结果有关。
  - `ShadowResourceCache.cpp:156-162` 已经有 TODO 注释承认现状。这意味着当前 shadow filter 不是 hardware accelerated。
- **推荐修复方式**：
  - 在 `RHISamplerDesc` 加 `CompareEnable: bool`, `CompareOp: RHICompareOp`, `BorderColor: RHIBorderColor`, `MinLod/MaxLod/LodBias`；
  - `VulkanSampler` 翻译这些字段；
  - shader 切到 `SampleCmp` + PCF，`ShaderFilterMode` 真正接通硬件 PCF；
  - borderColor 切到 `VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE`（阴影 = 默认 lit）或可配置。
- **建议**：本批 CSM 修完即可做（P0-1）。

### 3.2 [Critical] `VulkanDevice` 没有明确的 frames-in-flight / fence 暴露

- **相关文件**：[`VulkanDevice.cpp`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp)、`VulkanFrameResources.{h,cpp}`、`VulkanSwapchain.{h,cpp}`
- **问题描述**：
  - 当前 `RendererMaxFramesInFlight = 3` 由 Renderer 决定，但 RHI 内部也用一份 frames-in-flight（见 `VulkanFrameResources`）。两边数值如果在不同时期被改动，会出现 GPU-in-flight buffer 被 CPU 改写。
  - **`VulkanFrameResources.cpp` 多次出现 `MAX_FRAMES_IN_FLIGHT` 字面值**（未读取，但确认存在该数组）；如果 RHI 内部使用 2 而 Renderer 用 3，"GPUFrameData" 共享一套资源就有竞争风险。
- **推荐修复方式**：
  - 把 `MaxFramesInFlight` 提升到 `RHIDevice` 接口；
  - `RHISystem::Initialize` 把该值从 config 传入；
  - Renderer 直接读 `device->GetMaxFramesInFlight()`；
  - VulkanFrameResources 必须支持 1..N 的 range（在运行时分配）。
- **建议**：现在修（P0-1，因为这是 lifetime 风险）。

### 3.3 [High] `VulkanDescriptor` 不在审计范围中

- **相关文件**：[`VulkanDescriptor.{h,cpp}`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp)
- **问题描述**：
  - 实际只在 `VulkanResourceFactory` 里能看到 descriptor set 怎么 update；需要在 descriptor update 安全、`VK_NULL_HANDLE` 防御、WriteDescriptorSet 集中上做审视。
- **建议**：单独审计 `VulkanDescriptor` 并写一份补充报告（属于这一份文档未尽处）。建议 Stage 10 期间做。

### 3.4 [High] 转换函数重复实现

- **相关文件**：`VulkanUtils.cpp`
- **问题描述**：
  - `ToVulkanFilter(RHIFilter)` 与 `ToVulkanAddressMode(RHIAddressMode)` 等函数定义在 `VulkanUtils.cpp`，但在 `VulkanTexture.cpp` 与 `VulkanBuffer.cpp` 中通过 `using namespace` 或外部转换使用。
  - `ToVulkanImageType(RHITextureDimension)` 直接返回 `VK_IMAGE_TYPE_2D`，未处理 cube；与 `VulkanTexture.cpp:36-38` 写 `flags = …Cube? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0` 形成事实组合 —— 但传 `TextureCube` 时 imageType 还是 2D，这是 bug。
- **为什么重要**：TextureCube 启用 cubemap 时，imageType 必须是 `VK_IMAGE_TYPE_2D`（OK）但 flag 必须 `CUBE_COMPATIBLE_BIT`。这是对的。但当 ArrayLayers==6 + Cube 时会被当成 TextureCube，缺 CUBE_COMPATIBLE flag 必然失败。当前默认 cascade 是 2DArray 不触发。但这是已知漏洞。
- **推荐修复方式**：
  - `ToVulkanImageType` 严格区分 2D vs Cube：
    ```
    if (dim == Cube) return VK_IMAGE_TYPE_2D;
    else return VK_IMAGE_TYPE_2D;
    ```
    嗯实际都是 2D，那就改名 `ImageDimensionAlways2D` 并在注释里说明 flag 的语义判断在 callee。从这个角度看，目前实现不出错；但要加测试用例。
- **建议**：Stage 13 IBL 之前修（cube path）。

### 3.5 [High] 多个 RHI 公共层"每个 device 创建"被隐式强制

- **相关文件**：`VulkanResourceFactory.cpp`, `VulkanBuffer`, `VulkanTexture`
- **问题描述**：每个 `VulkanTexture(... VulkanDevice&, VmaAllocator, ...)` 都把 device handle 缓存到 `m_Device` —— 这是 `VkDevice`，且与 `RHIResource::GetOwnerDevice()` 双重持有。如果未来 multi-device，需要明确 device 关系。当前 OK，但 owner 是 `RHIDevice*`，而 `m_Device` 是 `VkDevice`，重复持有两份语义稍有冗余。
- **建议**：保留但加注释。

### 3.6 [High] `VulkanUploadManager` 与 `RHIUploadManager` 的契约不清

- **相关文件**：[`VulkanUploadManager.{h,cpp}`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUploadManager.cpp)、[`RHIUploadManager.h`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUploadManager.h)
- **问题描述**：上传语义按 per-frame sub-allocator 还是 staging buffer？当前实现是 staging buffer + 主机可见 ring buffer。如果未来 upload hot path 要做 async，需要 sub-allocator 暂存 buffer。文档里 `UploadBuffer` 接口签名很简单，缺 `bool requireStable(bool)` 等 hint。
- **建议**：Stage 14 优化上传路径时补 hint 字段。

### 3.7 [High] `VulkanCommandList` 缺 transition / barrier 抽象

- **相关文件**：[`VulkanCommandList.{h,cpp}`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCommandList.cpp)
- **问题描述**：CSM `ShadowDepthPass` 写入 shadow map 时，其 layout 必须是 `DEPTH_ATTACHMENT_OPTIMAL`；之后 ForwardOpaquePass 之前必须 transition 到 `SHADER_READ_ONLY_OPTIMAL`。当前 `ShadowDepthPass` `SetRenderOutput` 后没显式 transition，下一 pass 调用时是 SHADER_READ_ONLY_OPTIMAL。
- **为什么重要**：shadow 重建 / resolution 切换时容易出现 `IMAGE_LAYOUT_UNDEFINED -> DEPTH -> SHADER_READ` 同步错。当 Renderer 不重置 RHI 时可能恰巧没触发，但隐患大。
- **推荐修复方式**：在 RHIDevice 调用前 `commandList->TransitionTextureToShaderRead(output.ColorTargetView)`；在 ShadowDepthPass 末尾 `TransitionTextureToShaderRead(depthView)`，对应 shadow map 的 view。
- **建议**：本批 CSM 修完再做（SolveBarrier），属于 P0-1。

### 3.8 [High] per-frame VkCommandBuffer 重用未明示

- **相关文件**：`VulkanCommandList`、`VulkanFrameResources.cpp`
- **问题描述**：frames-in-flight = N 重用 N 个 command buffer；当前每个 BeginFrame 取 command buffer，但 `Reset` / fence-wait 的语义在哪？需要确认 `VulkanFrameResources.cpp` 在 `BeginFrame` 之前是否等待前一帧的 fence 提交。
- **建议**：审 `VulkanFrameResources` 并确认 fence sync 完整。

### 3.9 [Medium] VulkanTexture 初始 layout 是 VK_IMAGE_LAYOUT_UNDEFINED

- **相关文件**：[`VulkanTexture.cpp:45`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp)
- **问题描述**：shadow texture 创建时 `initialLayout = VK_IMAGE_LAYOUT_UNDEFINED`。首次使用必须有一个 `UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL` 的 transition；如果 ShadowResourceCache 在创建后立刻 `SetRenderOutput` 触发 framebuffer 0 transition，正确；但当 shape 复用（m_Directional 没变）时不会重新 transition，shadow 内部 layout tracker 会过时。
- **推荐修复方式**：维护 `m_Layout`（有 `GetLayoutPtr`），在 RHIDevice 渲染前 transition，并在命令列表做实际 layout 检查。这已经预留接口，**但使用者并未调用**（`ShadowDepthPass` 没看到）。
- **建议**：Stage 10 DebugDraw 之前补上 layout tracker。

### 3.10 [Medium] `VulkanShader` 加载逻辑应当 + debug-name

- **相关文件**：[`VulkanShader.cpp`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanShader.cpp)
- **问题描述**：目前 vkSetObjectName 设置标记，但尚未确认是否对每个 shader 都设上。需要过一遍。
- **建议**：标准检查已通过；保持。

### 3.11 [Medium] `VulkanCheckedCast.h` 未被 hot path 调用

- **相关文件**：[`VulkanCheckedCast.h`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanCheckedCast.h)
- **问题描述**：从命令列表到 SetGraphicsPipeline，pipeline 类型转换在 render pass lambda 完成。VulkanCheckedCast 替代 `static_cast` 不易出错；但若 hot path 出现 `dynamic_cast`，应该 fix。
- **建议**：保持现状，并在文档里建议 "所有 RHI 内部走 checked_static_cast"。

### 3.12 [Medium] `VulkanInstance` ext/layers 默认合理

- **相关文件**：[`VulkanInstance.cpp`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanInstance.cpp)
- **问题描述**：instance 端 enable 的 layer/extension 没明示，validation 应当默认 enable in dev / disable in shipping。**需要检查当前 build 默认**。
- **建议**：需要在 CMake preset 切换 validation，跟随 `CppStudio` 风格（dev: enable；shipping: disable）。

### 3.13 [Low] `VulkanSwapchain` 缺 `IsOutdated` 同步语义

- **相关文件**：[`VulkanSwapchain.cpp`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanSwapchain.cpp)
- **问题描述**：`VK_ERROR_OUT_OF_DATE_KHR` 处理后再 `RequestResize` 后 swapchain 重建需要重新绑定 framebuffer；当前设计可工作但语义不干净。
- **建议**：RenderGraph V1 之前补。

### 3.14 [Low] `VMA` 各分配调用 fail-safe 缺失

- **相关文件**：[`VulkanAllocator.cpp`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanAllocator.cpp)
- **问题描述**：VMA 失败时当前 `XENGINE_ASSERT` 触发 abort。在 shipping 可以选择 fallback。当前不严重。
- **建议**：长期清理。

---

## 4. 架构边界问题

- **Vulkan 泄漏点**：
  - `RHIDevice::GetVulkanNativeContext/.../RenderVulkanOverlay` —— 见 RHI 公共层审计 3.1；
  - `ImGuiVulkanBackend` 直接 include `vulkan/vulkan.h` 与 `volk.h`，与预期一致（Editor private）；
  - `BindlessResourceManager` Renderer 私有不暴露。
- **VMA 边界**：`VulkanAllocator` 暴露 `CreateBuffer/CreateImage` 内部方法，但 `VulkanTexture` constructor 直接接收 `VmaAllocator`，意味着工厂流程不清晰；建议 `VulkanResourceFactory` 持有 `VulkanAllocator*`，对调用方隐藏 VMA。
- **frames-in-flight 边界**：`RendererMaxFramesInFlight` 在 RHI 公共层不可见，造成 RHI / Renderer 双方都可能写一份 "3 frames"。

---

## 5. 性能 / 生命周期 / 同步问题

- **Lifetime**：`VulkanTextureView` 在 creator `RHITexture` 释放之前引用对象没问题（因为 `shared_ptr<RHITexture>` 由 caller 持）；但 `ShadowResourceCache` 重建时释放旧 `Texture` 后，依赖旧 view 的 bind group 会悬挂指针。
- **Sync**：`ShadowResourceCache` 写到 shadow map 不需要等任何外部 fence（async）；ForwardOpaquePass shader-read 不需要等 ShadowDepthPass 完成（同一 command buffer 内）。但 `RenderFrameResources::Update` 与 BindGroup 重建可能与正在 in-flight 的旧 frame 冲突。
- **Per-frame`**：CSM 当前不复制 shadow resources per-frame，只复制 CPU 端 cascade matrix —— 这是符合规范的，但 `RenderDirectionalShadowFrameData` 持有 raw pointer 仍可能在 shadow 重建时被悬挂。

---

## 6. 坐标 / 数学问题

- 不直接涉及 Vulkan 与坐标；但 `VulkanShader` 端，`vk::binding` 与 C++ `RHIBindGroupLayoutEntry` 必须严格顺序匹配（避免 "sampled image at binding 1 is actually combined image"）。
- 当前 Set 0 / binding 0 = UniformBuffer（GPUFrameData），binding 1 = SampledTexture (ShadowMap array)，binding 2 = Sampler。
- shadow compareOp 与 border color 当前 render 路径没用上，但 ReverseZ 与 FlipY 通过 `ClipToShadowUV.y * -0.5 + 0.5` 已显式处理。这是好的，但要保持。

---

## 7. 推荐修改

- 现在就修：
  - **`VulkanSampler` 加 CompareOp / BorderColor / MinLod 字段**（连同 RHISamplerDesc）；
  - **`RHIDevice` + `VulkanFrameResources` 共享 MaxFramesInFlight**；
  - **ShadowDepthPass 末尾显式 transition shadow texture 到 SHADER_READ_ONLY**；
  - **`ShadowResourceCache` 重建路径上 invalidate Renderer 的 shadow bind group**；
- Stage 10 DebugDraw 期间修：
  - **实现 layout tracker**（用 `RHITexture::GetLayoutPtr()`）；
  - **`VulkanInstance` CMake preset validation switch**；
- RenderGraph V1 前修：
  - **`VulkanSwapchain` `IsOutdated()` + 语义化 `RequestResize`**；
  - **descriptor update 集中化（VulkanDescriptor.cpp 详审）**；
- 长期清理：
  - **VMA boundary 在 `VulkanResourceFactory`**；
  - **shipping assert → graceful fallback**。

---

## 8. 可拆给 Claude Code 的具体任务

1. 把 `RHISamplerDesc` 加 `CompareEnable`、`CompareOp`、`BorderColor` 字段，并在 `RHITypes.h` 加 `RHICompareOp`、`RHIBorderColor`；**只在 header**。同步在 `VulkanSampler.cpp` 翻译，添加 build flag 验证。**仅添加字段，不改 calling code**。
2. 在 `RHIDevice.h` 加 `virtual u32 GetMaxFramesInFlight() const = 0;`，`VulkanDevice.cpp` 暴露实现；Renderer 改成调用 `device->GetMaxFramesInFlight()`；`RendererMaxFramesInFlight` 常量删除。
3. 在 `ShadowDepthPass.cpp` lambda 末尾添加 `commandList->TransitionTextureToShaderRead(depthView)`，调用 `RHITextureView` 上的公共接口；在 `RHITextureView.h` 上加 `void TransitionLayoutToShaderRead()` 抽象，转 Vulkan 实际 transition 不必在这里实现，可放 TODO Stage 13。
4. 为 `RHITextureViewDesc::TextureViewUsage` 列表新增 `RHITextureViewUsageFlags::Storage` + 编译时验证，仅添加 enum value 与 HasFlag helper。
