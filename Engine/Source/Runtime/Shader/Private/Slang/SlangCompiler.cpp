#include "SlangCompiler.h"

#include <XEngine/Core/ProjectPaths.h>
#include <XEngine/Logging/Log.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cstdlib>

namespace XEngine
{
    namespace
    {
#if defined(XENGINE_SLANGC_EXECUTABLE)
        constexpr const char* SlangcExecutable = XENGINE_SLANGC_EXECUTABLE;
#else
        constexpr const char* SlangcExecutable = "";
#endif

        const char* GetStageName(ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return "vertex";
            case ShaderStage::Fragment:
                return "fragment";
            case ShaderStage::Compute:
                return "compute";
            default:
                return "unknown";
            }
        }

        std::string Quote(const std::filesystem::path& path)
        {
            std::filesystem::path preferredPath = path;
            preferredPath.make_preferred();

            std::string result = "\"";
            result += preferredPath.string();
            result += "\"";
            return result;
        }

        std::vector<u8> ReadBinaryFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            return std::vector<u8>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }

        std::string ReadTextFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }
    }

    SlangCompiler::SlangCompiler()
    {
        m_Initialized = Initialize();
    }

    SlangCompiler::~SlangCompiler()
    {
        Shutdown();
    }

    bool SlangCompiler::IsAvailable() const
    {
        return m_Initialized;
    }

    CompiledShader SlangCompiler::Compile(const ShaderCompileDesc& desc)
    {
        CompiledShader shader;
        shader.Stage = desc.Stage;
        shader.Target = desc.Target;
        shader.EntryPoint = desc.EntryPoint;
        shader.SourcePath = desc.Path;

        if (!m_Initialized)
        {
            shader.Result = ShaderCompileResult::CompilerUnavailable;
            shader.Diagnostics = "Slang compiler is unavailable";
            return shader;
        }

        if (desc.Target != ShaderTarget::VulkanSPIRV)
        {
            shader.Result = ShaderCompileResult::UnsupportedTarget;
            shader.Diagnostics = "Stage 4A only supports Vulkan SPIR-V output";
            return shader;
        }

        std::filesystem::path inputPath = ProjectPaths::Resolve(desc.Path);

        if (!std::filesystem::exists(inputPath))
        {
            shader.Result = ShaderCompileResult::Failed;
            shader.Diagnostics = "Shader source does not exist: " + inputPath.string();
            return shader;
        }

        std::filesystem::path outputDirectory = ProjectPaths::Resolve("cache://Shaders/Vulkan");
        std::filesystem::create_directories(outputDirectory);

        std::filesystem::path outputPath = outputDirectory /
            (inputPath.stem().string() + "." + desc.EntryPoint + ".spv");
        std::filesystem::path diagnosticsPath = outputDirectory /
            (inputPath.stem().string() + "." + desc.EntryPoint + ".log");
        std::filesystem::path scriptPath = outputDirectory /
            (inputPath.stem().string() + "." + desc.EntryPoint + ".cmd");

        // TODO: Replace slangc fallback with Slang C++ API integration.
        std::filesystem::path slangcPath(SlangcExecutable);

        std::string command = Quote(slangcPath);
        command += " ";
        command += Quote(inputPath);
        command += " -target spirv";
        command += " -entry ";
        command += desc.EntryPoint;
        command += " -stage ";
        command += GetStageName(desc.Stage);
        command += " -o ";
        command += Quote(outputPath);

        if (desc.GenerateDebugInfo)
        {
            command += " -g";
        }

        if (!desc.EnableOptimization)
        {
            command += " -O0";
        }

        std::vector<std::filesystem::path> includeDirectories;
        const std::filesystem::path shaderRoot = ProjectPaths::GetShaderRoot();

        // Stage 8C:
        // Register shader include roots so pass shaders can share Common, Lighting,
        // BRDF, and Material helper files.
        includeDirectories.push_back(shaderRoot);
        includeDirectories.push_back(shaderRoot / "Common");
        includeDirectories.push_back(shaderRoot / "Lighting");
        includeDirectories.push_back(shaderRoot / "Materials");
        includeDirectories.push_back(shaderRoot / "Passes");
        for (const std::string& includeDirectory : desc.IncludeDirectories)
        {
            includeDirectories.push_back(ProjectPaths::Resolve(includeDirectory));
        }

        for (const std::filesystem::path& includeDirectory : includeDirectories)
        {
            command += " -I";
            command += Quote(includeDirectory);
        }

        for (const ShaderDefine& define : desc.Defines)
        {
            command += " -D";
            command += define.Name;
            if (!define.Value.empty())
            {
                command += "=";
                command += define.Value;
            }
        }

        std::string message = "Running shader compiler command: ";
        message += command;
        XENGINE_LOG_INFO(message);

        {
            std::ofstream script(scriptPath, std::ios::binary);
            script << "@echo off\r\n";
            script << command << " > " << Quote(diagnosticsPath) << " 2>&1\r\n";
        }

        const int exitCode = std::system(Quote(scriptPath).c_str());
        shader.Diagnostics = ReadTextFile(diagnosticsPath);
        if (exitCode != 0)
        {
            shader.Result = ShaderCompileResult::Failed;
            if (shader.Diagnostics.empty())
            {
                shader.Diagnostics = "slangc failed with exit code " + std::to_string(exitCode);
            }
            return shader;
        }

        shader.Bytecode = ReadBinaryFile(outputPath);
        if (shader.Bytecode.empty())
        {
            shader.Result = ShaderCompileResult::Failed;
            shader.Diagnostics = "slangc did not produce SPIR-V output: " + outputPath.string();
            return shader;
        }

        shader.Format = ShaderCodeFormat::Binary;
        shader.Result = ShaderCompileResult::Success;
        return shader;
    }

    bool SlangCompiler::Initialize()
    {
        if (std::string(SlangcExecutable).empty())
        {
            XENGINE_LOG_ERROR("slangc executable is not configured");
            return false;
        }

        XENGINE_LOG_INFO("Slang compiler initialized using slangc fallback");
        return true;
    }

    void SlangCompiler::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        XENGINE_LOG_INFO("Slang compiler shutdown");
        m_Initialized = false;
    }
}
