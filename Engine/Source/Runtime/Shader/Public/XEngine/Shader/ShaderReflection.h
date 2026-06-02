#pragma once

#include <XEngine/Shader/ShaderTypes.h>

#include <vector>

namespace XEngine
{
    struct ShaderReflection
    {
        std::vector<ShaderResourceBinding> Resources;
    };
}

