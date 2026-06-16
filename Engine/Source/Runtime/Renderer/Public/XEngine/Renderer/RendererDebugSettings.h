#pragma once

namespace XEngine
{
    struct RendererDebugSettings
    {
        bool VisualizeLighting = false;
        bool VisualizeNormals = false;
        bool VisualizeBaseColor = false;

        // Reserved for Stage 9 CSM debug tools; stored now so editor UI and
        // renderer state already agree on the future controls.
        bool VisualizeCascades = false;
        bool FreezeShadowMatrices = false;
    };
}
