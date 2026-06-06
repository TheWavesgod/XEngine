#pragma once

#include "AssetImporter.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace XEngine
{
    class ImporterRegistry
    {
    public:
        void RegisterImporter(std::unique_ptr<IAssetImporter> importer);

        IAssetImporter* FindImporterForExtension(std::string_view extension);
        const IAssetImporter* FindImporterForExtension(std::string_view extension) const;

        std::size_t GetImporterCount() const;

    private:
        static std::string NormalizeExtension(std::string_view extension);

        std::vector<std::unique_ptr<IAssetImporter>> m_Importers;
        std::unordered_map<std::string, IAssetImporter*> m_ExtensionToImporter;
    };
}
