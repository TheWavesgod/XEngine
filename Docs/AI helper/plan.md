# XEngine RHI 重构背景提要

## 1. 项目背景

XEngine 使用 C++20，目前主要图形后端是 Vulkan，未来计划支持：

* Vulkan
* Direct3D 12
* Metal

现有 RHI 是一个已经实现并投入使用过的版本，但经过进一步开发后，当前设计被认为存在较明显的架构问题。

本次工作不是对旧 RHI 做小范围修补，也不是要求保持旧接口兼容，而是一次从底层开始的重新设计。

旧 RHI 代码可以作为 Vulkan 实现细节和已有功能的参考，但不再被视为必须兼容的协议。

---

## 2. 本次重构的核心原则

本次采用推倒式、由下至上的重构方式。

当前只考虑：

* XEngine 的必要基础模块
* 新的 RHI Core
* VulkanRHI 后端
* RHI 单元测试
* VulkanRHI 集成测试
* VulkanRHI Smoke Test

当前不考虑：

* Renderer
* Editor
* Scene
* Asset
* Engine runtime
* Sandbox
* EditorApp
* ImGui Vulkan 集成
* 其他依赖旧 RHI 的模块

这些上层模块在新 RHI 完成并稳定之后，再根据最终协议重新设计和迁移。

因此，在 RHI 重构期间：

* 整个引擎可以无法编译；
* Renderer、Editor、Sandbox 等可以处于不可构建状态；
* 不需要为旧上层代码保留 compatibility wrapper；
* 不需要提供 forwarding header；
* 不需要维持旧 RHI public API；
* 不应该为了旧调用方而扭曲新设计。

---

## 3. 当前已确认的旧架构问题

现有工程中，通用 RHI 接口与 Vulkan 后端实现被编译进同一个 `XEngineRHI` target。

当前审计发现的问题包括：

* `XEngineRHI` 同时编译通用 RHI 与 `Private/Vulkan` 下的实现；
* Vulkan include directories 通过 `PUBLIC` 传播；
* `volk::volk` 通过 `PUBLIC` 链接传播；
* Vulkan 相关编译定义通过 `PUBLIC` 传播；
* VMA、SDL3 和 Vulkan SDK 依赖混入 RHI Core；
* RHI public API 中存在 Vulkan 专属接口；
* `RHIDevice` 暴露 Vulkan native context 和 native binding；
* `RHISystem` 直接包含并创建 VulkanDevice；
* RHI unit tests 因链接 `XEngineRHI` 而被动依赖 Vulkan；
* Core RHI 无法在不安装 Vulkan SDK 的环境中独立构建。

完整审计还确认，当前 Renderer、Asset、Scene 等模块没有大量直接包含 Vulkan 类型；主要 Vulkan 泄漏点集中在 RHI 本身和 Editor 的 ImGui Vulkan 集成。

由于本次不考虑旧上层模块兼容，Editor 和旧调用方的 Vulkan bridge 暂时不属于当前范围。

---

## 4. 新项目边界

目标至少拆分为两个独立模块：

```text
XEngineRHI
XEngineVulkanRHI
```

强制依赖方向：

```text
Foundation / 必要基础 Core
                ↓
           XEngineRHI
                ↓
       XEngineVulkanRHI
```

禁止出现：

```text
XEngineRHI → XEngineVulkanRHI
XEngineRHI → Vulkan SDK
XEngineRHI → volk
XEngineRHI → VMA
XEngineRHI → SDL3
XEngineRHI → Renderer
XEngineRHI → Editor
XEngineRHI → Asset
XEngineRHI → Scene
XEngineRHI → Shader compiler
```

`XEngineRHI` 应当能够在没有 Vulkan SDK 的环境中独立编译。

---

## 5. 建议项目结构

当前先保持简单，不提前建立大量尚无实际职责的子目录：

