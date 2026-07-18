# 02 RHI Public Layer 审计

## 1. 审计范围

`Engine/Source/Runtime/RHI/Public/XEngine/RHI/` 下：

- `RHI.h`、`RHITypes.h`、`RHIResource.h`、`RHIDevice.h`、`RHIQueue.h`、`RHICommandList.h`、`RHIResourceFactory.h`、`RHIUploadManager.h`、`RHISwapchain.h`、`RHISystem.h`、`RHIClipSpace.h`、`RHIUtils.h`
- `Resources/`：`RHIBindGroup.h`、`RHIBuffer.h`、`RHIPipeline.h`、`RHISampler.h`、`RHIShader.h`、`RHITexture.h`、`RHITextureView.h`
- `Native/VulkanNativeContext.h`

并参考 RHI private 实现路径与几个消费端的 `BindlessResourceManager` 和 Renderer。

---

## 2. 当前优点

- **资源/视图职责分离**：`RHITexture` 表示 GPU resource，`RHITextureView` 表示显式 view。`RHITextureViewDesc` 是显式 view 创建描述符，与 backend 类型解耦。这与提示文档期望方向 "RHITexture = GPU resource，RHITextureView = explicit view" 完全一致。
- **Format/Usage/Dimension/Aspect 等枚举集中于 [`RHITypes.h`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h)**：所有枚举（`RHIFormat`、`RHITextureUsageFlags`、`RHITextureViewUsageFlags`、`RHITextureAspectFlags`、`RHIBufferUsage`、`RHIShaderStageFlags`、`RHIBindingType`）和对应 desc 类型都在同一文件。
- **有 `HasFlag` + 运算符重载**：flag-style 枚举自洽。
- **`RHIBindingResource` 是 union-style**：把 `TextureView`/`Sampler`/`Buffer`/`BufferOffset`/`BufferSize` 集中绑定，使用者只需选 `RHIBindingType`。
- **`RHIResource` 是公共 owner-device 抽象**：每个资源返回 `GetOwnerDevice()` 和 `GetBackend()`，可用于 future multi-backend safety。
- **`VulkanNativeContext` 与 `VulkanNativeTextureBinding` 抽象** 把 Vulkan 原生类型收敛到 `RHI/Native/`，避免公开广泛暴露 Vulkan。
- **`RHIRenderOutputDesc` 是统一的 render output 描述**：`ColorTargetView`/`DepthTargetView`/`Viewport`/`ColorFormat`/`DepthFormat`/`RenderToSwapchain`。
- **`ApplyRHIClipSpaceConvention` 接口（RHIClipSpace.h）**：把 RHI 端的 clip-space 切换责任明确放在 RHI/projection 边界上。

---

## 3. 发现的问题

### 3.1 [Critical] `RHIDevice` 直接 expose `VulkanNativeContext` / `VulkanNativeTextureBinding` / `RenderVulkanOverlay`

- **相关文件**：[`RHIDevice.h:18-63`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h)、`Native/VulkanNativeContext.h`
- **问题描述**：
  - `RHIDevice` 是 RHI 公共接口，但它虚拟方法有 `GetVulkanNativeContext`、`GetVulkanNativeTextureBinding`、`RenderVulkanOverlay`。这等于让 RHI 公共层拥有 "Vulkan-specific 路径"。
  - "Public RHI headers 不能暴露 Vulkan types" 这条规则在 `VulkanNativeContext.h` 中破了例，把 `VkInstance`、`VkPhysicalDevice` 等加进 RHI public 层。
- **为什么重要**：
  - 这是泄漏点的根因。`ImGui/ImGuiVulkanBackend` 直接拿 Vulkan handle 是 OK 的，但应该走 `RHI/Native/` 之外的 adapter，而不是让 `RHIDevice` 本身给出 Vulkan 接口。
  - 将来加 D3D12/Metal backend 时，每个 backend 都会希望自己加一个 `GetD3D12...`、`GetMetal...`，最后 RHI 类变成臃肿 union-of-backends。
- **推荐修复方式**：
  - 把所有 Vulkan-specific native handle 提取到一个 `Native/Vulkan*BackendAdapter.h`，仅在 `RHI/Private/Vulkan/...` 与 `Editor/Private/ImGui/ImGuiVulkanBackend` 间共享；
  - 把 `RenderVulkanOverlay` 等只在 Editor/Vulkan backend 用到的接口改为回调 / handle-based 抽象；
  - 公共 `RHIDevice` 绝不再加 `GetD3D12X` / `GetMetalX` 这类方法。
- **建议**：现在修（属于 P0）。

### 3.2 [Critical] `RHITextureView` 没有 lifetime 防护

