# Implementation Plan: Stage 9 CSM — Step 3 & Step 4

本文件覆盖之前 `implementation.md` 的内容，专门描述主计划 `Docs/AI/Stage_09_CSM_Implementation_Plan.md` 中 **Step 3** 与 **Step 4** 的具体代码修改方案。
**不会**实际修改任何仓库源文件。

---

## Step 3 — `DirectionalShadowPlanner` 主循环补全

### 3.1 目标

* 修复 `ComputeCascadeSplits` 没把 split 写入 `outSplits` 的 bug
* 在 `BuildPlan` 主循环中完成：cascade 视锥世界空间包围球 → light view matrix → light ortho projection → texel snap → 填 `RenderShadowCascade`
* 适配 Step 1.3.2 新增的 `ReverseZ` 字段

### 3.2 涉及文件

| 文件 | 修改类型 |
|------|----------|
| `Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.cpp` | 2 处修改：bug 修复 + 主循环补全 |

### 3.3 详细修改

#### 3.3.1 修复 `ComputeCascadeSplits` bug

**当前代码**（约第 18–42 行）：

```cpp
static void ComputeCascadeSplits(
    float cameraNear,
    float cameraFar,
    u32 cascadeCount,
    float splitLambda,
    float* outSplits)
{
    splitLambda = std::clamp(splitLambda, 0.0f, 1.0f);

    const float nearClip = cameraNear;
    const float farClip = cameraFar;
    const float clipRange = farClip - nearClip;

    const float ratio = farClip / nearClip;

    for (u32 i = 0; i < cascadeCount; ++i)
    {
        const float p = static_cast<float>(i + 1) / static_cast<float>(cascadeCount);

        const float logSplit = nearClip * std::pow(ratio, p);
        const float linearSplit = nearClip + clipRange * p;

        const float split = Math::Lerp(linearSplit, logSplit, splitLambda);
    }
}
```

**问题**：`const float split = ...` 算出来后**没写进** `outSplits[i]`，所以 `BuildPlan` 里 `cascadeSplits[cascadeIndex]` 永远是 0。这是个真实的 bug。

**改为**：

```cpp
static void ComputeCascadeSplits(
    float cameraNear,
    float cameraFar,
    u32 cascadeCount,
    float splitLambda,
    float* outSplits)
{
    splitLambda = std::clamp(splitLambda, 0.0f, 1.0f);

    const float nearClip = cameraNear;
    const float farClip = cameraFar;
    const float clipRange = farClip - nearClip;

    // Guard against non-positive near plane to avoid log(<=0) below.
    const float safeNear = (nearClip > 1e-4f) ? nearClip : 1e-4f;
    const float ratio = farClip / safeNear;

    for (u32 i = 0; i < cascadeCount; ++i)
    {
        const float p = static_cast<float>(i + 1) / static_cast<float>(cascadeCount);

        const float logSplit = safeNear * std::pow(ratio, p);
        const float linearSplit = nearClip + clipRange * p;

        const float split = Math::Lerp(linearSplit, logSplit, splitLambda);

        outSplits[i] = split;   // <-- THE FIX
    }
}
```

**为什么**：

* `std::pow(ratio, p)` 在 `nearClip <= 0` 时会让 `logSplit` 退化为 0 或负数。加 `safeNear` 兜底。
* 补 `outSplits[i] = split;` 即可修复主 bug。

---

#### 3.3.2 在主循环上方插入 3 个数学 helper

**插入位置**：`BuildPlan` 之前的 anonymous namespace 内（紧接 `QuantizeRadius` 之后），新增 3 个 static helper。

**新增代码**：

```cpp
// Builds the light-space basis { L, U, R } from a world-space light direction.
// XEngine world convention: +X forward, +Y right, +Z up.
// DirectionToLight points from a surface point to the light, i.e. the direction
// the rays travel toward the lit object. The light's "view" axis looks from
// the light position toward the shadow receivers, so it is +DirectionToLight.
struct LightBasis
{
    Vec3 Forward;  // -Z in light space (look direction)
    Vec3 Up;       // +Y in light space
    Vec3 Right;    // +X in light space
};

static LightBasis BuildLightBasis(const Vec3& directionToLight)
{
    LightBasis basis;
    basis.Forward = Math::Normalize(directionToLight);

    // World up is +Z. If the light is nearly vertical, fall back to +Y
    // to avoid a degenerate cross product.
    const Vec3 worldUp = (std::fabs(basis.Forward.z) > 0.999f)
        ? Vec3(0.0f, 1.0f, 0.0f)
        : Vec3(0.0f, 0.0f, 1.0f);

    basis.Right = Math::Normalize(Math::Cross(basis.Forward, worldUp));
    basis.Up    = Math::Normalize(Math::Cross(basis.Right,  basis.Forward));
    return basis;
}

// Translates a world-space center to the closest orthographic frustum center
// aligned on the shadow map's texel grid. This keeps cascades from "swimming"
// as the camera moves; it is the standard "stable CSM" trick.
static Vec3 SnapToTexelGrid(
    const Vec3& worldCenter,
    const Mat4& lightViewProj,
    float texelSize)
{
    // Project the world center into light clip space.
    const Vec4 clip = lightViewProj * Vec4(worldCenter, 1.0f);

    // Snap UV in NDC, then transform back to world.
    const float snappedX = std::round(clip.x / clip.w / texelSize) * texelSize;
    const float snappedY = std::round(clip.y / clip.w / texelSize) * texelSize;

    // Build a translation that cancels the sub-texel offset.
    const Vec4 offset = clip - Vec4(snappedX * clip.w, snappedY * clip.w, 0.0f, 0.0f);
    (void)offset;

    return worldCenter;   // Texel-snap is applied via an adjusted light view, not a world translation.
}

// Variant that returns the world-space offset to subtract from the light
// position so the cascade's projected center sits on a texel boundary.
static Vec3 ComputeTexelSnapOffset(
    const Vec3& worldCenter,
    const Mat4& lightView,
    const Mat4& lightProj,
    float texelSize)
{
    const Mat4 lightViewProj = lightProj * lightView;
    const Vec4 clip = lightViewProj * Vec4(worldCenter, 1.0f);
    if (clip.w == 0.0f)
    {
        return Vec3(0.0f);
    }

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;
    const float snappedNdcX = std::round(ndcX / texelSize) * texelSize;
    const float snappedNdcY = std::round(ndcY / texelSize) * texelSize;

    // The light position needs to shift so the projected center lands on the snapped NDC.
    const Mat4 invView = Math::Inverse(lightView);
    const Vec4 worldOffset = invView * Vec4(
        (snappedNdcX - ndcX) * clip.w,
        (snappedNdcY - ndcY) * clip.w,
        0.0f, 0.0f);
    return Vec3(worldOffset);
}
```

