# 06 Scene / Asset / Serialization 审计

## 1. 审计范围

- `Runtime/Scene/Public/XEngine/Scene/`：`Scene.h`、`Entity.h`、`SceneSystem.h`、`SceneSerializer.h`、`DebugCameraController.h`、`Components.h`、`Components/*.h`
- `Runtime/Scene/Private/`：`Scene.cpp`、`SceneSystem.cpp`、`Entity.cpp`、`Serialization/SceneSerializer.cpp`、`Systems/TransformSystem.{h,cpp}`、`DebugCameraController.cpp`
- `Runtime/Asset/Public/XEngine/Asset/`：`AssetHandle.h`、`AssetSystem.h`、`Assets/MeshAsset.h`、`Assets/MaterialAsset.h`、`Assets/TextureAsset.h`
- `Runtime/Asset/Private/`：`AssetDatabase.cpp`、`AssetManager.cpp`、`AssetRegistry.{h,cpp}`、`AssetSystem.cpp`、`Importers/*`
- `Runtime/Serialization/Public/XEngine/Serialization/`：`JsonSerialization.h`、`SerializationContext.h`、`SerializationVersion.h`
- `Runtime/Serialization/Private/JsonSerialization.cpp`
- 间接消费：`RenderExtraction`、`RenderTextureManager`、`RenderMeshManager`、`RenderMaterialSystem`、`GltfImporter`

---

## 2. 当前优点

- **Scene 完全不依赖 Renderer / RHI / Vulkan**：在 `Scene.h` 中只引用 `AssetHandle`、`CameraComponent`、`MeshRendererComponent`、`TransformComponent`、`LightComponent`、`Entity`。这与提示文档 "Scene 不能依赖 Renderer 或 RHI" 完全一致。
- **Asset public header 不暴露 RHI / Vulkan / Slang / stb / fastgltf**：检查 `MeshAsset.h`、`MaterialAsset.h`、`TextureAsset.h`、`AssetHandle.h`，全部是 CPU-side struct + handle。**这一条完全符合**。
- **Entity 是轻量句柄**：`Entity { u32 Index; u32 Generation; }` + `InvalidEntityIndex`，与 slot-map pattern 一致；死亡/重建不影响其他 Entity。
- **TransformComponent 设计两套矩阵**：`m_LocalMatrix` / `m_WorldMatrix` / `m_PreviousWorldMatrix`，便于 TAA / motion vector 流程。
- **Hierarchy 在 Scene 内表达**：通过 `m_Parents` / `m_Children` / 隐式 SceneRoot，明确"空 grouping entity"也支持。
- **TransformSystem.UpdateRecursive 是层次遍历**：根→孩子递归 + previous/world 矩阵同步；dirty 清理。
- **Asset import 隔离第三方**：`GltfImporter.cpp` 在 `Asset/Private/Importers/` 内部包含 `<fastgltf/...>`，公共头文件无第三方依赖。
- **坐标系转换在 import 边界**：`CoordinateConversion.h` 提供 `GltfPositionToXEngine`、`GltfDirectionToXEngine`、`GltfTangentToXEngine`，并标 `GltfToXEngineFlipsHandedness()` —— 这与提示文档 "Asset import 在导入边界处理外部坐标系转换" 完全吻合。
- **JSON 序列化版本号**：`SerializationVersion.h` 存在；可在 `SceneSerializer` 中读 / 写。

---

## 3. 发现的问题

### 3.1 [High] `EntityRecord::Alive` 默认是 false，但 Scene 没显式 death-sweep

- **相关文件**：[`Scene.h:72-92`](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Scene.h)、`Scene.cpp`、`Entity.cpp`
- **问题描述**：`EntityRecord { Generation = 0; Alive = false; Name; }`。`Scene::CreateEntity` 应该走 slot-map，但未读实现，可能存在以下问题：
  - `Index = InvalidEntityIndex` 时 `EntityRecord.Alive=false`；
  - 但 `m_EntityRecords` 用 `std::vector<EntityRecord>` 是 vector-of-struct，可能 grow 后没有 compaction；
  - `DestroyEntity` 把 `Alive=false`、`Generation++`，但 `Generation` 上溢？
- **推荐修复方式**：阅读 `Scene.cpp` 确认 Create/Destroy 行为，最好写一组 unit test：
  - Create/Destroy 100 次，同一 Index 多次 generation；
  - `IsValid` 在 Destroy 后 false；
  - `GetTransform(InvalidEntity)` 返回 nullptr。
- **建议**：P0-1。

### 3.2 [High] `Entity` 的 hash / unordered_set 兼容性

