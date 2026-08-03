#pragma once

namespace XEngine
{
    enum class RHIFrontFace
    {
        CounterClockwise,
        Clockwise
    };

    struct RHIClipSpaceConvention
    {
        bool DepthZeroToOne = true;
        bool FlipProjectionY = false;
        bool UseInvertedViewportY = false;
        RHIFrontFace DefaultFrontFace = RHIFrontFace::CounterClockwise;
    };
}