```text
Engine/
├── Source/
│   └── Runtime/
│       ├── RHI/
│       │   ├── Public/
│       │   │   └── XEngine/RHI/
│       │   ├── Private/
│       │   └── CMakeLists.txt
│       │
│       └── VulkanRHI/
│           ├── Public/
│           │   └── XEngine/VulkanRHI/
│           ├── Private/
│           └── CMakeLists.txt
│
└── Tests/
    ├── Unit/
    │   └── RHI/
    ├── Integration/
    │   └── VulkanRHI/
    ├── Smoke/
    │   └── VulkanRHI/
    └── TestSupport/
```

不要仅为了目录整齐而提前创建：

* Common
* Services
* Instance
* Adapter
* Device
* Resources
* Pipeline
* Synchronization

这些目录应当在相关职责真正确定后再创建。

---

## 6. 测试结构

测试按层级组织，而不是全部堆在一个 RHI 测试目录中。

### 6.1 Unit Tests

目标：

```text
XEngineRHIUnitTests
```

依赖：

```text
XEngineRHI
GoogleTest
CTest
```

不得依赖：

* Vulkan SDK
* XEngineVulkanRHI
* volk
* VMA
* SDL3
* Renderer
* Editor
* Asset
* Scene

Unit tests 用于验证纯协议和纯 CPU 逻辑，例如：

* enum flags
* Result/Error 语义
* descriptor validation
* format classification
* subresource range
* adapter scoring
* capability matching
* command state machine
* binding compatibility

### 6.2 Integration Tests

目标：

```text
XEngineVulkanRHIIntegrationTests
```

依赖：

```text
XEngineRHI
XEngineVulkanRHI
GoogleTest
Vulkan backend private dependencies
```

用于验证真实 GPU 行为，但尽量不依赖窗口：

* Vulkan instance 创建
* adapter 枚举
* device 创建
* buffer 创建
* upload/copy/readback
* texture 创建
* command submission
* synchronization
* compute dispatch
* offscreen rendering
* Vulkan validation layer 错误

### 6.3 Smoke Test

目标：

```text
XEngineVulkanRHISmokeTest
```

这是独立 executable，用于验证完整最小运行链：

* window
* surface
* swapchain
* clear
* draw
* present
* resize
* frames in flight
* shutdown

Smoke Test 不依赖正式 Renderer。

---

## 7. 测试工具

使用：

```text
GoogleTest
CTest
```

GoogleTest 用于：

* 编写测试；
* fixture；
* parameterized test；
* assertions；
* 失败定位。

CTest 用于：

* 从 CMake 中注册测试；
* 统一运行所有测试程序；
* 汇总结果；
* 接入 CI。

GoogleMock 可以随 GoogleTest 提供，但当前不计划大量使用。

优先级：

```text
真实纯逻辑测试
> 简单手写 fake
> 少量 gmock
> 不 mock Vulkan API
```

不应逐函数 mock `vkCreateImage`、`vkAllocateMemory` 等 Vulkan 调用。Vulkan 后端应通过真实 integration test 和 validation layer 验证。

---

## 8. RHI 的设计定位

新 RHI 应当是现代显式图形 API 的共同抽象，而不是 Vulkan API 的重命名。

它应表达 Vulkan、D3D12 和 Metal 可以合理共享的 GPU 概念，例如：

* Instance
* Adapter
* Device
* Queue
* CommandList
* Buffer
* Texture
* TextureView
* Sampler
* Shader
* BindGroupLayout
* BindGroup
* PipelineLayout
* ComputePipeline
* GraphicsPipeline
* Fence
* Semaphore
* Resource state
* Barrier
* Surface
* Swapchain

但不应表达：

* VkRenderPass
* VkFramebuffer
* VkDescriptorSet
* VkImageLayout
* VkQueueFamilyIndex
* D3D12 descriptor heap
* Metal argument buffer 细节
* Renderer pass
* Material
* Shadow
* CSM
* RenderGraph
* Scene
* Asset import

---

## 9. 后端选择与设备约束

已经确认以下产品约束：