- **相关文件**：[`Entity.h`](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Entity.h)
- **问题描述**：`Entity` 没有自定义 hash，不能直接进 `std::unordered_set<Entity>`。当前 Scene 用 `std::unordered_map<u32, Entity>`（按 Index），所以避开了 hash；但 Children 列表是用 `std::vector<Entity>`，没问题。但若未来想做 `unordered_set<Entity>` 时会缺。
- **推荐修复方式**：补 `std::hash<Entity>`。
- **建议**：Stage 12 之前。

### 3.3 [High] `MeshRendererComponent` 同时持 `MeshAsset` 与 `MaterialAsset`，但 `MeshSubmesh::MaterialSlot` 是 placeholder

- **相关文件**：[`MeshRendererComponent.h`](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Components/MeshRendererComponent.h)、[`MeshAsset.h:23-30`](../../Engine/Source/Runtime/Asset/Public/XEngine/Asset/Assets/MeshAsset.h)
- **问题描述**：
  - `MeshSubmesh::MaterialSlot` 是 u32 placeholder；
  - `MeshRendererComponent` 一次性持有"整个 mesh"的 1 个 material asset。这意味着 multi-submesh 的 mesh 必须用同 material，**错误**。
  - glTF 标准下，每个 submesh 应能绑定独立 material；现在 architecture 不支持。
- **为什么重要**：实际场景会要求一个 mesh 里多 submesh 各用不同 material（车头是金属、车窗是玻璃）。当前架构要么逼用户把不同 material 的物体拆 entity，要么错乱使用。
- **推荐修复方式**：
  - `MeshRendererComponent` 持 `std::vector<AssetHandle> MaterialSlots;`，长度与 mesh submesh 数匹配；
  - `MaterialSlot` 字段保留以便 asset 查询。
- **建议**：Stage 10 之前修。

### 3.4 [High] `LightComponent` 与 `RenderLight` 类型映射不清晰

- **相关文件**：[`LightComponent.h`](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Components/LightComponent.h)、[`RenderExtraction.cpp:16-28`](../../Engine/Source/Runtime/Renderer/Private/Scene/RenderExtraction.cpp)
- **问题描述**：
  - `LightComponent::Type ∈ { Directional, Point, Spot }`；
  - `LightComponent` 不持 `Position` / `Rotation`（来自 `TransformComponent`），也没有 `Direction`；
  - `RenderExtraction::ConvertLightType` 在 `default` 支返回 `Spot`，逻辑反直觉。
  - 同时：`RenderLight::Position = transform.GetWorldPosition()`、`DirectionToLight = -forward`。这暗含了对 entity transform 的依赖；如果 entity 有 directional light 但**没有 TransformComponent**，则 `direction = -X`（即世界 -X），而不是"光永远向下" —— 这是 silent bug。
- **推荐修复方式**：
  - `LightComponent` 中加 `bool UseEntityTransform = true` 默认，允许 light 自带 direction（没有 transform 时默认 +X forward / -X direction-to-light）；
  - `ConvertLightType` 的 default 行为改为 log warn。
- **建议**：P0-1。

### 3.5 [High] `Scene::SetParent`/`ClearParent`/`GetChildren` 没有强制 TransformComponent 必须存在

- **相关文件**：[`Scene.h:51-57`](../../Engine/Source/Runtime/Scene/Public/XEngine/Scene/Scene.h)
- **问题描述**：parent/children 关系存于 `m_Parents` / `m_Children` map，但 `TransformSystem::UpdateRecursive` 中 `transform == nullptr` 时不递归（用 `childParentTransform` 兜底）。这是 OK 但是 silent：entity A 作 parent 但没有 TransformComponent，下面的 children 仍能继承"world = local"，OK。
- **建议**：Stage 12 维持。

### 3.6 [High] `AssetHandle` 是 `u64` 不带类型 tag

- **相关文件**：[`AssetHandle.h`](../../Engine/Source/Runtime/Asset/Public/XEngine/Asset/AssetHandle.h)
- **问题描述**：`AssetHandle` 是 `u64`。`MeshRendererComponent.MeshAsset = AssetHandle` 与 `MaterialAsset` 同类型。若未来有 typed asset handle，应可在编译期阻挡。
- **推荐修复方式**：Stage 12 之内用强类型 `Handle<class MeshAssetTag>;` 替换。
- **建议**：Stage 12。

### 3.7 [High] `AssetRegistry.h` 私有，不是公共 API

- **相关文件**：[`Runtime/Asset/Private/AssetRegistry.{h,cpp}`](../../Engine/Source/Runtime/Asset/Private/AssetRegistry.h)
- **问题描述**：私有的好。但 private/Public 边界要确认。
- **建议**：保持。

### 3.8 [High] `MaterialAsset` 的结构与 `RenderMaterialSystem` 之间字段映射不清

- **相关文件**：`MaterialAsset.h`、`RenderMaterialSystem.{h,cpp}`、`MaterialImporter.cpp`
- **问题描述**：MaterialAsset 与 GPU Material data 的映射未审完。这是一个关键缝隙，需要补审计。
- **建议**：Stage 10 DebugDraw 之前补审计。