- **相关文件**：`RHITextureView.h`、`RHITypes.h:284-296`、`VulkanTextureView.cpp`
- **问题描述**：
  - `RHITextureViewDesc::Texture = nullptr;` 是裸指针，但是当 RHI 内部释放 `RHITexture`（如 `ShadowResourceCache` 重建纹理时 `m_Directional = {};`）时，并没有机制通知已创建的 `RHITextureView`。
  - `DirectionalShadowResources` 内 `SampledView` 和 `LayerDepthViews` 都是 `shared_ptr<RHITextureView>`，它们的 owner 是 `shared_ptr`；但是 `m_Directional = {};` 直接 reset，没清理对应共享关系。`ShadowResourceCache` 自己也持有 shared_ptr，OK。但 `RenderDirectionalShadowFrameData` 里是裸 `RHITextureView*`，万一未来 ShadowResourceCache 重建（用户改分辨率 / cascade count），frame 上既有的绑定写的是旧 view，这种事现在没保护。
- **为什么重要**：recreate 路径上 cbuffer 引用和 view 引用一致性是 silent bug 主要来源，目前仅靠 "同 shape 不重建" 保护（`ShadowResourceCache` 第 58 行），其实只是规避，不是根治。
- **推荐修复方式**：
  - 引入 frame-level `RHIBindGroup` 在重建 shadow 时显式 `Reset()`，并由 `RenderFrameResources::Update` 在每帧先 invalidate 再 set；
  - 给 `RHITextureView` 加 debug-time parent 关联，便于 dereference 检查；
  - 把 `RenderDirectionalShadowFrameData` 中的 raw pointer 替换为 `std::shared_ptr<RHITextureView>` 的 observer 或者 lock 到 ShadowResourceCache 上的 handle。
- **建议**：P0-1：现在修 lifetime；P0-2：Stage 13 Multi-light 之前整体加固。

### 3.3 [High] `RHIRenderOutputDesc` 是 union-like 但有重复定义

- **相关文件**：[`RHITypes.h:385-393`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h)
- **问题描述**：
  - `RenderToSwapchain = true` 时绑定 swapchain 内部 surface；当 `false` 时必须提供 `ColorTargetView`/`DepthTargetView`。
  - 代码使用如 `ShadowDepthPass.cpp:79-86` 与 `RenderSystem.cpp:165-174`，如果 `RenderToSwapchain=true` 但 `ColorTargetView != nullptr` 行为是怎样的？RHIDevice 没明确规定；如果多个 caller 都用同一 field，行为是未定义的。
- **为什么重要**：未来的 Frame Graph executor 需要明确这个语义。
- **推荐修复方式**：在 `RHIRenderOutputDesc` 顶层加 `IsValid()` 方法（不变量：`RenderToSwapchain XOR (ColorTargetView != nullptr || DepthTargetView != nullptr)`），并在 RHIDevice 调用前断言。
- **建议**：RenderGraph V1 前修。

### 3.4 [High] `RHICapabilities` 字段少且未充分传递

- **相关文件**：[`RHITypes.h:238-247`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h)
- **问题描述**：
  - 现有 `MaxTextureDimension2D`、`MaxTextureArrayLayers`、`MaxPushConstantSize`、`MaxBoundDescriptorSets`、`SupportsSamplerAnisotropy`、`MaxSamplerAnisotropy`、`SupportsDynamicRendering`。
  - 缺：`MaxPerStageDescriptorSamplers`、`MaxPerStageDescriptorSampledImages`、`MaxPerStageDescriptorUniformBuffers`、`MaxPerStageDescriptorStorageBuffers`、`MaxColorAttachmentSamples`、depth clip supported、timeline semaphore、push descriptor、bindless、buffer device address、ray tracing 等。
  - 同时 `SamplerAnisotropy` 默认 1.0，但代码里 `RHISamplerDesc::MaxAnisotropy` 默认也是 1.0。VulkanSampler 用 `capabilities.MaxSamplerAnisotropy` 来 clamp，是对的，但消费代码（如 PBR shader）不知道 anisotropy 是否可用。
- **推荐修复方式**：扩展 `RHICapabilities` 字段；当前可以保持极简，但配套加 `// TODO Stage 13` 注释，否则后续会发现 capabilities 不足以做 feature gate。
- **建议**：Stage 13 之前补齐。

### 3.5 [High] `RHIBufferUsage::Storage` 没有 SSBO 描述，bindless 无法消费

- **相关文件**：[`RHITypes.h:34-43`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h)、[Resources/RHIBuffer.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBuffer.h)
- **问题描述**：
  - `Storage` 枚举在，但 `RHIShaderDesc` / pipeline-desc 都未消费 storage buffer。可以为 GPU-scene / SSBO 拉出一条路。