* 用户可以选择图形后端；
* 用户可以选择 Adapter；
* 默认选择性能最强且满足要求的 Adapter；
* 可以枚举多个 Adapter；
* 整个运行时只允许创建并使用一个 RHIDevice；
* 不支持多 Device；
* 不支持 multi-GPU；
* 不支持跨 Device 资源共享；
* 运行期间不支持切换 Device；
* 切换 backend 或 GPU 需要重新初始化应用。

默认 Adapter 选择不是简单选择显存最大的设备，而应首先满足 required capabilities，再根据策略评分。

未来可支持：

```text
Automatic
HighPerformance
LowPower
Explicit
```

但当前只需围绕单 Device 模型设计协议。

---

## 10. 后端实现模式

推荐采用：

* C++ abstract interface；
* backend `final` derived classes；
* RAII；
* 启动阶段使用 factory 选择后端；
* 运行阶段通过虚接口调用；
* 不在每个渲染调用中判断 backend。

例如：

```text
RHI abstract interface
        ↑
Vulkan implementation
D3D12 implementation
Metal implementation
```

不计划在当前阶段采用完整 handle-based RHI。

原因是 handle-based 模型会立即要求：

* generation handle
* resource registry
* handle pool
* centralized destruction
* backend lookup
* lifetime validation

当前更适合 typed C++ object interface。

创建接口可以使用 NVI：

```text
public non-virtual validation
        ↓
protected virtual backend implementation
```

高频 CommandList API 是否使用完整 NVI，需要后续按性能和验证需求决定。

后端内部不应到处使用 `dynamic_cast`。可以使用集中式、带 owner-device 检查的 `static_cast` helper。

---

## 11. 资源所有权原则

已经确认：

* RHI 管理底层 GPU/native object；
* RHI 管理 device ownership；
* RHI 管理 GPU-safe destruction；
* Renderer 以后负责语义层资源所有权、缓存和复用；
* 资源属于创建它的 Device；
* 不允许跨 Device 使用资源；
* 当前为单 Device 模型。

Texture 与 TextureView 必须是独立对象：

```text
RHITexture
RHITextureView
```

不采用：

* Texture 内置默认 view；
* Texture 自己缓存所有 view；
* Renderer 通过 Vulkan native view 操作资源。

Buffer 当前不设计通用 `RHIBufferView`，优先使用：

```text
Buffer + offset + size
```

只有未来出现明确跨后端语义时，再评估 BufferView。

---

## 12. 服务层边界

下列功能应当建立在 RHI Core 之上，而不属于最小 GPU 协议本身：

* UploadManager
* DeferredDeletion
* StagingBufferAllocator
* DescriptorAllocator
* PipelineCache

例如 UploadManager 应使用：

* Buffer
* CommandList
* Queue
* Fence

来实现 CPU 到 GPU 上传，而不是成为 Device 基础协议的一部分。

这些服务等核心资源、命令和同步协议稳定后再实现。

---

## 13. 推荐实现顺序

不要一次性设计完整 RHI。

每一个阶段都遵循：

```text
讨论协议
→ 写 Unit Tests
→ 实现 RHI Core
→ 实现 VulkanRHI
→ 写 Integration Tests
→ 验证
→ 再进入下一阶段
```

推荐里程碑：

### M0：工程与测试骨架

* 独立 `XEngineRHI`
* 独立 `XEngineVulkanRHI`
* Unit / Integration / Smoke targets
* RHI 不依赖 Vulkan

### M1：基础类型

* namespace
* export macro
* backend enum
* Result/Error
* flags
* debug naming
* 基础 object/lifetime 规则

### M2：Instance 与 Adapter

* RHIInstance
* RHIInstanceDesc
* RHIAdapter
* RHIAdapterInfo
* RHIAdapterPreference
* adapter selection
* 单 Device 创建约束

### M3：Device、Queue 与 Capabilities

* RHIDevice
* RHIDeviceDesc
* RHIQueue
* required/optional features
* supported capabilities 与 enabled capabilities 分离

### M4：Buffer、Copy 与 Readback

* RHIBuffer
* RHIBufferDesc
* memory usage
* command recording
* queue submit
* fence
* buffer upload/copy/readback