### 3.9 [High] `GltfImporter` 与 `MaterialImporter` / `TextureImporter` 之间的强制依赖

- **相关文件**：[`GltfImporter.cpp`](../../Engine/Source/Runtime/Asset/Private/Importers/GltfImporter.cpp)、[`MaterialImporter.cpp`](../../Engine/Source/Runtime/Asset/Private/Importers/MaterialImporter.cpp)、`TextureImporter.cpp`、`ImageImporter.cpp`、`StbImageImporter.cpp`
- **问题描述**：
  - glTF 的 Image / Sampler / Texture 与基础材质之间耦合：read image accessor → 写 texture asset → 写 material asset。
  - 当前有 `ImageImporter` 与 `StbImageImporter` 两条路径，可能存在竞争：哪些 texture 是 glTF inline image vs external。
- **推荐修复方式**：用 `ImageImporter::Import(...)` 路径，并且让 `TextureImporter::ImportFromImageData(...)` 处理 format / sRGB / mip。
- **建议**：Stage 11 PBR Test 期间修。

### 3.10 [High] `AABB::TransformAABB` 在 Skinned Mesh / Multi-Mesh 上失效

- **相关文件**：`MeshAsset.h`、`RenderExtraction.cpp`
- **问题描述**：
  - `meshAsset->Bounds` 是 local-space AABB；
  - `RenderObject::WorldBounds = Math::TransformAABB(asset->Bounds, transform.GetWorldMatrix())`；
  - 这个 AABB 是 conservative axis-aligned，skin / morph 后真实顶点可能超出。
- **推荐修复方式**：用 8 corners compute，或等 Stage 13 GPU scene 时换 GPU-driven culling。
- **建议**：Stage 13。

### 3.11 [High] `JsonSerialization` 用 `nlohmann/json`

- **相关文件**：[`Serialization/Public/XEngine/Serialization/JsonSerialization.h`](../../Engine/Source/Runtime/Serialization/Public/XEngine/Serialization/JsonSerialization.h)
- **问题描述**：nlohmann/json 是重量级头文件；如果每个 TU 都包含会引入几千行模板与编译时长。
- **推荐修复方式**：用 pimpl 隐藏 nlohmann 在 .cpp 中，让公共头只暴露抽象。
- **建议**：Stage 14 编译时间优化。

### 3.12 [Medium] `SceneSerializer` 单元测试缺

- **相关文件**：`SceneSerializer.cpp`
- **问题描述**：未审完。应当 round-trip 测试：(a) 创建 scene with 多种 component；(b) Serialize to JSON；(c) Parse back；(d) 检查 equal。
- **建议**：Stage 12 加 CTest。

### 3.13 [Medium] `MaterialImporter` 是否覆盖 glTF full PBR (metallic-roughness)

- **相关文件**：`MaterialImporter.cpp`
- **问题描述**：未审完。glTF 实际 PBR 是 metal-roughness 或 spec-gloss，需要确认 importer 输出 `GPUMaterialData` 字段（BaseColorFactor、MetallicFactor、RoughnessFactor、NormalScale 等）。
- **建议**：Stage 11 PBR Test。

### 3.14 [Medium] `DebugCameraController` 与 Editor camera 不分离

- **相关文件**：`Scene/Private/DebugCameraController.cpp`
- **问题描述**：Debug Camera 是 Scene 私有，可能与 EditorEditorCamera 重复。审计 Editor 之后再交叉引用。
- **建议**：Stage 10 之前清理。

### 3.15 [Medium] `SceneSystem` 不持有 RenderExtraction 反馈

- **相关文件**：`SceneSystem.h/.cpp`
- **问题描述**：当前 SceneSystem 只暴露 Active Scene；RenderExtraction 自己硬取 Scene，缺少 dirty flag 与 frame-feedback。这导致 Scene 改动但 Renderer 不知道。
- **推荐修复方式**：Scene 提供 Version counter，RenderSystem 在 version 不一致时重新 extraction。
- **建议**：Stage 13。

### 3.16 [Medium] `AssetManager` 与 `AssetRegistry` 区别不清

- **相关文件**：`AssetManager.cpp`, `AssetRegistry.cpp`
- **问题描述**：未审完。需要明确 "AssetManager = Application-level API for Asset load/save" vs "AssetRegistry = on-disk index / metadata"。
- **建议**：Stage 13。

### 3.17 [Medium] `Scene::SetWorldRotationDegrees` 等接口存在但 `Rotator` 单位含糊

- **相关文件**：`Scene.h:60-65`、`Rotator.h`
- **问题描述**：见 Core 审计 3.5。

### 3.18 [Low] `DebugCameraController` 与 `FreeCameraController` (Editor) 重复

