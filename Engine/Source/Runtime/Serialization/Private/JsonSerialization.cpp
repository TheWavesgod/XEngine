#include <XEngine/Serialization/JsonSerialization.h>

#include <XEngine/Logging/Log.h>

#include <fstream>
#include <string>

namespace XEngine::JsonSerialization
{
    bool LoadJsonFile(const std::filesystem::path& path, Json& outJson)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            XENGINE_LOG_ERROR(std::string("Failed to open JSON file: ") + path.generic_string());
            return false;
        }

        try
        {
            file >> outJson;
        }
        catch (const std::exception& exception)
        {
            XENGINE_LOG_ERROR(
                std::string("Failed to parse JSON file: ") +
                path.generic_string() +
                " - " +
                exception.what());
            return false;
        }

        XENGINE_LOG_INFO(std::string("JSON file loaded: ") + path.generic_string());
        return true;
    }

    bool SaveJsonFile(const std::filesystem::path& path, const Json& json)
    {
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary);
        if (!file)
        {
            XENGINE_LOG_ERROR(std::string("Failed to open JSON file for writing: ") + path.generic_string());
            return false;
        }

        file << json.dump(4);
        file << '\n';
        XENGINE_LOG_INFO(std::string("JSON file saved: ") + path.generic_string());
        return true;
    }
}