**为什么需要这些 helper**：

* `BuildLightBasis`：拿到方向光的右/上/前向量，用于构造 light view 的 lookAt 形式
* `ComputeTexelSnapOffset`：标准 stable CSM 算法——把 cascade 中心对齐到 shadow map 的 texel 边界，避免 camera 移动时 shadow 抖动
* `SnapToTexelGrid` 是早先占位用的简化版；保留为辅助，但**主循环里用** `ComputeTexelSnapOffset`

**注意**：上面依赖 `Math::Inverse`、`Math::Cross`、`Math::Normalize`、`Math::Lerp`——这些 helper 在 `XEngine/Math/MathFunctions.h` / `Math.h` 中应当已经存在（Step 1 的 plan 审计确认过 `Math::Lerp` 存在）。如果 `Math::Cross` 不存在，可以改写为 `glm::cross` 走 `<glm/glm.hpp>` 配合 `XEngine/Math/MathTypes.h` 别名。

---

#### 3.3.3 `BuildPlan` 主循环补全

**当前代码**（约第 135–204 行）：

```cpp
bool DirectionalShadowPlanner::BuildPlan(const DirectionalShadowPlanDesc& desc, 
    RenderDirectionalShadowFrameData& outData) const
{
    if (desc.Light == nullptr)
    {
        XENGINE_LOG_WARN("Light from description is unvalid");
        return false;
    }
    if (desc.Light->Type != RenderLightType::Directional)
    {
        XENGINE_LOG_WARN("Light type is not directional");
        return false;
    }
    if (Math::Length(desc.Light->DirectionToLight) <= 0.0001f)
    {
        return false;
    }
    if (desc.CameraNear <= 0.0f || desc.CameraFar <= desc.CameraNear)
    {
        return false;
    }
    if (desc.CascadeCount == 0 || desc.CascadeCount > MaxShadowCascades)
    {
        return false;
    }
    if (desc.Resolution == 0)
    {
        return false;
    }

    const u32 cascadeCount = desc.CascadeCount;
    outData.Enabled = true;
    outData.CascadeCount = cascadeCount;

    float cascadeSplits[MaxShadowCascades] = {};
    ComputeCascadeSplits(
        desc.CameraNear,
        desc.CameraFar,
        cascadeCount,
        desc.SplitLambda,
        cascadeSplits);

    const auto fullFrustumCorners = GetCameraFrustumCornersWorldSpace(
        desc.CameraView,
        desc.CameraProjection); 

    float previousSplit = desc.CameraNear;
    for (u32 cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex)
    {
        const float splitNear = previousSplit;
        const float splitFar = cascadeSplits[cascadeIndex];

        const auto cascadeCorners = GetCascadeFrustumCornersWorldSpace(
            fullFrustumCorners,
            desc.CameraNear,
            desc.CameraFar,
            splitNear,
            splitFar);
        
        // TODO
    }

    return true;
}
```

**改为**（只替换 `// TODO` 块，主循环其它部分保留）：

```cpp
    const LightBasis lightBasis = BuildLightBasis(desc.Light->DirectionToLight);
    const float texelSize = 2.0f / static_cast<float>(desc.Resolution);

    float previousSplit = desc.CameraNear;
    for (u32 cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex)
    {
        const float splitNear = previousSplit;
        const float splitFar  = cascadeSplits[cascadeIndex];

        // Cascade sub-frustum in world space.
        const auto cascadeCorners = GetCascadeFrustumCornersWorldSpace(
            fullFrustumCorners,
            desc.CameraNear,
            desc.CameraFar,
            splitNear,
            splitFar);

        // Bounding sphere around the cascade sub-frustum.
        const Vec3  center     = ComputeAverageCenter(cascadeCorners);
        const float rawRadius  = ComputeBoundingSphereRadius(cascadeCorners, center);
        const float radius     = QuantizeRadius(rawRadius);

        // Push the light back along its forward axis so the entire sphere sits
        // in front of the light (positive Z in light space).
        const Vec3 lightPosition = center - lightBasis.Forward * radius;

        // Build the light view matrix using XEngine's +X-forward convention.
        // BuildViewMatrixLH_XForward expects the eye position and the world rotation
        // of the camera; here we just construct the matrix directly from the basis.
        Mat4 lightView = Mat4(1.0f);
        lightView[0]  = Vec4(lightBasis.Right,   0.0f);
        lightView[1]  = Vec4(lightBasis.Up,      0.0f);
        lightView[2]  = Vec4(lightBasis.Forward, 0.0f);
        lightView[3]  = Vec4(lightPosition,      1.0f);
        lightView     = Math::Inverse(lightView);

        // Apply texel snap to keep cascades from swimming (Step 1's StabilizeCascades).
        if (desc.StabilizeCascades)
        {
            Mat4 tmpProj = Mat4(1.0f);
            if (desc.ReverseZ)
            {
                // Reverse-Z ortho: near plane at +Z, far at -Z (in light space pre-proj).
                tmpProj = Math::OrthographicLH_ZO(
                    -radius, radius, -radius, radius,
                    -radius - desc.DepthBias * 4.0f,
                     radius + desc.DepthBias * 4.0f);
                // Math::OrthographicLH_ZO in XEngine produces a 0..1 depth range.
                // To make it reverse-Z, swap near/far at the call site OR post-multiply
                // by a flip matrix. Easiest: pass near/far in reversed order:
                tmpProj = Math::OrthographicLH_ZO(
                    -radius, radius, -radius, radius,
                     radius + desc.DepthBias * 4.0f,
                    -radius - desc.DepthBias * 4.0f);
            }
            else
            {
                tmpProj = Math::OrthographicLH_ZO(
                    -radius, radius, -radius, radius,
                    -radius - desc.DepthBias * 4.0f,
                     radius + desc.DepthBias * 4.0f);
            }

            const Vec3 snap = ComputeTexelSnapOffset(
                center, lightView, tmpProj, texelSize);
            lightView[3] -= Vec4(snap, 0.0f);
        }

        // Final light projection (orthographic). Re-derived here so the
        // texel-snap path applies the snap with the same projection.
        Mat4 lightProjection = Mat4(1.0f);
        if (desc.ReverseZ)
        {
            lightProjection = Math::OrthographicLH_ZO(
                -radius, radius, -radius, radius,
                 radius + desc.DepthBias * 4.0f,
                -radius - desc.DepthBias * 4.0f);
        }
        else
        {
            lightProjection = Math::OrthographicLH_ZO(
                -radius, radius, -radius, radius,
                -radius - desc.DepthBias * 4.0f,
                 radius + desc.DepthBias * 4.0f);
        }

        const Mat4 lightViewProj = lightProjection * lightView;

        // Fill the cascade slot.
        RenderShadowCascade& cascade = outData.Cascades[cascadeIndex];
        cascade.LightView           = lightView;
        cascade.LightProjection     = lightProjection;
        cascade.LightViewProjection = lightViewProj;
        cascade.SplitNear           = splitNear;
        cascade.SplitFar            = splitFar;
        cascade.LayerIndex          = cascadeIndex;
        cascade.Resolution          = desc.Resolution;
        cascade.ShadowMapSize       = Vec4(
            static_cast<float>(desc.Resolution),
            static_cast<float>(desc.Resolution),
            1.0f / static_cast<float>(desc.Resolution),
            1.0f / static_cast<float>(desc.Resolution));
        cascade.BiasParams = Vec4(
            desc.DepthBias,
            desc.NormalBias,
            0.0f,    // slope-scaled bias factor (Stage 10+)
            0.0f);

        // World bounds: the 8 cascade frustum corners.
        cascade.WorldBounds = AABB::FromPoints(cascadeCorners.data(), 8);

        // Light-space bounds: transform corners into light space and re-fit.
        Vec3 lightSpaceMin( std::numeric_limits<float>::infinity());
        Vec3 lightSpaceMax(-std::numeric_limits<float>::infinity());
        for (const Vec3& corner : cascadeCorners)
        {
            const Vec4 ls = lightView * Vec4(corner, 1.0f);
            lightSpaceMin = Math::Min(lightSpaceMin, Vec3(ls));
            lightSpaceMax = Math::Max(lightSpaceMax, Vec3(ls));
        }
        cascade.LightSpaceBounds = AABB(lightSpaceMin, lightSpaceMax);

        previousSplit = splitFar;
    }
```

