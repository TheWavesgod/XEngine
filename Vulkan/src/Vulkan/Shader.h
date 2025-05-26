#pragma once

#include "VKEasyHeader.h"

namespace VK
{
	class ShaderModule
	{
		VkShaderModule handle = VK_NULL_HANDLE;

	public:
		ShaderModule() = default;
		ShaderModule(VkShaderModuleCreateInfo& createInfo) { Create(createInfo); }
		ShaderModule(const char* filename /*VkShaderModuleCreateFlags flags*/) { Create(filename); }
		ShaderModule(size_t codeSize, const uint32_t* pCode /*VkShaderModuleCreateFlags flags*/) { Create(codeSize, pCode); }

		ShaderModule(ShaderModule&& other) noexcept { MoveHandle; }
		~ShaderModule();

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
		result_t Create(VkShaderModuleCreateInfo& createInfo);

		result_t Create(const char* filename /*VkShaderModuleCreateFlags flags*/);

		result_t Create(size_t codeSize, const uint32_t* pCode);
	};
}