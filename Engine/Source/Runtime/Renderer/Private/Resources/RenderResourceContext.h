/*
 * Purpose:
 * Groups renderer-side resource managers so pipelines and passes do not need long parameter lists.
 */

#pragma once

namespace XEngine
{
    class RenderTextureManager;
    class RenderMeshManager;
    class RenderMaterialSystem;
    class RenderShaderLibrary;
    class RenderPipelineStateCache;
    class RenderFrameResources;
    class RenderShadowManager;

    struct RenderResourceContext
    {
        RenderTextureManager* Textures = nullptr;
        RenderMeshManager* Meshes = nullptr;
        RenderMaterialSystem* Materials = nullptr;
        RenderShaderLibrary* Shaders = nullptr;
        RenderPipelineStateCache* PipelineStates = nullptr;
        RenderFrameResources* FrameResources = nullptr;
        RenderShadowManager* ShadowManager = nullptr;

        bool IsValid() const
        {
            return Textures != nullptr
                && Meshes != nullptr
                && Materials != nullptr
                && Shaders != nullptr
                && PipelineStates != nullptr
                && FrameResources != nullptr
                && ShadowManager != nullptr;
        }
    };
}
