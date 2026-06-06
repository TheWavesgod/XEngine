#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace XEngine
{
    // Math V0 uses glm aliases for speed of development.
    // Future stages may replace aliases with XEngine-owned math structs if needed.
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