- **相关文件**：`Scene/Private/DebugCameraController.cpp` vs `Editor/Private/FreeCameraController.cpp`
- **问题描述**：两个非常类似。Stage 10 时合并。
- **建议**：Stage 10 期间。

### 3.19 [Low] `JsonSerialization` 不强制版本回滚 / migration

- **相关文件**：`SerializationVersion.h`、`SceneSerializer.cpp`
- **问题描述**：版本号存在，但当前没有 "old version -> new version" 迁移逻辑。当 Serializer version 升到 2 时，旧存档丢失。
- **推荐修复方式**：写一个 `SceneMigration::Migrate_v1_to_v2(json)` 等。
- **建议**：Stage 14。

---

## 4. 架构边界问题

- **Scene 不依赖 Renderer/RHI** ✓
- **Asset public header 不暴露第三方** ✓
- **Private/Public 边界**：AssetRegistry 在 Private；ImageImporter、StbImageImporter 等都在 Private。OK。
- **`Scene/Components` 没有 `RHI / Renderer` include** ✓
- **未审计完整**：`SceneSerializer.cpp` 没读完；`MaterialImporter.cpp` 没读完；`TextureImporter.cpp`、`ImageImporter.cpp` 没读完。

---

## 5. 性能 / 生命周期 / 同步问题

- **Scene 频繁 Clear**：Editor 切换 Scene 时直接 `Scene.Clear()`。所有 EntityRecord / Transform / Component 都重新分配。当 Scene 大时该操作慢。
- **TransformSystem** 是 `O(N)` 递归，每次 `Update` 都遍历所有 entity；适合 Stage 12 加 dirty subtree。
- **Asset Import** 是 CPU 阻塞；当前没有 async import。预计 Stage 14 引入 JobSystem 异步。
- **Asset Handle** 是 u64，生命周期由 AssetManager 保证；如果 AssetManager 释放而 user 仍持 handle 会出现 dangling。需要做出 "Asset pointer stable" 或 "AssetManager returns std::shared_ptr<const Asset>"。

---

## 6. 坐标 / 数学问题

- **`TransformComponent::GetWorldRotation()`**：从 `m_WorldRotation` 拿（quat），`RenderExtraction` 调 `GetForwardVector(m_WorldRotation)` 得 +X forward —— 这是 XEngine 约定。OK。
- **`SetWorldRotationDegrees(Rotator)`**：Rotator 没单位注释；潜在 bug。
- **DirectionToLight** 是 `-forward`，shader 端 `GPULight.DirectionType` 拿这个值作为 "to light direction" —— OK。
- **glTF coordinate conversion**：在 importer 边界做。所有 mesh / animation 数据进入 XEngine 后都是 LH+ +X forward + +Z up。

---

## 7. 推荐修改

- 现在就修：
  - **3.3** `MaterialSlots` 多 submesh support；
  - **3.4** `LightComponent` 没有 TransformComponent 时 default direction；
  - **3.1** Scene slot-map 单元测试。
- Stage 10 DebugDraw 期间修：
  - **3.6** `AssetHandle` 强类型；
  - **3.14** Debug Camera 与 Editor Camera 合并；
  - **3.17** Rotator 单位显式。
- RenderGraph V1 前修：
  - **3.8** MaterialImporter 与 RenderMaterialSystem 字段对齐；
  - **3.13** glTF PBR metal-roughness 完整；
- 长期清理：
  - **3.11** nlohmann/json pimpl；
  - **3.16** AssetManager vs AssetRegistry 分离；
  - **3.19** Scene migration。

---

## 8. 可拆给 Claude Code 的具体任务

1. 在 `MeshRendererComponent` 加 `std::vector<AssetHandle> MaterialSlots;`，并提供 `void SetMaterialSlot(u32 submeshIndex, AssetHandle handle)`、`AssetHandle GetMaterialSlot(u32 submeshIndex) const`，并保持 `MaterialAsset` 旧字段 deprecated 但保留（仅 Renderer 内 fallback）。**仅添加**。
2. 在 `LightComponent` 加 `bool UseEntityTransform = true; bool HasExplicitDirection = false; Vec3 ExplicitDirection { 0, 0, -1 };`，并在 `RenderExtraction` 中应用：若 `HasExplicitDirection=true` 则 `DirectionToLight = Normalize(ExplicitDirection)`。**仅添加字段与 RenderExtraction 分支**。
3. 写单元测试 `SceneSlotMapTests`：创建 100 entity，destroy 50，再 create 50；确认 generation 正确、IsValid 正确、无悬挂 pointer。**只测试；不改 Scene.cpp 行为除非测试失败**。
4. 在 `AssetHandle.h` 加 `using MeshAssetHandle = AssetHandle<class MeshAssetTag>;` 之类 typed alias，并保留 `using AssetHandle = u64;` deprecated。**仅添加**。
