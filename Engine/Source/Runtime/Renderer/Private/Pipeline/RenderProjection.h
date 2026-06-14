#pragma once

#include <XEngine/Math/MathTypes.h>
#include <XEngine/RHI/RHIClipSpace.h>

namespace XEngine
{
    inline Mat4 ApplyRHIClipSpaceConvention(
        const Mat4& projection,
        const RHIClipSpaceConvention& convention)
    {
        Mat4 adapted = projection;
        if (convention.FlipProjectionY)
        {
            adapted[1][1] *= -1.0f;
        }
        return adapted;
    }
}
