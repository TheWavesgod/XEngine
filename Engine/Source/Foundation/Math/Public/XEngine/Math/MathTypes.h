#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace XEngine
{
    // Math V0 uses glm aliases for speed of development.
    // Future stages may replace aliases with XEngine-owned math structs if needed.

    // XEngine math is column-major (GLM default).
    // - Mat4::operator[] indexes columns; m[0] is the first column, m[3] is the translation column.
    // - Vector-as-column transform reads as `M * v`. TransformPoint builds `M * Vec4(p, 1)`.
    // - View/projection construction in CameraMatrices writes columns directly (e.g. `view[0] = Vec4(right, 0)`).
    // DO NOT write row-major code; if a function reads `m[i][j]` as row-then-column it will silently produce transposed results.

    // XEngine math namespace policy:
    // - All GLM math calls in Runtime/Scene/Asset/Editor must go through `XEngine::Math::*` or `XEngine::*` (see MathFunctions.h / CameraMatrices.h).
    // - Direct `glm::lookAt / glm::perspective / glm::ortho` is forbidden in Runtime code; this is enforced by a grep rule (see Docs/audit/CHECKLIST_math.md).
    // - The internal vector/matrix types themselves remain GLM-backed; only call sites are funneled through the Math namespace.

    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;

    using IVec2 = glm::ivec2;
    using IVec3 = glm::ivec3;
    using IVec4 = glm::ivec4;

    using UVec2 = glm::uvec2;
    using UVec3 = glm::uvec3;
    using UVec4 = glm::uvec4;

    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;

    using Quat = glm::quat;
}
