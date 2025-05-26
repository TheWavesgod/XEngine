#include "Shader.h"
#include "VkBase.h"

namespace VK
{
	ShaderModule::~ShaderModule()
	{
		DestroyHandleBy(vkDestroyShaderModule);
	}

	result_t ShaderModule::Create(VkShaderModuleCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		VkResult result = vkCreateShaderModule(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ shader ] ERROR\nFailed to create a shader module!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t ShaderModule::Create(const char* filename /*VkShaderModuleCreateFlags flags*/)
	{
		std::string filepath = "../shaders/spv/";
		filepath += filename;
		filepath += ".spv";

		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file)
		{
			outStream << std::format("[ shader ] ERROR\nFailed to open the file: {}\n", filepath);
			return VK_RESULT_MAX_ENUM; 
		}

		size_t fileSize = size_t(file.tellg());
		std::vector<uint32_t> binaries(fileSize / 4);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(binaries.data()), fileSize);
		file.close();
		return Create(fileSize, binaries.data());
	}

	result_t ShaderModule::Create(size_t codeSize, const uint32_t* pCode)
	{
		VkShaderModuleCreateInfo createInfo = {
			.codeSize = codeSize,
			.pCode = pCode
		};
		return Create(createInfo);
	}
}
