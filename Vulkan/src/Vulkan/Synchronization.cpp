#include "Synchronization.h"

namespace VK
{
	result_t Fence::Wait() const 
	{
		VkResult result = vkWaitForFences(VkBase::Base().Device(), 1, &handle, false, UINT64_MAX);
		if (result)
		{
			outStream << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Fence::Reset() const
	{
		VkResult result = vkResetFences(VkBase::Base().Device(), 1, &handle);
		if (result)
		{
			outStream << std::format("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Fence::WaitAndReset() const
	{
		VkResult result = Wait();
		result || (result = Reset());
		return result;
	}

	result_t Fence::Status() const
	{
		VkResult result = vkGetFenceStatus(VkBase::Base().Device(), handle);
		if (result < 0) //vkGetFenceStatus(...)成功时有两种结果，所以不能仅仅判断result是否非0
		{
			outStream << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Fence::Create(VkFenceCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkResult result = vkCreateFence(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Fence::Create(VkFenceCreateFlags flags)
	{
		VkFenceCreateInfo createInfo = {
			.flags = flags
		};
		return Create(createInfo);
	}

	result_t Semaphore::Create(VkSemaphoreCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkResult result = vkCreateSemaphore(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
		{
			outStream << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
		}
		return result;
	}

	result_t Semaphore::Create(/*VkSemaphoreCreateFlags flags*/)
	{
		VkSemaphoreCreateInfo createInfo = {};
		return Create(createInfo);
	}
}