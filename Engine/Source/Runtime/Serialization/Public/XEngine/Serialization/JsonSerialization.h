#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>

namespace XEngine::JsonSerialization
{
    // Runtime/Serialization wraps nlohmann/json so engine modules use one
    // narrow file IO path instead of depending on upstream repository layout.
    using Json = nlohmann::json;

    bool LoadJsonFile(const std::filesystem::path& path, Json& outJson);
    bool SaveJsonFile(const std::filesystem::path& path, const Json& json);
}
