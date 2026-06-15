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

    struct RenderResourceContext
    {
        RenderTextureManager* Textures = nullptr;
        RenderMeshManager* Meshes = nullptr;
        RenderMaterialSystem* Materials = nullptr;
        RenderShaderLibrary* Shaders = nullptr;
        RenderPipelineStateCache* PipelineStates = nullptr;
        RenderFrameResources* FrameResources = nullptr;

        bool IsValid() const
        {
            return Textures != nullptr
                && Meshes != nullptr
                && Materials != nullptr
                && Shaders != nullptr
                && PipelineStates != nullptr
                && FrameResources != nullptr;
        }
    };
}