**需要的新增 include**（在 `DirectionalShadowPlanner.cpp` 顶部追加）：

```cpp
#include <cmath>      // std::fabs, std::round
#include <limits>     // std::numeric_limits
```

**为什么**：

* `LightBasis` 提供 light view 矩阵的右/上/前向量
* 用 `center` 作 light look-at target，`center - lightBasis.Forward * radius` 作 light position，确保 cascade 包围球在 light 视线方向上完整落在 ortho frustum 内
* `ReverseZ` 字段决定 near/far 顺序：Vulkan（reverse）下 near=+radius, far=-radius（数学上在 light space 里远的物体在 -Z 方向）
* 第二次构造 `lightProjection` 看似重复，但保证 `cascade.LightViewProjection = lightProjection * lightView` 用的就是最终 projection（与 texel-snap 分支里用来求 snap 的 projection 一致）
* `AABB::FromPoints` 是 `XEngine/Math/AABB.h` 中已存在的 helper（Project_Cache 提到过 `TransformAABB / CombineAABB` 同模块）

**如果 `AABB::FromPoints` 不存在**——临时方案：

```cpp
// Fallback if AABB::FromPoints is not available:
Vec3 mn( std::numeric_limits<float>::infinity());
Vec3 mx(-std::numeric_limits<float>::infinity());
for (const Vec3& corner : cascadeCorners)
{
    mn = Math::Min(mn, corner);
    mx = Math::Max(mx, corner);
}
cascade.WorldBounds = AABB(mn, mx);
```

---

### 3.4 Step 3 验证清单

* [ ] 打开 `DirectionalShadowPlanner.cpp`，`ComputeCascadeSplits` 内部应有 `outSplits[i] = split;`
* [ ] 主循环中 `LightBasis` / `ComputeTexelSnapOffset` 两个 helper 已新增
* [ ] 主循环 `// TODO` 已替换为完整 light view / light projection / cascade 填充
* [ ] 临时 sandbox 测试：写一段独立测试代码（不进 repo）调用 `DirectionalShadowPlanner::BuildPlan`，检查 `outData.Cascades[0..3]` 的 `LightViewProjection` 是否非零、合法（det ≠ 0）
* [ ] `outData.Enabled == true && outData.CascadeCount == desc.CascadeCount`
* [ ] 拆分：lambda=0 时 splits 应该是等差；lambda=1 时 splits 应该是几何级数

### 3.5 Step 3 常见错误

* 误把 `lightView[3] = Vec4(lightPosition, 1.0f)` 写为列主序（GLM 是列主序，所以 `lightView[col][row]` 访问；上面代码用 `lightView[3]` 拿到的是第 4 列，没问题）
* 漏 `Math::Inverse(lightView)`——直接拿手写的 `lightView` 当 view matrix 是错的，view 应当是 world→light 的反向
* `ReverseZ` 拼反——上面的代码两次构造 projection 都显式 `desc.ReverseZ ? swapped : normal`，**不要**让 Stage 3 之后又改一次
* `texelSize` 算错——`ortho half-width = radius`，ortho 总宽 = `2*radius`，NDC 范围 [-1, 1] 共 2 单位，所以 texel size = 2.0 / resolution

---

## Step 4 — RHI 关键扩展（Texture View、Depth-only Pipeline、Per-frame Descriptor Update）

### 4.1 目标

为 Stage 9 的 shadow 框架提供 RHI 层基础设施：

