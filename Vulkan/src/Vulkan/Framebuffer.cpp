#include "Framebuffer.h"

namespace VK
{
	result_t Framebuffer::Create(VkFramebufferCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		VkResult result = vkCreateFramebuffer(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ framebuffer ] ERROR\nFailed to create a framebuffer!\nError code: {}\n", int32_t(result));
		}
		return result;
	}
}