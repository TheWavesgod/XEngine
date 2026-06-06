#include "ImporterRegistry.h"

#include <XEngine/Logging/Log.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace XEngine
{
    void ImporterRegistry::RegisterImporter(std::unique_ptr<IAssetImporter> importer)
    {
        if (!importer)
        {
            return;
        }

        IAssetImporter* importerPtr = importer.get();
        m_Importers.push_back(std::move(importer));

        for (std::string_view extension : importerPtr->GetSupportedExtensions())
        {
            std::string key = NormalizeExtension(extension);
            if (key.empty())
            {
                continue;
            }

            const auto [it, inserted] = m_ExtensionToImporter.emplace(key, importerPtr);
            if (!inserted)
            {
                XENGINE_LOG_WARN(std::string("Replacing asset importer for extension: ") + key);
                it->second = importerPtr;
            }
        }
    }

    IAssetImporter* ImporterRegistry::FindImporterForExtension(std::string_view extension)
    {
        return const_cast<IAssetImporter*>(
            static_cast<const ImporterRegistry*>(this)->FindImporterForExtension(extension));
    }

    const IAssetImporter* ImporterRegistry::FindImporterForExtension(std::string_view extension) const
    {
        const auto it = m_ExtensionToImporter.find(NormalizeExtension(extension));
        if (it == m_ExtensionToImporter.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::size_t ImporterRegistry::GetImporterCount() const
    {
        return m_Importers.size();
    }

    std::string ImporterRegistry::NormalizeExtension(std::string_view extension)
    {
        std::string normalized(extension);
        if (normalized.empty())
        {
            return normalized;
        }

        if (normalized.front() != '.')
        {
            normalized.insert(normalized.begin(), '.');
        }

        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return normalized;
    }
}