1. 新增 `RHITextureView` 抽象（per-layer depth view + whole-array sampled view）
2. 让 `RHIRenderOutputDesc` 的 `DepthTarget` 升级为 `RHITextureView*`（per-layer 视图）
3. 让 `RHIGraphicsPipelineDesc` 支持 `HasColorAttachment = false`（depth-only pipeline）
4. 让 `RHIDevice` 支持 `CreateTextureView` 和 `UpdateBindGroupSampledTexture`
5. Vulkan 实现上述，并修复 `VulkanTexture::GetImageViewType` 缺失 `Texture2DArray` 分支
6. 修补 `VulkanPipeline` 支持 `colorAttachmentCount = 0`

### 4.2 涉及文件

| 文件 | 操作 |
|------|------|
| `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHITextureView.h` | **新增** |
| `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHITypes.h` | 改 `RHIRenderOutputDesc::DepthTarget` 类型 |
| `Engine/Source/Runtime/RHI/Public/XEngine/RHI/Resources/RHIPipeline.h` | 加 `HasColorAttachment` |
| `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHIDevice.h` | 加 `CreateTextureView` + `UpdateBindGroupSampledTexture` |
| `Engine/Source/Runtime/RHI/Public/XEngine/RHI/RHICommandList.h` | 无 API 变化（已在 RHICommandList 抽象层兼容） |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.h` | **新增** |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTextureView.cpp` | **新增** |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanTexture.cpp` | 补 `GetImageViewType` 分支 |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.h` | 补方法声明 |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDevice.cpp` | 实现 `CreateTextureView` / `UpdateBindGroupSampledTexture`，扩 descriptor pool |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanPipeline.cpp` | 支持 `colorAttachmentCount = 0` |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.h` | 加 `VulkanBindGroup::UpdateSampledTexture(...)` 声明 |
| `Engine/Source/Runtime/RHI/Private/Vulkan/VulkanDescriptor.cpp` | 实现 `UpdateSampledTexture` |

### 4.3 详细修改

#### 4.3.1 新增 `RHITextureView.h`

**完整文件**：

```cpp
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHITexture;

    enum class RHITextureViewDimension : u8
    {
        Texture2D,
        Texture2DArray,
        TextureCube,
    };

    enum class RHITextureAspectFlags : u8
    {
        None     = 0,
        Color    = 1 << 0,
        Depth    = 1 << 1,
        Stencil  = 1 << 2,
        DepthStencil = Depth | Stencil,
    };

    inline RHITextureAspectFlags operator|(
        RHITextureAspectFlags lhs, RHITextureAspectFlags rhs)
    {
        return static_cast<RHITextureAspectFlags>(
            static_cast<u8>(lhs) | static_cast<u8>(rhs));
    }

    inline bool HasFlag(RHITextureAspectFlags value, RHITextureAspectFlags flag)
    {
        return (static_cast<u8>(value) & static_cast<u8>(flag)) != 0;
    }

    struct RHITextureViewDesc
    {
        RHITexture*             Texture = nullptr;
        RHITextureViewDimension ViewDimension = RHITextureViewDimension::Texture2D;
        RHIFormat               Format = RHIFormat::Undefined;
        u32                     BaseMipLevel = 0;
        u32                     MipCount = 1;
        u32                     BaseArrayLayer = 0;
        u32                     ArrayLayerCount = 1;
        RHITextureAspectFlags   Aspect = RHITextureAspectFlags::Color;
        const char*             DebugName = nullptr;
    };

    class RHITextureView
    {
    public:
        virtual ~RHITextureView() = default;

        virtual const RHITextureViewDesc& GetDesc() const = 0;

        // Backend-specific native image view handle. Returns nullptr for
        // backends whose handles are owned by the backend object itself.
        virtual void* GetNativeImageView(RHIBackend backend) const
        {
            (void)backend;
            return nullptr;
        }
    };
}
```

**为什么**：

* 不让 RHI 公共头知道"cascades"——只暴露通用 image view 抽象
* `AspectFlags` 用位运算，便于 depth/stencil 单独或合并
* `BaseArrayLayer + ArrayLayerCount`：per-cascade view 只需 `ArrayLayerCount = 1`，sampled view 用 `ArrayLayerCount = CascadeCount`

---

#### 4.3.2 修改 `RHITypes.h` — `RHIRenderOutputDesc::DepthTarget` 改类型

**当前代码**（约第 143–151 行）：

```cpp
struct RHIRenderOutputDesc
{
    RHITexture* ColorTarget = nullptr;
    RHITexture* DepthTarget = nullptr;
    RHIRect2D Viewport {};
    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;
    bool RenderToSwapchain = true;
};
```

**改为**：

```cpp
class RHITextureView;

struct RHIRenderOutputDesc
{
    RHITexture*     ColorTarget   = nullptr;   // null for depth-only rendering
    RHITextureView* DepthTarget   = nullptr;   // may be a per-layer view
    RHIRect2D       Viewport      {};
    RHIFormat       ColorFormat    = RHIFormat::BGRA8Unorm;
    RHIFormat       DepthFormat    = RHIFormat::D32Float;
    bool            RenderToSwapchain = true;
};
```

**注意**：

* `RHITextureView` 只需前置声明（因为是指针），不需要把 `#include "Resources/RHITextureView.h"` 加进来——`RHITypes.h` 是被广泛 include 的低层头，不应再拉入更多依赖
* `ColorTarget == nullptr + ColorFormat == Undefined` 作为"depth-only rendering"的合法输入（ShadowDepthPass 会用到）

**前向声明位置**：放在 `RHITypes.h` 顶部 `class RHITexture;` 之后。

---

#### 4.3.3 修改 `RHIPipeline.h` — 加 `HasColorAttachment`

**当前代码**（约第 26–45 行）：

```cpp
struct RHIGraphicsPipelineDesc
{
    RHIShader* VertexShader = nullptr;
    RHIShader* FragmentShader = nullptr;

    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;
    RHIFormat DepthFormat = RHIFormat::D32Float;

    bool EnableDepthTest = true;
    bool EnableDepthWrite = true;

    RHIVertexBufferLayoutDesc VertexLayout;
    std::vector<RHIBindGroupLayout*> BindGroupLayouts;

    u32 PushConstantSize = 0;
    RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;

    const char* DebugName = nullptr;
};
```

**改为**：

