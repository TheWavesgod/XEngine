#pragma once

#include "VkBase.h"

namespace VK
{
	/**
	 *  A collection of a series of image attachment used for specific
	 */
	class Framebuffer
	{
		VkFramebuffer handle = VK_NULL_HANDLE;

	public:
		Framebuffer() = default;
		Framebuffer(VkFramebufferCreateInfo& createInfo) { Create(createInfo); }

		Framebuffer(Framebuffer&& other) noexcept { MoveHandle; }
		~Framebuffer() { DestroyHandleBy(vkDestroyFramebuffer); }

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		//Non-const Function
		result_t Create(VkFramebufferCreateInfo& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			VkResult result = vkCreateFramebuffer(VkBase::Base().Device(), &createInfo, nullptr, &handle);
			if (result)
			{
				outStream << std::format("[ framebuffer ] ERROR\nFailed to create a framebuffer!\nError code: {}\n", int32_t(result));
			}
			return result;
		}
	};
}

