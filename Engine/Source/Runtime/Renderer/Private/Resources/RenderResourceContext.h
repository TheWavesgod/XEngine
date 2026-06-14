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

    struct RenderResourceContext
    {
        RenderTextureManager* Textures = nullptr;
        RenderMeshManager* Meshes = nullptr;
        RenderMaterialSystem* Materials = nullptr;
        RenderShaderLibrary* Shaders = nullptr;
        RenderPipelineStateCache* PipelineStates = nullptr;

        bool IsValid() const
        {
            return Textures != nullptr
                && Meshes != nullptr
                && Materials != nullptr
                && Shaders != nullptr
                && PipelineStates != nullptr;
        }
    };
}