```cpp
struct RHIGraphicsPipelineDesc
{
    RHIShader* VertexShader = nullptr;
    RHIShader* FragmentShader = nullptr;

    RHIFormat ColorFormat = RHIFormat::BGRA8Unorm;   // Ignored when HasColorAttachment == false.
    RHIFormat DepthFormat = RHIFormat::D32Float;

    // True for color-writing pipelines. False for depth-only pipelines
    // (e.g. ShadowDepth). When false, ColorFormat is ignored and
    // the backend will skip color attachment setup.
    bool HasColorAttachment = true;

    bool EnableDepthTest = true;
    bool EnableDepthWrite = true;

    RHIVertexBufferLayoutDesc VertexLayout;
    std::vector<RHIBindGroupLayout*> BindGroupLayouts;

    u32 PushConstantSize = 0;
    RHIShaderStageFlags PushConstantStages = RHIShaderStageFlags::Vertex;

    const char* DebugName = nullptr;
};
```

**为什么**：

* 默认 `true` 保持现有 forward pipeline 行为不变
* Stage 9 的 `ShadowDepth` pipeline 设 `HasColorAttachment = false`
* `ColorFormat` 字段语义不破坏：保留字段，只是当 `HasColorAttachment == false` 时忽略

---

#### 4.3.4 修改 `RHIDevice.h` — 加两个工厂方法

**当前代码**（约第 47–62 行）：

```cpp
virtual std::shared_ptr<RHITexture> CreateTexture(
    const RHITextureDesc& desc,
    const void* initialData,
    std::size_t initialDataSize) = 0;

virtual std::shared_ptr<RHISampler> CreateSampler(
    const RHISamplerDesc& desc) = 0;
```

**改为**（在 `CreateSampler` 之后追加两个方法）：

```cpp
virtual std::shared_ptr<RHITexture> CreateTexture(
    const RHITextureDesc& desc,
    const void* initialData,
    std::size_t initialDataSize) = 0;

virtual std::shared_ptr<RHITextureView> CreateTextureView(
    const RHITextureViewDesc& desc) = 0;

virtual std::shared_ptr<RHISampler> CreateSampler(
    const RHISamplerDesc& desc) = 0;

// Update an existing bind group's sampled-texture binding in-place.
// Used by RenderFrameResources to swap shadow texture / sampler when
// ShadowResourceCache rebuilds the shadow array.
virtual void UpdateBindGroupSampledTexture(
    RHIBindGroup* bindGroup,
    u32 binding,
    RHITextureView* view,
    RHISampler* sampler) = 0;
```

**需要的前向声明**：在 `RHIDevice.h` 顶部 `class RHICommandList;` 后追加：

```cpp
class RHITextureView;
```

---

#### 4.3.5 修改 `RHICommandList.h` — 无 API 变化

无需修改。`SetRenderOutput(const RHIRenderOutputDesc&)` 已经按 desc 接受任意 `DepthTarget`（`RHITextureView*` 也行），抽象层兼容。

**但是**：Vulkan 后端 `VulkanCommandList::SetRenderOutput` 的实现需要从 `m_RenderOutput.DepthTarget` 拿出 view 然后用 view 的 native handle 创建 `VkRenderingAttachmentInfo`。这是 §4.3.11 的工作。

---

#### 4.3.6 新增 `VulkanTextureView.h`

```cpp
#pragma once

#include <XEngine/RHI/Resources/RHITextureView.h>

#include <volk.h>

namespace XEngine
{
    class VulkanTextureView final : public RHITextureView
    {
    public:
        VulkanTextureView() = default;
        ~VulkanTextureView() override;

        VulkanTextureView(const VulkanTextureView&) = delete;
        VulkanTextureView& operator=(const VulkanTextureView&) = delete;

        bool Create(
            VkDevice device,
            RHITexture* sourceTexture,
            const RHITextureViewDesc& desc);

        void Destroy();

        VkImageView GetHandle() const;
        const RHITextureViewDesc& GetDesc() const override;
        void* GetNativeImageView(RHIBackend backend) const override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        RHITextureViewDesc m_Desc {};
    };
}
```

---

#### 4.3.7 新增 `VulkanTextureView.cpp`

```cpp
#include "VulkanTextureView.h"

#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <string>

namespace XEngine
{
    namespace
    {
        VkImageViewType ToVulkanImageViewType(RHITextureViewDimension dim)
        {
            switch (dim)
            {
            case RHITextureViewDimension::Texture2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case RHITextureViewDimension::Texture2DArray:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case RHITextureViewDimension::TextureCube:
                return VK_IMAGE_VIEW_TYPE_CUBE;
            }
            return VK_IMAGE_VIEW_TYPE_2D;
        }

        VkImageAspectFlags ToVulkanAspectFlags(RHITextureAspectFlags flags)
        {
            VkImageAspectFlags result = 0;
            if (HasFlag(flags, RHITextureAspectFlags::Depth))
            {
                result |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (HasFlag(flags, RHITextureAspectFlags::Stencil))
            {
                result |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            if (result == 0)
            {
                result = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            return result;
        }
    }

    VulkanTextureView::~VulkanTextureView()
    {
        Destroy();
    }

    bool VulkanTextureView::Create(
        VkDevice device,
        RHITexture* sourceTexture,
        const RHITextureViewDesc& desc)
    {
        Destroy();

        if (device == VK_NULL_HANDLE || sourceTexture == nullptr)
        {
            XENGINE_LOG_ERROR("VulkanTextureView requires a valid device and source texture");
            return false;
        }

        auto* vkTexture = dynamic_cast<VulkanTexture*>(sourceTexture);
        if (vkTexture == nullptr || vkTexture->GetImage() == VK_NULL_HANDLE)
        {
            XENGINE_LOG_ERROR("VulkanTextureView requires a VulkanTexture source");
            return false;
        }

        m_Device = device;
        m_Desc = desc;
        m_Desc.Texture = sourceTexture;

        const VkFormat format = (desc.Format == RHIFormat::Undefined)
            ? RHIFormatToVulkanFormat(vkTexture->GetDesc().Format)
            : RHIFormatToVulkanFormat(desc.Format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            XENGINE_LOG_ERROR("VulkanTextureView received an unsupported format");
            m_Desc = {};
            m_Device = VK_NULL_HANDLE;
            return false;
        }

        VkImageViewCreateInfo viewInfo {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vkTexture->GetImage();
        viewInfo.viewType = ToVulkanImageViewType(desc.ViewDimension);
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = ToVulkanAspectFlags(desc.Aspect);
        viewInfo.subresourceRange.baseMipLevel = desc.BaseMipLevel;
        viewInfo.subresourceRange.levelCount = desc.MipCount;
        viewInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
        viewInfo.subresourceRange.layerCount = desc.ArrayLayerCount;

        const VkResult result = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView);
        if (result != VK_SUCCESS)
        {
            std::string message = "Failed to create Vulkan image view: ";
            message += VulkanResultToString(result);
            XENGINE_LOG_ERROR(message);
            m_Device = VK_NULL_HANDLE;
            m_Desc = {};
            return false;
        }

        return true;
    }

    void VulkanTextureView::Destroy()
    {
        if (m_Device != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
        m_Desc = {};
    }

    VkImageView VulkanTextureView::GetHandle() const
    {
        return m_ImageView;
    }

    const RHITextureViewDesc& VulkanTextureView::GetDesc() const
    {
        return m_Desc;
    }

    void* VulkanTextureView::GetNativeImageView(RHIBackend backend) const
    {
        return backend == RHIBackend::Vulkan
            ? const_cast<void*>(static_cast<const void*>(&m_ImageView))
            : nullptr;
    }
}
```

