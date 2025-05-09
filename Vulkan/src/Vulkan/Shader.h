#pragma once

#include "VkBase.h"

namespace VK
{
	class ShaderModule
	{
		VkShaderModule handle = VK_NULL_HANDLE;

	public:
		ShaderModule() = default;
		ShaderModule(VkShaderModuleCreateInfo& createInfo) { Create(createInfo); }
		ShaderModule(const char* filepath /*VkShaderModuleCreateFlags flags*/) { Create(filepath); }
		ShaderModule(size_t codeSize, const uint32_t* pCode /*VkShaderModuleCreateFlags flags*/) { Create(codeSize, pCode); }
		ShaderModule(ShaderModule&& other) noexcept { MoveHandle; }
		~ShaderModule() { DestroyHandleBy(vkDestroyShaderModule); }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Const Function
		VkPipelineShaderStageCreateInfo StageCreateInfo(VkShaderStageFlagBits stage, const char* entry = "main") const
		{
			return {
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,//sType
				nullptr,                                            //pNext
				0,                                                  //flags
				stage,                                              //stage
				handle,                                             //module
				entry,                                              //pName
				nullptr                                             //pSpecializationInfo
			};
		}

		// Const Function
		result_t Create(VkShaderModuleCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			VkResult result = vkCreateShaderModule(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ shader ] ERROR\nFailed to create a shader module!\nError code: {}\n", int32_t(result));
			}
			return result;
		}

		result_t Create(const char* filepath /*VkShaderModuleCreateFlags flags*/)
		{
			std::ifstream file(filepath, std::ios::ate | std::ios::binary);
			if (!file)
			{
				outStream << std::format("[ shader ] ERROR\nFailed to open the file: {}\n", filepath);
				return VK_RESULT_MAX_ENUM; //没有合适的错误代码，别用VK_ERROR_UNKNOWN
			}

			size_t fileSize = size_t(file.tellg());
			std::vector<uint32_t> binaries(fileSize / 4);
			file.seekg(0);
			file.read(reinterpret_cast<char*>(binaries.data()), fileSize);
			file.close();
			return Create(fileSize, binaries.data());
		}

		result_t Create(size_t codeSize, const uint32_t* pCode)
		{
			VkShaderModuleCreateInfo createInfo = {
				.codeSize = codeSize,
				.pCode = pCode
			};
			return Create(createInfo);
		}
	};
}