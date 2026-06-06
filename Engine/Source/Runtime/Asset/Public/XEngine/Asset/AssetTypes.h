#pragma once

namespace XEngine
{
    // Broad source/runtime asset categories. Stage 7A does not split glTF into sub-assets yet.
    enum class AssetType
    {
        Unknown,
        Texture,
        Mesh,
        Material,
        Scene,
        Shader,
        Gltf,
        Folder
    };

    // CPU-side asset lifecycle state tracked by AssetSystem metadata only.
    enum class AssetLoadState
    {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

    // Importer result classification. Importers do not create RHI resources.
    enum class AssetImportResultCode
    {
        Success,
        Failed,
        UnsupportedFormat,
        FileNotFound,
        InvalidData,
        ImporterUnavailable
    };
}