**注意**：`GetNativeImageView` 返回的是 `VkImageView*` 指针的地址。其它后端用 `void*` 装 native handle 没问题，但 Vulkan 通常直接用 `VkImageView` 值。Stage 9 内部用 `dynamic_cast<VulkanTextureView*>` 拿 handle 更稳。这个 helper 主要给 Editor 跨后端调试用，Stage 9 不依赖它的正确性。

---

#### 4.3.8 修改 `VulkanTexture.cpp` — 修补 `GetImageViewType`

**当前代码**（约第 17–28 行）：

```cpp
VkImageViewType GetImageViewType(RHITextureDimension dimension)
{
    switch (dimension)
    {
    case RHITextureDimension::TextureCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case RHITextureDimension::Texture2D:
    default:
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}
```

**改为**：

```cpp
VkImageViewType GetImageViewType(RHITextureDimension dimension)
{
    switch (dimension)
    {
    case RHITextureDimension::TextureCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case RHITextureDimension::Texture2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case RHITextureDimension::Texture2D:
    default:
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}
```

**为什么**：Stage 9 之前 `Texture2DArray` 还没被实际用过，所以这个 bug 没暴露；现在 `VulkanTexture` 在 `VulkanDevice::CreateTexture` 中直接拿这个函数算 default view 的 view type，必须补上。

---

#### 4.3.9 修改 `VulkanDevice.h` — 加方法声明 + 头依赖

**当前代码**（约第 60–64 行）：

```cpp
std::shared_ptr<RHITexture> CreateTexture(
    const RHITextureDesc& desc,
    const void* initialData,
    std::size_t initialDataSize) override;

std::shared_ptr<RHISampler> CreateSampler(
    const RHISamplerDesc& desc) override;
```

**改为**（中间插入 `CreateTextureView`）：

```cpp
std::shared_ptr<RHITexture> CreateTexture(
    const RHITextureDesc& desc,
    const void* initialData,
    std::size_t initialDataSize) override;

std::shared_ptr<RHITextureView> CreateTextureView(
    const RHITextureViewDesc& desc) override;

std::shared_ptr<RHISampler> CreateSampler(
    const RHISamplerDesc& desc) override;

void UpdateBindGroupSampledTexture(
    RHIBindGroup* bindGroup,
    u32 binding,
    RHITextureView* view,
    RHISampler* sampler) override;
```

**顶部 include**（在现有 `VulkanTexture.h` 之后追加）：

```cpp
#include "VulkanTextureView.h"
```

---

#### 4.3.10 修改 `VulkanDevice.cpp` — 实现 + descriptor pool 扩容

**3 处修改：**

**(a) 扩 descriptor pool**（在 `CreateDescriptorPool` 中，修改 `poolSizes` 数组）：

**当前**（约第 817–821 行）：

```cpp
VkDescriptorPoolSize poolSizes[] = {
    VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
    VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128 },
    VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 }
};
```

**改为**：

```cpp
VkDescriptorPoolSize poolSizes[] = {
    VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 },
    VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128 },
    VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 }
};
```

**为什么**：shadow array 占用 3 个 frame-in-flight × 1 个 `CombinedImageSampler` 描述符 + 其它材质采样器——1024 够用但偏紧，扩到 4096 留余量。

**(b) 实现 `CreateTextureView`**（紧接 `CreateTexture` 之后）：

```cpp
std::shared_ptr<RHITextureView> VulkanDevice::CreateTextureView(
    const RHITextureViewDesc& desc)
{
    auto view = std::make_shared<VulkanTextureView>();
    if (!view->Create(m_Device, desc.Texture, desc))
    {
        return nullptr;
    }
    return view;
}
```

**(c) 实现 `UpdateBindGroupSampledTexture`**（紧接 `CreateBindGroup` 之后）：

```cpp
void VulkanDevice::UpdateBindGroupSampledTexture(
    RHIBindGroup* bindGroup,
    u32 binding,
    RHITextureView* view,
    RHISampler* sampler)
{
    auto* vkBindGroup = dynamic_cast<VulkanBindGroup*>(bindGroup);
    if (vkBindGroup == nullptr)
    {
        XENGINE_LOG_ERROR("UpdateBindGroupSampledTexture requires a Vulkan bind group");
        return;
    }

    vkBindGroup->UpdateSampledTexture(binding, view, sampler);
}
```

---

#### 4.3.11 修改 `VulkanDescriptor.h` — 加方法声明

**在 `VulkanBindGroup` 类的 public 区追加**：

```cpp
    // Stage 9: live-update one combined-image-sampler binding without
    // reallocating the descriptor set. Used for shadow texture swaps.
    void UpdateSampledTexture(
        u32 binding,
        RHITextureView* view,
        RHISampler* sampler);
```

---

#### 4.3.12 修改 `VulkanDescriptor.cpp` — 实现 `UpdateSampledTexture`