这是第一个真实 GPU 数据闭环。

### M5：Texture、TextureView 与 Sampler

* RHITexture
* RHITextureView
* RHISampler
* formats
* usage
* mip/layer/subresource
* upload/readback

### M6：Barrier 与同步

* resource state
* barriers
* fence
* semaphore
* command state machine

RHI 保持显式同步，不自动猜测 Renderer 的所有资源状态。

### M7：Shader 与 Binding

* shader bytecode
* entry point
* shader stage
* BindGroupLayout
* BindGroup
* PipelineLayout

RHI 不依赖 Slang。Shader 模块以后向 RHI 提供已编译 bytecode。

### M8：Compute Pipeline

先做 Compute Pipeline，再做 Graphics Pipeline。

建立：

```text
Buffer
→ Upload
→ Bind
→ Dispatch
→ Readback
```

这是验证资源、命令、同步、Shader、Binding 和 Pipeline 的最佳早期闭环。

### M9：Graphics Pipeline 与离屏渲染

* graphics pipeline state
* dynamic rendering 风格 attachment model
* offscreen clear
* triangle draw
* texture readback
* pixel validation

不要先引入 swapchain。

### M10：Surface、Swapchain 与 Present

* window integration
* surface
* swapchain
* acquire
* present
* resize
* frames in flight

### M11：Services

* UploadManager
* DeferredDeletion
* staging allocation
* descriptor allocation
* pipeline cache

### M12：迁移上层模块

新 RHI 稳定之后，再依次迁移：

* Renderer
* Shader integration
* Editor
* ImGui backend
* Asset
* Engine
* Apps

迁移时以上层需求验证协议，但不重新把 Vulkan 类型泄漏进通用模块。

---

## 14. 当前非目标

目前明确不做：

* RHILoader 独立模块；
* BackendRegistry；
* 动态后端插件；
* stable C ABI；
* DLL ABI 设计；
* D3D12RHI 空壳；
* MetalRHI 空壳；
* 多 GPU；
* 多 Device；
* bindless 完整系统；
* RenderGraph；
* Renderer resource manager；
* Material；
* Shadow/CSM；
* 完整 performance framework；
* Vulkan 函数级 mock；
* 为旧 Renderer 保留兼容接口。

后端当前可以作为独立静态 target。

未来若确有动态链接需求，再在已稳定的模块边界上增加 loader 和 ABI 层。

---

## 15. 与 Claude Code 协作要求

Claude Code 在每个阶段开始前应：

1. 阅读本文件；
2. 检查仓库实际代码和 CMake；
3. 对比当前实现与本阶段目标；
4. 给出小范围设计方案；
5. 列出预计修改文件；
6. 等设计确认后再实施，除非任务明确要求直接执行。

实施时应：

* 每次只处理一个里程碑；
* 不自动补全后续全部 RHI；
* 不因旧代码存在而保留错误协议；
* 不修改无关模块；
* 不进行无关格式化；
* 不自行引入 loader、registry 或插件系统；
* 不为了“全引擎编译成功”修改上层；
* 每个阶段增加对应测试；
* 每个阶段输出构建和测试结果；
* 遇到设计分歧时明确列出选择和影响，不自行做重大架构决定。

每个任务结束时应输出：

* 修改文件列表；
* 当前接口摘要；
* target 依赖；
* 测试覆盖；
* 构建结果；
* 已知限制；
* 下一阶段建议；
* 然后停止，不继续实现下一阶段。

---

## 16. 当前下一步

当前建议从 M0 开始：

```text
建立新的 XEngineRHI 与 XEngineVulkanRHI 边界
建立独立测试闭环
排除旧 RHI 实现和旧上层模块
验证 RHI Core 不依赖 Vulkan
```

M0 完成后，正式讨论 M1：

```text
RHI 基础类型
Result/Error
RHIObject 是否需要
对象生命周期
debug naming
flags
public header 规则
```

未经讨论，不应直接开始实现 Instance、Adapter 或 Device。
