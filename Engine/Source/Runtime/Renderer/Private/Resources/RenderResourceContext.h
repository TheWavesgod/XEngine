/*
 * Purpose:
 * Groups renderer-side resource managers so pipelines and passes do not need long parameter lists.
 */

#pragma once

namespace XEngine
{
    class TextureManager;
    class RenderMeshManager;
    class MaterialSystem;

    struct RenderResourceContext
    {
        TextureManager* Textures = nullptr;
        RenderMeshManager* Meshes = nullptr;
        MaterialSystem* Materials = nullptr;

        bool IsValid() const 
        {
            return Textures != nullptr
                && Meshes != nullptr
                && Materials != nullptr;
        }
    };
}