**在 `VulkanBindGroup::GetDesc` 之后追加**：

```cpp
void VulkanBindGroup::UpdateSampledTexture(
    u32 binding,
    RHITextureView* view,
    RHISampler* sampler)
{
    if (m_Device == VK_NULL_HANDLE || m_Set == VK_NULL_HANDLE)
    {
        XENGINE_LOG_ERROR("Cannot update unbound Vulkan bind group");
        return;
    }

    if (view == nullptr || sampler == nullptr)
    {
        // Stage 9 V0: when shadow is disabled, callers may pass null.
        // We still update with a placeholder view+ sampler if possible.
        XENGINE_LOG_WARN("UpdateSampledTexture called with null view or sampler; "
                         "frame bind group should be configured to handle this case");
        return;
    }

    auto* vkView = dynamic_cast<VulkanTextureView*>(view);
    auto* vkSampler = dynamic_cast<VulkanSampler*>(sampler);
    if (vkView == nullptr || vkSampler == nullptr)
    {
        XENGINE_LOG_ERROR("UpdateSampledTexture requires VulkanTextureView and VulkanSampler");
        return;
    }

    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView   = vkView->GetHandle();
    imageInfo.sampler    = vkSampler->GetHandle();

    VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_Set;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
}
```

**需要的 include**（在 `VulkanDescriptor.cpp` 顶部追加）：

```cpp
#include "VulkanTextureView.h"
```

---

#### 4.3.13 修改 `VulkanPipeline.cpp` — 支持 `colorAttachmentCount = 0`

**当前代码**（约第 174–186 行）：

```cpp
const VkFormat colorFormat = RHIFormatToVulkanFormat(desc.ColorFormat);
const VkFormat depthFormat = RHIFormatToVulkanFormat(desc.DepthFormat);
if (colorFormat == VK_FORMAT_UNDEFINED)
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported color format");
    return;
}

VkPipelineRenderingCreateInfo renderingCreateInfo {};
renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingCreateInfo.colorAttachmentCount = 1;
renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
renderingCreateInfo.depthAttachmentFormat = depthFormat;
```

**改为**：

```cpp
const VkFormat depthFormat = RHIFormatToVulkanFormat(desc.DepthFormat);
if (depthFormat == VK_FORMAT_UNDEFINED)
{
    XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported depth format");
    return;
}

VkPipelineRenderingCreateInfo renderingCreateInfo {};
renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingCreateInfo.depthAttachmentFormat = depthFormat;

VkFormat colorFormat = VK_FORMAT_UNDEFINED;
if (desc.HasColorAttachment)
{
    colorFormat = RHIFormatToVulkanFormat(desc.ColorFormat);
    if (colorFormat == VK_FORMAT_UNDEFINED)
    {
        XENGINE_LOG_ERROR("Vulkan graphics pipeline received an unsupported color format");
        return;
    }
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
}
else
{
    renderingCreateInfo.colorAttachmentCount = 0;
    renderingCreateInfo.pColorAttachmentFormats = nullptr;
}
```

**为什么**：

* 当 `HasColorAttachment == false` 时（shadow depth pipeline），不要校验 `ColorFormat`，也不要 attach 任何 color
* `depthAttachmentFormat` 仍然需要——shadow pass 写深度

**配套改动 — color blend state**（约第 152–159 行）：

**当前**：

```cpp
VkPipelineColorBlendAttachmentState colorBlendAttachment {};
colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;

VkPipelineColorBlendStateCreateInfo colorBlend {};
colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlend.attachmentCount = 1;
colorBlend.pAttachments = &colorBlendAttachment;
```

**改为**：

```cpp
VkPipelineColorBlendStateCreateInfo colorBlend {};
colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

if (desc.HasColorAttachment)
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachment;
}
else
{
    colorBlend.attachmentCount = 0;
    colorBlend.pAttachments = nullptr;
}
```

**为什么**：Vulkan spec 要求 `pAttachments` 字段在 `attachmentCount == 0` 时为 nullptr（或有效指针，但 `attachmentCount` 必须为 0）。

---

#### 4.3.14 修改 `VulkanCommandList`（若需要）— 适配 `RHITextureView* DepthTarget`

**当前代码**（约第 60 行）：

```cpp
RHIRenderOutputDesc m_RenderOutput {};
```

这是成员，类型已变成 `DepthTarget: RHITextureView*`，无成员级修改。**但是** `SetRenderOutput` 实际实现可能用到了 `DepthTarget` 的 native handle。需要把现有从 `RHITexture*` 抽 `VkImage` / `VkImageView` 的代码切到走 view。

**审计当前 `VulkanCommandList::SetRenderOutput`**（需要读 `VulkanCommandList.cpp` 来确认，但本 plan 假设它当前类似）：

```cpp
// 伪代码示意：
auto* depthTex = dynamic_cast<VulkanTexture*>(m_RenderOutput.DepthTarget);
VkImageView depthView = depthTex->GetImageView();   // <-- 不对，per-layer 时是错的
```

**改为**：

```cpp
auto* depthView = dynamic_cast<VulkanTextureView*>(m_RenderOutput.DepthTarget);
VkImageView vkDepthView = depthView ? depthView->GetHandle() : VK_NULL_HANDLE;
```

**建议**：在 `VulkanCommandList::SetRenderOutput` 内部把 `m_RenderOutput.DepthTarget` 视作 `RHITextureView*`：

```cpp
// 取出 depth view
RHITextureView* depthView = m_RenderOutput.DepthTarget;
VkImageView vkDepthView = VK_NULL_HANDLE;
if (depthView != nullptr)
{
    auto* vkView = dynamic_cast<VulkanTextureView*>(depthView);
    if (vkView != nullptr)
    {
        vkDepthView = vkView->GetHandle();
    }
}
```

并把所有 `m_DepthTexture`（当前是 `VulkanTexture*`，只是 swapchain 的）字段处理逻辑保持原状——swapchain depth 仍可直接用 `RHITexture*` 因为它总是一个 2D texture 不需要 view。

**具体怎么改要等读到 `VulkanCommandList.cpp` 全文才能确定。** 本 plan 只给方向，不给逐行 diff。

---

#### 4.3.15 修改 `RHITexture.h` — 保留不变