- **建议**：RenderGraph V1 之前补齐。

### 3.6 [High] `RHIBuffer` 没有 Update 抽象，只有 `UploadManager::UploadBuffer`

- **相关文件**：[`RHIUploadManager.h`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUploadManager.h)、[Resources/RHIBuffer.h](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIBuffer.h)
- **问题描述**：
  - `RenderFrameResources.cpp:128` 调 `buffer->Update(&data, sizeof(data))` —— 但 `RHIBuffer` 接口里没有 `Update` 方法直接说明，看 RHIBuffer.h 是 ctor / GetXXX / IsValid 等。这是私有的 VulkanBuffer 方法。
  - 跨 backend 移植到 D3D12/Metal 时需要 STAGING_BUFFER 路径，direct `Update` 在 D3D12 不可用（需要 DISCARD/NO_OVERWRITE hint 或 staging）。建议把它统一收敛到 `RHIUploadManager::UploadBuffer`。
- **推荐修复方式**：把 `buffer->Update` 私有化或删除，统一改走 `UploadManager`。
- **建议**：现在修（小修补）。

### 3.7 [High] `BindlessResourceManager` 不属于 V0 设计

- **相关文件**：[`Renderer/Private/Resources/BindlessResourceManager.h`](../../Engine/Source/Runtime/Renderer/Private/Resources/BindlessResourceManager.h)、[`.cpp`](../../Engine/Source/Runtime/Renderer/Private/Resources/BindlessResourceManager.cpp)
- **问题描述**：
  - 这是 Renderer 私有路径，但它已经写了 skeleton（已在私有）。RHI Bindless 列入 "本期不做"。这个文件需要标注 TODO 并不进入 CSM / Forward 路径，否则 slip into production。
- **推荐修复方式**：
  - 检查仓库是否真的不被引用（看起来现在只有自身 cpp/h，无外部 include）。如果是清白，可以留着但加 `// Stage 13`，并禁用。
- **建议**：Stage 13 之前清理。

### 3.8 [Medium] `RHIUtils` 没有实质内容

- **相关文件**：[`RHIUtils.h`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIUtils.h)、[`RHIUtils.cpp`](../../Engine/Source/Runtime/RHI/Private/RHIUtils.cpp)
- **问题描述**：`RHIUtils` 应该是 backend-neutral 的辅助（FormatSizeInBytes、SampleCountToMSAA、FormatIsDepth 等）。当前看是不存在实现。
- **推荐修复方式**：补上 `IsDepthFormat`、`IsStencilFormat`、`GetFormatSizeInBytes`、`GetAspectMask` 等工具。同时让 Renderer/RHI 都引用这些工具。
- **建议**：现在修（小工作量）。

### 3.9 [Medium] `RHIDevice::CreateX` 抽象已经迁移到 `RHIResourceFactory`，但仍有 `GetDevice()->GetResourceFactory()` 调用点

- **相关文件**：[`RenderSystem.cpp:312-340`](../../Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp)
- **问题描述**：
  - `RHIDevice` 没有 `CreateBuffer` 等方法（已迁移），但调用点 `device->GetResourceFactory().CreateBuffer` 是间接四层。下一步可选：(a) 收敛工厂到 Device，(b) 把 Factory 实例 cache 在 `RHIDevice` 上，避免每次调用都 `GetResourceFactory()`。
  - `RenderSystem.cpp` 还会 `device->WaitIdle()` + `device->BeginFrame()` + `device->EndFrame()` + `device->GetSwapchainFormat()`，这些都是 Device 接口，OK。
- **建议**：保留现状，Stage 14 时再清理。

### 3.10 [Medium] `RHITextureDimension::TextureCube` 仅 6 面，未支持 cubemap array

- **相关文件**：[`RHITypes.h:97-102`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h)
- **问题描述**：当前只 2D / 2DArray / Cube。IBL/Skybox 需要 Cube / CubeArray。
- **建议**：IBL 之前补 CubeArray，至少留 `// TODO CubeArray`。

### 3.11 [Low] `RHISwapchain` 没有 `GetCurrentImageIndex()` 一致接口

- **相关文件**：[`RHISwapchain.h`](../../Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHISwapchain.h)
- **问题描述**：Renderer 通过 RHIDevice 出口拿到 swapchain，`BeginFrame` 是隐式的 image-acquire。当前设计较新但缺乏语义（`BeginFrame` 失败 → return nullptr，没有稳定的 imageIndex）。
- **建议**：RenderGraph V1 之前增补。

