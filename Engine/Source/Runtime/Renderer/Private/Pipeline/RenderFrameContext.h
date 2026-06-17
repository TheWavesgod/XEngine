/*
 * Purpose:
 * Carries per-frame data needed by pipelines and passes
 */
#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/Math/MathTypes.h>
#include <XEngine/RHI/RHITypes.h>

namespace XEngine
{
    class RHIDevice;
    class RHICommandList;

    struct RenderFrameContext
    {
        RHIDevice* Device = nullptr;
        RHICommandList* CommandList = nullptr;
        RHIRenderOutputDesc Output {};

        u32 FrameIndex = 0;

        u32 SwapchainWidth = 0; 
        u32 SwapchainHeight = 0; 
        
        float DeltaTime = 0.0f; 
        float TimeSeconds = 0.0f; 
        
        Mat4 ViewMatrix { 1.0f }; 

        // ProjectionMatrix should already include RHI clip-space adaptation.
        Mat4 ProjectionMatrix { 1.0f }; 
        Mat4 ViewProjectionMatrix { 1.0f };

        // CPU-side frame context. CameraWorldPosition is used for shader frame data
        // and should already be in XEngine world coordinates.
        Vec3 CameraWorldPosition { 0.0f };
    };

} // namespace XEngine