Stage 4 不需要修改 `RHITexture` 抽象本身。`RHITexture::GetNativeImageView` 已有默认实现，足够 Stage 9 调用。

---

### 4.4 Step 4 验证清单

* [ ] `RHITextureView.h` 存在并能被 include
* [ ] `RHITypes.h::RHIRenderOutputDesc::DepthTarget` 类型是 `RHITextureView*`
* [ ] `RHIPipeline.h::RHIGraphicsPipelineDesc` 含 `HasColorAttachment` 字段（默认 `true`）
* [ ] `RHIDevice::CreateTextureView` / `UpdateBindGroupSampledTexture` 是纯虚（或有默认实现）
* [ ] `VulkanTextureView.h/.cpp` 创建且 `Create` / `Destroy` / `GetHandle` 可用
* [ ] `VulkanTexture.cpp::GetImageViewType` 新增 `Texture2DArray → VK_IMAGE_VIEW_TYPE_2D_ARRAY` 分支
* [ ] `VulkanPipeline.cpp` 在 `HasColorAttachment == false` 时设 `colorAttachmentCount = 0; pColorAttachmentFormats = nullptr;`，color blend state 同样归零
* [ ] `VulkanDescriptor.cpp` 增 `VulkanBindGroup::UpdateSampledTexture`，且使用 `dynamic_cast<VulkanTextureView*>` 拿 handle
* [ ] `VulkanDevice::CreateTextureView` 调用 `VulkanTextureView::Create`；`UpdateBindGroupSampledTexture` 转发到 `VulkanBindGroup::UpdateSampledTexture`
* [ ] `VulkanCommandList` 把 `RHIRenderOutputDesc::DepthTarget` 视作 `RHITextureView*` 并取 native handle
* [ ] descriptor pool 中 `COMBINED_IMAGE_SAMPLER` 升到 4096

### 4.5 Step 4 常见错误

* `RHITextureView::GetNativeImageView` 返回 `void*` 但实际是 `VkImageView*`——Vulkan 是值类型不是指针类型，要么返回 `VkImageView` 的地址（已用 `const_cast`），要么直接返回 `nullptr` 然后靠 `dynamic_cast` 拿
* `VkImageViewCreateInfo::subresourceRange.layerCount = 0` 是非法（VUID 报错）——要保证至少 1
* `VkImageViewCreateInfo::subresourceRange.aspectMask` 漏 `VK_IMAGE_ASPECT_DEPTH_BIT`，shadow texture 采样会得到 undefined 内容
* `pColorAttachmentFormats` 在 0 attachments 时仍传非空指针——Vulkan validation layer 会报 `VUID-VkPipelineRenderingCreateInfo-colorAttachmentCount-06060`
* `VulkanBindGroup::UpdateSampledTexture` 没校验 binding 在 layout 范围内——若 caller 传错 binding 不会报错但 descriptor 不会更新
* `RHITextureViewDesc::Format == RHIFormat::Undefined` 时回退到 `sourceTexture.Format`——`VulkanTextureView::Create` 已实现，**不要**漏掉
* `descriptor pool` 容量忘了扩——Stage 9 的 `CreateTextureView` 会让 pool 中实际占用数 +3（每 frame 1 个 sampled + 4 个 per-cascade depth view），原 1024 够但是偏紧

---

## Step 3 + Step 4 全部改完后整体验证

* [ ] `XEngineRenderer` 与 `XEngineRHI` 都能编译
* [ ] 现有 Sandbox / Editor 仍能启动（无 shadow 视觉变化，因为 ShadowResourceCache 还没实现）
* [ ] 临时可在 `ShadowResourceCache::GetOrCreateDirectionalShadowResources` 中加一行 `XENGINE_LOG_INFO("Shadow texture created")`，并加一个临时 sandbox 测试代码创建 `Texture2DArray(D32, 4 layers)` + 1 个 sampled view + 4 个 per-layer view，确认 Vulkan validation layer 不报 `VUID-VkImageViewCreateInfo-*`
* [ ] 临时可在 `VulkanCommandList::SetRenderOutput` 中加一个 `XENGINE_LOG_INFO` 打印 `DepthTarget != nullptr ? "view" : "null"`，确认 shadow pass 调用时打印 "view"

---

## 附：本次改动未触及的文件

```
Engine/Source/Runtime/Scene/**
Engine/Source/Runtime/Asset/**
Engine/Source/Runtime/Serialization/**
Engine/Source/Runtime/Shader/**
Engine/Source/Runtime/Renderer/Public/XEngine/Renderer/RenderSystem.h
Engine/Source/Runtime/Renderer/Private/RenderSystem.cpp
Engine/Source/Runtime/Renderer/Private/Pipeline/**
Engine/Source/Runtime/Renderer/Private/Passes/**
Engine/Source/Runtime/Renderer/Private/Resources/**
Engine/Source/Runtime/Renderer/Private/ShaderInterop/**
Engine/Source/Runtime/Renderer/Private/Scene/RenderExtraction.cpp
Engine/Source/Runtime/Renderer/Private/Shadows/DirectionalShadowPlanner.h   (Step 1 已改)
Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowType.h          (Step 1 已改)
Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.h
Engine/Source/Runtime/Renderer/Private/Shadows/RenderShadowManager.cpp
Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.h
Engine/Source/Runtime/Renderer/Private/Shadows/ShadowResourceCache.cpp
Engine/Source/Editor/**
Engine/Shaders/**
```

---

## 总结

| Step | 文件数 | 新增代码行 | 删除代码行 | 关键风险 |
|------|--------|------------|------------|----------|
| Step 3 | 1 | ~100 | ~3 | `Math::Inverse / Cross / AABB::FromPoints` 是否存在；`ReverseZ` 方向 |
| Step 4 | 13 (3 新 + 10 改) | ~250 | ~10 | `RHITextureView` 抽象正确性；`colorAttachmentCount = 0` 在 `VulkanPipeline` 中的特殊路径；`VulkanCommandList` 切到 view-based depth target |
| **合计** | **14** | **~350** | **~13** | 中等 |

完成这两步后，可以进入 Step 5（`ShadowResourceCache` 实现）。Step 5-6-7 是把 RHI 扩展真正串起来；Step 8 开始跑通 CascadeCount=1 路径。
