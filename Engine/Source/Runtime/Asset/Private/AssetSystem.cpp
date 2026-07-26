#include <XEngine/Asset/AssetSystem.h>

#include "AssetRegistry.h"
#include "Importers/GltfImporter.h"
#include "Importers/ImageImporter.h"
#include "Importers/ImporterRegistry.h"

#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Core/ProjectPaths.h>
#include <XEngine/Logging/Log.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>

namespace XEngine
{
    struct AssetSystem::Impl
    {
        std::unique_ptr<AssetRegistry> Registry;
        std::unique_ptr<ImporterRegistry> ImporterRegistry;

        std::unordered_map<u32, MaterialAsset> MaterialAssets;
        std::unordered_map<u32, MeshAsset> MeshAssets;
        std::unordered_map<u32, TextureAsset> TextureAssets;
        
        bool Initialized = false;
    };

    namespace
    {
        std::string ToLower(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(std::tolower(c));
                });
            return value;
        }
    }

    AssetSystem::AssetSystem()
        : m_Impl(std::make_unique<Impl>())
    {
    }

    AssetSystem::~AssetSystem()
    {
        OnDestroy();
    }

    void AssetSystem::OnCreate(const SubsystemContext&)
    {
        if (m_Impl->Initialized)
        {
            return;
        }

        m_Impl->Registry = std::make_unique<AssetRegistry>();
        m_Impl->ImporterRegistry = std::make_unique<ImporterRegistry>();
        m_Impl->ImporterRegistry->RegisterImporter(std::make_unique<ImageImporter>(*this));
        m_Impl->ImporterRegistry->RegisterImporter(std::make_unique<GltfImporter>(*this));

        RegisterValidationAssetMetadata();

        m_Impl->Initialized = true;

        std::string message = "AssetSystem initialized with ";
        message += std::to_string(GetAssetCount());
        message += " metadata entries";
        XENGINE_LOG_INFO(message);
    }

    void AssetSystem::OnDestroy()
    {
        if (!m_Impl ||
            (!m_Impl->Initialized && !m_Impl->Registry && !m_Impl->ImporterRegistry &&
             m_Impl->MaterialAssets.empty() && m_Impl->MeshAssets.empty() && m_Impl->TextureAssets.empty()))
        {
            return;
        }

        XENGINE_LOG_INFO("AssetSystem shutdown");
        m_Impl->MaterialAssets.clear();
        m_Impl->MeshAssets.clear();
        m_Impl->TextureAssets.clear();
        m_Impl->ImporterRegistry.reset();
        m_Impl->Registry.reset();
        m_Impl->Initialized = false;
    }

    AssetHandle AssetSystem::RegisterSourceAsset(
        const std::filesystem::path& sourcePath,
        AssetType type)
    {
        if (!m_Impl->Registry)
        {
            return {};
        }

        const AssetHandle existing = m_Impl->Registry->FindHandleByPath(sourcePath);
        if (existing.IsValid())
        {
            return existing;
        }

        AssetMetadata metadata;
        metadata.SourcePath = sourcePath.lexically_normal();
        metadata.Type = type;
        metadata.Name = metadata.SourcePath.stem().string();
        metadata.LoadState = AssetLoadState::Unloaded;

        return m_Impl->Registry->RegisterAsset(metadata);
    }

    const AssetMetadata* AssetSystem::GetMetadata(AssetHandle handle) const
    {
        return m_Impl->Registry ? m_Impl->Registry->GetMetadata(handle) : nullptr;
    }

    AssetMetadata* AssetSystem::GetMetadata(AssetHandle handle)
    {
        return m_Impl->Registry ? m_Impl->Registry->GetMetadata(handle) : nullptr;
    }

    AssetHandle AssetSystem::FindAssetByPath(const std::filesystem::path& sourcePath) const
    {
        return m_Impl->Registry ? m_Impl->Registry->FindHandleByPath(sourcePath) : AssetHandle {};
    }

    const AssetMetadata* AssetSystem::FindMetadataByPath(const std::filesystem::path& sourcePath) const
    {
        return m_Impl->Registry ? m_Impl->Registry->FindByPath(sourcePath) : nullptr;
    }

    AssetImportResult AssetSystem::ImportAsset(const std::filesystem::path& sourcePath)
    {
        AssetImportResult result;
        const std::filesystem::path resolvedSourcePath = ProjectPaths::Resolve(sourcePath.generic_string());
        if (!std::filesystem::exists(resolvedSourcePath))
        {
            result.Code = AssetImportResultCode::FileNotFound;
            result.Diagnostics = "Asset source file was not found.";
            return result;
        }

        AssetImportContext context;
        context.SourcePath = resolvedSourcePath.lexically_normal();
        context.RequestedType = GuessAssetTypeFromPath(context.SourcePath);

        if (!m_Impl->ImporterRegistry)
        {
            result.Code = AssetImportResultCode::ImporterUnavailable;
            result.Diagnostics = "Asset importer registry is not initialized.";
            return result;
        }

        // Stage 7B dispatches importers by extension. CanImport() remains as a
        // final validation hook rather than the primary lookup mechanism.
        IAssetImporter* importer = m_Impl->ImporterRegistry->FindImporterForExtension(context.SourcePath.extension().string());
        if (importer == nullptr)
        {
            result.Code = AssetImportResultCode::ImporterUnavailable;
            result.Diagnostics = "No registered importer handles this asset extension.";
            return result;
        }

        if (!importer->CanImport(context))
        {
            result.Code = AssetImportResultCode::UnsupportedFormat;
            result.Diagnostics = "Registered importer rejected this asset source.";
            return result;
        }

        return importer->Import(context);
    }

    const MeshAsset* AssetSystem::GetMeshAsset(AssetHandle handle) const
    {
        const AssetMetadata* metadata = GetMetadata(handle);
        if (metadata == nullptr || metadata->Type != AssetType::Mesh)
        {
            return nullptr;
        }

        const auto it = m_Impl->MeshAssets.find(handle.Index);
        if (it == m_Impl->MeshAssets.end())
        {
            return nullptr;
        }

        return it->second.IsValid() ? &it->second : nullptr;
    }

    const MaterialAsset* AssetSystem::GetMaterialAsset(AssetHandle handle) const
    {
        const AssetMetadata* metadata = GetMetadata(handle);
        if (metadata == nullptr || metadata->Type != AssetType::Material)
        {
            return nullptr;
        }

        const auto it = m_Impl->MaterialAssets.find(handle.Index);
        if (it == m_Impl->MaterialAssets.end())
        {
            return nullptr;
        }

        return it->second.IsValid() ? &it->second : nullptr;
    }

    const TextureAsset* AssetSystem::GetTextureAsset(AssetHandle handle) const
    {
        const AssetMetadata* metadata = GetMetadata(handle);
        if (metadata == nullptr || metadata->Type != AssetType::Texture)
        {
            return nullptr;
        }

        const auto it = m_Impl->TextureAssets.find(handle.Index);
        if (it == m_Impl->TextureAssets.end())
        {
            return nullptr;
        }

        return it->second.IsValid() ? &it->second : nullptr;
    }

    AssetHandle AssetSystem::RegisterTextureAsset(
        const std::filesystem::path& sourcePath,
        TextureAsset textureAsset)
    {
        if (!textureAsset.IsValid())
        {
            return {};
        }

        AssetHandle handle = RegisterSourceAsset(sourcePath, AssetType::Texture);
        if (!handle.IsValid())
        {
            return {};
        }

        if (AssetMetadata* metadata = GetMetadata(handle))
        {
            metadata->LoadState = AssetLoadState::Loaded;
            metadata->Type = AssetType::Texture;
        }

        m_Impl->TextureAssets[handle.Index] = std::move(textureAsset);
        return handle;
    }

    AssetHandle AssetSystem::CreateProceduralCubeMeshAsset(const std::string& name)
    {
        // Stage 7C uses a procedural cube to validate the MeshAsset -> RenderMesh
        // bridge before glTF import is introduced in Stage 7E.
        MeshAsset mesh;
        mesh.Name = name.empty() ? "ProceduralCube" : name;
        mesh.SourcePath = "procedural/" + mesh.Name + ".mesh";
        mesh.Bounds.Min = Vec3 { -0.5f, -0.5f, -0.5f };
        mesh.Bounds.Max = Vec3 { 0.5f, 0.5f, 0.5f };

        auto appendFace = [&mesh](
            const Vec3& normal,
            const Vec4& tangent,
            const Vec3& v0,
            const Vec3& v1,
            const Vec3& v2,
            const Vec3& v3)
        {
            const u32 baseVertex = static_cast<u32>(mesh.Vertices.size());
            mesh.Vertices.push_back(MeshVertex { v0, normal, tangent, Vec2 { 0.0f, 0.0f } });
            mesh.Vertices.push_back(MeshVertex { v1, normal, tangent, Vec2 { 1.0f, 0.0f } });
            mesh.Vertices.push_back(MeshVertex { v2, normal, tangent, Vec2 { 1.0f, 1.0f } });
            mesh.Vertices.push_back(MeshVertex { v3, normal, tangent, Vec2 { 0.0f, 1.0f } });

            mesh.Indices.push_back(baseVertex + 0);
            mesh.Indices.push_back(baseVertex + 1);
            mesh.Indices.push_back(baseVertex + 2);
            mesh.Indices.push_back(baseVertex + 0);
            mesh.Indices.push_back(baseVertex + 2);
            mesh.Indices.push_back(baseVertex + 3);
        };

        appendFace(
            Vec3 { 0.0f, 0.0f, 1.0f },
            Vec4 { 1.0f, 0.0f, 0.0f, 1.0f },
            Vec3 { -0.5f, -0.5f, 0.5f },
            Vec3 { 0.5f, -0.5f, 0.5f },
            Vec3 { 0.5f, 0.5f, 0.5f },
            Vec3 { -0.5f, 0.5f, 0.5f });
        appendFace(
            Vec3 { 0.0f, 0.0f, -1.0f },
            Vec4 { -1.0f, 0.0f, 0.0f, 1.0f },
            Vec3 { 0.5f, -0.5f, -0.5f },
            Vec3 { -0.5f, -0.5f, -0.5f },
            Vec3 { -0.5f, 0.5f, -0.5f },
            Vec3 { 0.5f, 0.5f, -0.5f });
        appendFace(
            Vec3 { -1.0f, 0.0f, 0.0f },
            Vec4 { 0.0f, 0.0f, 1.0f, 1.0f },
            Vec3 { -0.5f, -0.5f, -0.5f },
            Vec3 { -0.5f, -0.5f, 0.5f },
            Vec3 { -0.5f, 0.5f, 0.5f },
            Vec3 { -0.5f, 0.5f, -0.5f });
        appendFace(
            Vec3 { 1.0f, 0.0f, 0.0f },
            Vec4 { 0.0f, 0.0f, -1.0f, 1.0f },
            Vec3 { 0.5f, -0.5f, 0.5f },
            Vec3 { 0.5f, -0.5f, -0.5f },
            Vec3 { 0.5f, 0.5f, -0.5f },
            Vec3 { 0.5f, 0.5f, 0.5f });
        appendFace(
            Vec3 { 0.0f, 1.0f, 0.0f },
            Vec4 { 1.0f, 0.0f, 0.0f, 1.0f },
            Vec3 { -0.5f, 0.5f, 0.5f },
            Vec3 { 0.5f, 0.5f, 0.5f },
            Vec3 { 0.5f, 0.5f, -0.5f },
            Vec3 { -0.5f, 0.5f, -0.5f });
        appendFace(
            Vec3 { 0.0f, -1.0f, 0.0f },
            Vec4 { 1.0f, 0.0f, 0.0f, 1.0f },
            Vec3 { -0.5f, -0.5f, -0.5f },
            Vec3 { 0.5f, -0.5f, -0.5f },
            Vec3 { 0.5f, -0.5f, 0.5f },
            Vec3 { -0.5f, -0.5f, 0.5f });

        MeshSubmesh submesh;
        submesh.FirstIndex = 0;
        submesh.IndexCount = static_cast<u32>(mesh.Indices.size());
        submesh.VertexOffset = 0;
        submesh.MaterialSlot = 0;
        submesh.Bounds = mesh.Bounds;
        mesh.Submeshes.push_back(submesh);

        return RegisterMeshAsset(mesh.SourcePath, std::move(mesh));
    }

    AssetHandle AssetSystem::CreateTestMaterialAsset(
        const std::string& name,
        AssetHandle baseColorTexture)
    {
        // Stage 7D validates the Asset -> Renderer material bridge before glTF
        // material import is introduced in Stage 7E.
        MaterialAsset material;
        material.Name = name.empty() ? "Stage7D_TestMaterial" : name;
        material.SourcePath = "procedural/" + material.Name + ".material";
        material.ShadingModel = MaterialAssetShadingModel::Lit;
        material.AlphaMode = MaterialAssetAlphaMode::Opaque;
        material.BaseColorFactor = Vec4 { 1.0f, 0.85f, 0.65f, 1.0f };
        material.MetallicFactor = 0.0f;
        material.RoughnessFactor = 0.5f;
        material.AlphaCutoff = 0.5f;
        material.BaseColorTexture = baseColorTexture;
        material.DoubleSided = false;

        return RegisterMaterialAsset(material.SourcePath, std::move(material));
    }

    AssetHandle AssetSystem::RegisterMeshAsset(
        const std::filesystem::path& sourcePath,
        MeshAsset meshAsset)
    {
        if (!meshAsset.IsValid())
        {
            return {};
        }

        AssetHandle handle = RegisterSourceAsset(sourcePath, AssetType::Mesh);
        if (!handle.IsValid())
        {
            return {};
        }

        if (AssetMetadata* metadata = GetMetadata(handle))
        {
            metadata->LoadState = AssetLoadState::Loaded;
            metadata->Type = AssetType::Mesh;
        }

        m_Impl->MeshAssets[handle.Index] = std::move(meshAsset);
        return handle;
    }

    AssetHandle AssetSystem::RegisterMaterialAsset(
        const std::filesystem::path& sourcePath,
        MaterialAsset materialAsset)
    {
        if (!materialAsset.IsValid())
        {
            return {};
        }

        AssetHandle handle = RegisterSourceAsset(sourcePath, AssetType::Material);
        if (!handle.IsValid())
        {
            return {};
        }

        if (AssetMetadata* metadata = GetMetadata(handle))
        {
            metadata->LoadState = AssetLoadState::Loaded;
            metadata->Type = AssetType::Material;
        }

        m_Impl->MaterialAssets[handle.Index] = std::move(materialAsset);
        return handle;
    }

    AssetType AssetSystem::GuessAssetTypeFromPath(const std::filesystem::path& sourcePath) const
    {
        if (const AssetMetadata* metadata = FindMetadataByPath(sourcePath))
        {
            return metadata->Type;
        }

        const std::string extension = ToLower(sourcePath.extension().string());

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
            extension == ".tga" || extension == ".bmp")
        {
            return AssetType::Texture;
        }

        if (extension == ".gltf" || extension == ".glb")
        {
            return AssetType::Gltf;
        }

        if (extension == ".slang")
        {
            return AssetType::Shader;
        }

        return AssetType::Unknown;
    }

    std::size_t AssetSystem::GetAssetCount() const
    {
        return m_Impl->Registry ? m_Impl->Registry->GetAssetCount() : 0;
    }

    void AssetSystem::RegisterValidationAssetMetadata()
    {
        const std::filesystem::path validationRoot = ProjectPaths::GetAssetRoot() / "Models/gltf";
        if (!std::filesystem::exists(validationRoot))
        {
            return;
        }

        // Stage 7A records source metadata only. Parsing these glTF files starts in Stage 7E.
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::recursive_directory_iterator(validationRoot))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            const AssetType type = GuessAssetTypeFromPath(entry.path());
            if (type == AssetType::Unknown)
            {
                continue;
            }

            RegisterSourceAsset(entry.path(), type);
        }
    }
}