### 3.12 [Low] `RHIBufferUsage::TransferSrc/TransferDst` 在 VulkanBackend 有时未传递 memory property

- **相关文件**：[`VulkanUtils.cpp:138-148`](../../Engine/Source/Runtime/RHI/Private/Vulkan/VulkanUtils.cpp)
- **问题描述**：仅转换 usage flags，没检查 VMA 是否要求 `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT`。
- **建议**：Stage 14 优化上传路径时统一处理。

---

## 4. 架构边界问题

- `RHIDevice` 含 `GetVulkanNativeContext` 等暴露点 → 跨 backend 抽象泄漏（已详 3.1）。
- `BindlessResourceManager` 现状保留对 Renderer 内部，未来需严格隔离。
- `RHIResource` 的 owner-device 是 raw pointer，SHARED resource 在多 device 情景下会失真。目前看 RHI 是单 device，问题不显现。

---

## 5. 性能 / 生命周期 / 同步问题

- **Lifetime**：RHI 公共层没有 weak-ptr 风格的 resource-handle。当前依赖 shared_ptr 自我管理，遇到 shadow resource 重建 + frame bind group 持有 raw pointer 的场景就会出问题（详见 3.2）。
- **Sync**：RHICommandList 没有明确的 "frame fence" 概念；`RendererMaxFramesInFlight = 3` 在 `RenderFrameResources.h:19` 写死，但 RHI 自身帧数（`VulkanFrameResources`）没有同源。这是 RHI/Renderer 的耦合 leakage。
- **Recreate 中途**：`ShadowResourceCache::GetOrCreate...` 在分辨率/cascade 改变时直接 reset `m_Directional`，但 `RenderFrameResources` 的 bind group 持有的 view 只在初始化时 set 一次。从 P0-1 安全角度看，shadow 重建期间需要 deinit pipeline/clear bind group。

---

## 6. 坐标 / 数学问题

- 不直接涉及，但 `RHIClipSpace.h::RHIClipSpaceConvention` 已经做了 `DepthZeroToOne/FlipProjectionY/UseInvertedViewportY/DefaultFrontFace`。消费侧是 `ApplyRHIClipSpaceConvention`。这是好的边界。下一步确保 shadow depth range 与 camera depth range 共享同一 convention。

---

## 7. 推荐修改

- 现在就修：
  - **去掉 `RHIDevice` 上所有 Vulkan-specific 函数，挪到 `RHI/Native/` adapter**；
  - 把 `buffer->Update` 收敛到 `RHIUploadManager`；
  - 补 `RHIUtils` 工具；
  - 在 `RHITextureView` 与 frame-bind-group 之间加入 `Reset()`/`Rebind()` 流程，使 shadow 重建安全；
  - `BindlessResourceManager` 标 TODO，禁止被引用；
- Stage 10 DebugDraw 期间修：
  - `RHIRenderOutputDesc::IsValid()` 不变量检查；
  - `MaxTextureArrayLayers` 等扩展 capabilities；
- RenderGraph V1 前修：
  - 多 device 抽象（如未来需要）；
  - swapchain imageIndex 一致性；
- 长期清理：
  - RHISwapchain 接口补齐；
  - bindless 真正实现时；
  - SSBO / storage buffer descriptor 完整化。

---

## 8. 可拆给 Claude Code 的具体任务

1. 把 `RHIDevice.{GetVulkanNativeContext, GetVulkanNativeTextureBinding, RenderVulkanOverlay}` 三个方法从公共层移到 `RHI/Private/Vulkan/VulkanNativeAdapter.h`，并在 `Editor/Private/ImGui/ImGuiVulkanBackend` 改成 dynamic_cast 到 Vulkan adapter。**只重构，不改外部行为**。
2. 在 `RHIUtils` 内部补：`bool IsDepthFormat(RHIFormat)`、`bool IsStencilFormat(RHIFormat)`、`uint32_t GetFormatSizeInBytes(RHIFormat)`、`RHITextureAspectFlags GetAspectMask(RHIFormat)`。**仅添加，不修改现有调用**。
3. 写一组单元测试覆盖 `RHIUtils`（不依赖任何 backend）：
   - `IsDepthFormat(D32Float) == true`；
   - `GetFormatSizeInBytes(RGBA8Unorm) == 4`；
   - `GetAspectMask(D32Float) == RHITextureAspectFlags::Depth`；
4. 改一行：在 `RHITextureView` 默认构造里置 `nullptr`，并在 `BindGroup` 重建时显式 set；并加 debug-time assertion 当 `Texture` 被 reset 时所有引用都更新。**只改头/cpp 注释和默认值，不改现有成员**。
