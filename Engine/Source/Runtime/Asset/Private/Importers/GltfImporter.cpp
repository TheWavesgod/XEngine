#include "GltfImporter.h"

#include "ImageImporter.h"

#include <XEngine/Asset/AssetSystem.h>
#include <XEngine/Asset/Assets/MaterialAsset.h>
#include <XEngine/Asset/Assets/MeshAsset.h>
#include <XEngine/Asset/Assets/TextureAsset.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Math/CoordinateConversion.h>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace XEngine
{
    namespace
    {
        using GltfVec2 = fastgltf::math::nvec2;
        using GltfVec3 = fastgltf::math::nvec3;
        using GltfVec4 = fastgltf::math::nvec4;

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

        bool IsSupportedGltfExtension(const std::filesystem::path& path)
        {
            const std::string extension = ToLower(path.extension().string());
            return extension == ".gltf" || extension == ".glb";
        }

        std::string MakeGeneratedAssetPath(
            const std::filesystem::path& sourcePath,
            const char* category,
            std::size_t index,
            const char* extension)
        {
            std::filesystem::path path = sourcePath;
            path += ".";
            path += category;
            path += ".";
            path += std::to_string(index);
            path += extension;
            return path.lexically_normal().generic_string();
        }

        Vec2 ToVec2(const GltfVec2& value)
        {
            return Vec2 { static_cast<f32>(value[0]), static_cast<f32>(value[1]) };
        }

        Vec3 ToVec3(const GltfVec3& value)
        {
            return Vec3 { static_cast<f32>(value[0]), static_cast<f32>(value[1]), static_cast<f32>(value[2]) };
        }

        Vec4 ToVec4(const GltfVec4& value)
        {
            return Vec4 {
                static_cast<f32>(value[0]),
                static_cast<f32>(value[1]),
                static_cast<f32>(value[2]),
                static_cast<f32>(value[3])
            };
        }

        void ExpandBounds(AABB& bounds, const Vec3& position, bool& hasBounds)
        {
            if (!hasBounds)
            {
                bounds.Min = position;
                bounds.Max = position;
                hasBounds = true;
                return;
            }

            bounds.Min.x = std::min(bounds.Min.x, position.x);
            bounds.Min.y = std::min(bounds.Min.y, position.y);
            bounds.Min.z = std::min(bounds.Min.z, position.z);
            bounds.Max.x = std::max(bounds.Max.x, position.x);
            bounds.Max.y = std::max(bounds.Max.y, position.y);
            bounds.Max.z = std::max(bounds.Max.z, position.z);
        }

        const fastgltf::Accessor* FindAttributeAccessor(
            const fastgltf::Asset& asset,
            const fastgltf::Primitive& primitive,
            std::string_view name)
        {
            const auto attribute = primitive.findAttribute(name);
            if (attribute == primitive.attributes.cend() || attribute->accessorIndex >= asset.accessors.size())
            {
                return nullptr;
            }

            return &asset.accessors[attribute->accessorIndex];
        }

        std::vector<Vec3> ReadVec3Attribute(
            const fastgltf::Asset& asset,
            const fastgltf::Accessor* accessor)
        {
            std::vector<Vec3> values;
            if (accessor == nullptr || accessor->type != fastgltf::AccessorType::Vec3)
            {
                return values;
            }

            values.reserve(accessor->count);
            fastgltf::iterateAccessor<GltfVec3>(asset, *accessor, [&](const GltfVec3& value)
            {
                values.push_back(ToVec3(value));
            });
            return values;
        }

        std::vector<Vec4> ReadVec4Attribute(
            const fastgltf::Asset& asset,
            const fastgltf::Accessor* accessor)
        {
            std::vector<Vec4> values;
            if (accessor == nullptr || accessor->type != fastgltf::AccessorType::Vec4)
            {
                return values;
            }

            values.reserve(accessor->count);
            fastgltf::iterateAccessor<GltfVec4>(asset, *accessor, [&](const GltfVec4& value)
            {
                values.push_back(ToVec4(value));
            });
            return values;
        }

        std::vector<Vec2> ReadVec2Attribute(
            const fastgltf::Asset& asset,
            const fastgltf::Accessor* accessor)
        {
            std::vector<Vec2> values;
            if (accessor == nullptr || accessor->type != fastgltf::AccessorType::Vec2)
            {
                return values;
            }

            values.reserve(accessor->count);
            fastgltf::iterateAccessor<GltfVec2>(asset, *accessor, [&](const GltfVec2& value)
            {
                values.push_back(ToVec2(value));
            });
            return values;
        }

        std::vector<u32> ReadIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, u32 vertexCount)
        {
            std::vector<u32> indices;
            if (primitive.indicesAccessor && *primitive.indicesAccessor < asset.accessors.size())
            {
                const fastgltf::Accessor& accessor = asset.accessors[*primitive.indicesAccessor];
                indices.reserve(accessor.count);
                fastgltf::iterateAccessor<u32>(asset, accessor, [&](u32 index)
                {
                    indices.push_back(index);
                });
                return indices;
            }

            indices.reserve(vertexCount);
            for (u32 index = 0; index < vertexCount; ++index)
            {
                indices.push_back(index);
            }
            return indices;
        }

        MaterialAssetAlphaMode ToMaterialAssetAlphaMode(fastgltf::AlphaMode mode)
        {
            switch (mode)
            {
            case fastgltf::AlphaMode::Mask:
                return MaterialAssetAlphaMode::Masked;
            case fastgltf::AlphaMode::Blend:
                return MaterialAssetAlphaMode::Blend;
            case fastgltf::AlphaMode::Opaque:
            default:
                return MaterialAssetAlphaMode::Opaque;
            }
        }

        bool TextureInfoHasImage(const fastgltf::Asset& asset, std::size_t textureIndex)
        {
            if (textureIndex >= asset.textures.size())
            {
                return false;
            }

            const fastgltf::Texture& texture = asset.textures[textureIndex];
            return texture.imageIndex && *texture.imageIndex < asset.images.size();
        }

        std::string ImageGeneratedPath(
            const std::filesystem::path& sourcePath,
            std::size_t imageIndex)
        {
            return MakeGeneratedAssetPath(sourcePath, "image", imageIndex, ".texture");
        }
    }

    GltfImporter::GltfImporter(AssetSystem& assetSystem)
        : m_AssetSystem(assetSystem)
    {
    }

    std::string_view GltfImporter::GetName() const
    {
        return "GltfImporter";
    }

    std::vector<std::string_view> GltfImporter::GetSupportedExtensions() const
    {
        return { ".gltf", ".glb" };
    }

    bool GltfImporter::CanImport(const AssetImportContext& context) const
    {
        return context.RequestedType == AssetType::Gltf &&
               IsSupportedGltfExtension(context.SourcePath) &&
               std::filesystem::exists(context.SourcePath);
    }

    AssetImportResult GltfImporter::Import(const AssetImportContext& context)
    {
        AssetImportResult result;
        const std::filesystem::path sourcePath = context.SourcePath.lexically_normal();
        const std::filesystem::path sourceDirectory = sourcePath.parent_path();

        XENGINE_LOG_INFO(std::string("Importing glTF: ") + sourcePath.generic_string());

        auto buffer = fastgltf::GltfDataBuffer::FromPath(sourcePath);
        if (!buffer)
        {
            result.Code = AssetImportResultCode::InvalidData;
            result.Diagnostics = std::string("Failed to read glTF source: ") +
                std::string(fastgltf::getErrorMessage(buffer.error()));
            return result;
        }

        fastgltf::Parser parser;
        const fastgltf::Options options =
            fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::LoadExternalImages |
            fastgltf::Options::GenerateMeshIndices;
        auto gltf = parser.loadGltf(buffer.get(), sourceDirectory, options);
        if (!gltf)
        {
            result.Code = AssetImportResultCode::InvalidData;
            result.Diagnostics = std::string("fastgltf parse failed: ") +
                std::string(fastgltf::getErrorMessage(gltf.error()));
            return result;
        }

        fastgltf::Asset& asset = gltf.get();
        std::vector<std::string> warnings;
        std::vector<AssetHandle> materialHandles(asset.materials.size());
        std::unordered_map<std::size_t, AssetHandle> imageCache;

        auto importImage = [&](std::size_t imageIndex, bool isSRGB) -> AssetHandle
        {
            const auto cached = imageCache.find(imageIndex);
            if (cached != imageCache.end())
            {
                return cached->second;
            }

            if (imageIndex >= asset.images.size())
            {
                return {};
            }

            const fastgltf::Image& image = asset.images[imageIndex];
            TextureAsset textureAsset;
            std::filesystem::path textureSourcePath = ImageGeneratedPath(sourcePath, imageIndex);
            std::string diagnostics;

            std::visit(fastgltf::visitor {
                [&](const fastgltf::sources::URI& uri)
                {
                    if (!uri.uri.isLocalPath())
                    {
                        warnings.push_back("Skipping non-local glTF image URI.");
                        return;
                    }

                    // glTF external URIs are resolved relative to the source .gltf file.
                    textureSourcePath = (sourceDirectory / uri.uri.path()).lexically_normal();
                    textureAsset = LoadTextureAssetRGBA8(textureSourcePath, &diagnostics);
                    textureAsset.IsSRGB = isSRGB;
                },
                [&](const fastgltf::sources::Array& array)
                {
                    std::span<const std::byte> bytes(array.bytes.data(), array.bytes.size_bytes());
                    textureAsset = LoadTextureAssetRGBA8FromMemory(
                        bytes,
                        ImageGeneratedPath(sourcePath, imageIndex),
                        isSRGB,
                        &diagnostics);
                },
                [&](const fastgltf::sources::Vector& vector)
                {
                    std::span<const std::byte> bytes(vector.bytes.data(), vector.bytes.size());
                    textureAsset = LoadTextureAssetRGBA8FromMemory(
                        bytes,
                        ImageGeneratedPath(sourcePath, imageIndex),
                        isSRGB,
                        &diagnostics);
                },
                [&](const fastgltf::sources::BufferView& bufferView)
                {
                    if (bufferView.bufferViewIndex >= asset.bufferViews.size())
                    {
                        warnings.push_back("Skipping glTF image with invalid bufferView.");
                        return;
                    }

                    const auto bytes = fastgltf::DefaultBufferDataAdapter {}(asset, bufferView.bufferViewIndex);
                    textureAsset = LoadTextureAssetRGBA8FromMemory(
                        std::span<const std::byte>(bytes.data(), bytes.size()),
                        ImageGeneratedPath(sourcePath, imageIndex),
                        isSRGB,
                        &diagnostics);
                },
                [&](const auto&)
                {
                    warnings.push_back("Skipping unsupported glTF image source.");
                }
            }, image.data);

            if (!textureAsset.IsValid())
            {
                if (!diagnostics.empty())
                {
                    warnings.push_back(diagnostics);
                }
                return {};
            }

            textureAsset.IsSRGB = isSRGB;
            if (textureAsset.SourcePath.empty())
            {
                textureAsset.SourcePath = textureSourcePath.lexically_normal().generic_string();
            }

            AssetHandle handle = m_AssetSystem.RegisterTextureAsset(textureSourcePath, std::move(textureAsset));
            if (handle.IsValid())
            {
                imageCache.emplace(imageIndex, handle);
                result.ImportedAssets.push_back(handle);
            }
            return handle;
        };

        auto resolveTexture = [&](const auto& info, bool isSRGB) -> AssetHandle
        {
            if (!info || !TextureInfoHasImage(asset, info->textureIndex))
            {
                return {};
            }

            const fastgltf::Texture& texture = asset.textures[info->textureIndex];
            return importImage(*texture.imageIndex, isSRGB);
        };

        for (std::size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex)
        {
            const fastgltf::Material& gltfMaterial = asset.materials[materialIndex];

            MaterialAsset material;
            material.Name = gltfMaterial.name.empty() ?
                ("Material_" + std::to_string(materialIndex)) :
                std::string(gltfMaterial.name);
            material.SourcePath = MakeGeneratedAssetPath(sourcePath, "material", materialIndex, ".material");
            material.ShadingModel = gltfMaterial.unlit ? MaterialAssetShadingModel::Unlit : MaterialAssetShadingModel::Lit;
            material.AlphaMode = ToMaterialAssetAlphaMode(gltfMaterial.alphaMode);
            material.BaseColorFactor = ToVec4(gltfMaterial.pbrData.baseColorFactor);
            material.MetallicFactor = static_cast<f32>(gltfMaterial.pbrData.metallicFactor);
            material.RoughnessFactor = static_cast<f32>(gltfMaterial.pbrData.roughnessFactor);
            material.AlphaCutoff = static_cast<f32>(gltfMaterial.alphaCutoff);
            material.DoubleSided = gltfMaterial.doubleSided;

            material.BaseColorTexture = resolveTexture(gltfMaterial.pbrData.baseColorTexture, true);
            material.NormalTexture = resolveTexture(gltfMaterial.normalTexture, false);
            material.MetallicRoughnessTexture = resolveTexture(gltfMaterial.pbrData.metallicRoughnessTexture, false);
            material.AOTexture = resolveTexture(gltfMaterial.occlusionTexture, false);

            AssetHandle handle = m_AssetSystem.RegisterMaterialAsset(material.SourcePath, std::move(material));
            if (handle.IsValid())
            {
                materialHandles[materialIndex] = handle;
                result.ImportedAssets.push_back(handle);

                XENGINE_LOG_INFO(
                    "Imported glTF material: " +
                    std::string(gltfMaterial.name.empty() ? "<unnamed>" : gltfMaterial.name.c_str()) +
                    " baseColor=" + (asset.materials[materialIndex].pbrData.baseColorTexture ? "yes" : "no"));
            }
        }

        std::size_t importedMeshCount = 0;
        for (std::size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex)
        {
            const fastgltf::Mesh& gltfMesh = asset.meshes[meshIndex];

            MeshAsset mesh;
            mesh.Name = gltfMesh.name.empty() ? ("Mesh_" + std::to_string(meshIndex)) : std::string(gltfMesh.name);
            mesh.SourcePath = MakeGeneratedAssetPath(sourcePath, "mesh", meshIndex, ".mesh");

            bool meshHasBounds = false;
            for (const fastgltf::Primitive& primitive : gltfMesh.primitives)
            {
                if (primitive.type != fastgltf::PrimitiveType::Triangles)
                {
                    warnings.push_back("Skipping non-triangle glTF primitive.");
                    continue;
                }

                const fastgltf::Accessor* positionsAccessor = FindAttributeAccessor(asset, primitive, "POSITION");
                std::vector<Vec3> positions = ReadVec3Attribute(asset, positionsAccessor);
                if (positions.empty())
                {
                    warnings.push_back("Skipping glTF primitive without POSITION data.");
                    continue;
                }

                std::vector<Vec3> normals = ReadVec3Attribute(asset, FindAttributeAccessor(asset, primitive, "NORMAL"));
                std::vector<Vec4> tangents = ReadVec4Attribute(asset, FindAttributeAccessor(asset, primitive, "TANGENT"));
                std::vector<Vec2> uvs = ReadVec2Attribute(asset, FindAttributeAccessor(asset, primitive, "TEXCOORD_0"));
                std::vector<u32> localIndices = ReadIndices(asset, primitive, static_cast<u32>(positions.size()));

                const u32 firstIndex = static_cast<u32>(mesh.Indices.size());
                const u32 vertexOffset = static_cast<u32>(mesh.Vertices.size());
                AABB submeshBounds {};
                bool submeshHasBounds = false;

                mesh.Vertices.reserve(mesh.Vertices.size() + positions.size());
                for (std::size_t vertexIndex = 0; vertexIndex < positions.size(); ++vertexIndex)
                {
                    MeshVertex vertex;
                    vertex.Position = CoordinateConversion::GltfPositionToXEngine(positions[vertexIndex]);
                    if (vertexIndex < normals.size())
                    {
                        vertex.Normal = CoordinateConversion::GltfDirectionToXEngine(normals[vertexIndex]);
                    }
                    if (vertexIndex < tangents.size())
                    {
                        vertex.Tangent = CoordinateConversion::GltfTangentToXEngine(tangents[vertexIndex]);
                    }
                    if (vertexIndex < uvs.size())
                    {
                        vertex.TexCoord0 = uvs[vertexIndex];
                    }

                    ExpandBounds(submeshBounds, vertex.Position, submeshHasBounds);
                    ExpandBounds(mesh.Bounds, vertex.Position, meshHasBounds);
                    mesh.Vertices.push_back(vertex);
                }

                mesh.Indices.reserve(mesh.Indices.size() + localIndices.size());
                if (CoordinateConversion::GltfToXEngineFlipsHandedness())
                {
                    for (std::size_t index = 0; index + 2 < localIndices.size(); index += 3)
                    {
                        mesh.Indices.push_back(localIndices[index]);
                        mesh.Indices.push_back(localIndices[index + 2]);
                        mesh.Indices.push_back(localIndices[index + 1]);
                    }
                }
                else
                {
                    mesh.Indices.insert(mesh.Indices.end(), localIndices.begin(), localIndices.end());
                }

                MeshSubmesh submesh;
                submesh.FirstIndex = firstIndex;
                submesh.IndexCount = static_cast<u32>(localIndices.size());
                submesh.VertexOffset = vertexOffset;
                submesh.MaterialSlot = primitive.materialIndex ?
                    static_cast<u32>(*primitive.materialIndex) :
                    0u;
                submesh.Bounds = submeshBounds;
                mesh.Submeshes.push_back(submesh);
            }

            if (!mesh.IsValid())
            {
                continue;
            }

            // TODO Stage 10/15: mesh optimization, meshlets, and GPU-driven data preparation.
            AssetHandle handle = m_AssetSystem.RegisterMeshAsset(mesh.SourcePath, std::move(mesh));
            if (handle.IsValid())
            {
                if (!result.MainAsset.IsValid())
                {
                    result.MainAsset = handle;
                }
                result.ImportedAssets.push_back(handle);
                ++importedMeshCount;
            }
        }

        if (!result.MainAsset.IsValid() && !result.ImportedAssets.empty())
        {
            result.MainAsset = result.ImportedAssets.front();
        }

        std::ostringstream diagnostics;
        diagnostics << "Meshes imported: " << importedMeshCount
                    << "\nMaterials imported: " << asset.materials.size()
                    << "\nTextures imported: " << imageCache.size()
                    << "\nImported asset handles: " << result.ImportedAssets.size();
        if (!warnings.empty())
        {
            diagnostics << "\nWarnings:";
            for (const std::string& warning : warnings)
            {
                diagnostics << "\n- " << warning;
            }
        }

        XENGINE_LOG_INFO(diagnostics.str());

        result.Diagnostics = diagnostics.str();
        result.Code = result.MainAsset.IsValid() ? AssetImportResultCode::Success : AssetImportResultCode::Failed;
        if (!result.Succeeded() && warnings.empty())
        {
            result.Diagnostics = "glTF import completed but produced no supported assets.";
        }
        return result;
    }
}
