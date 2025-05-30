#include "Synchronization.h"

#include "VkBase.h"

namespace VK
{
	Fence::~Fence()
	{
		DestroyHandleBy(vkDestroyFence);
	}

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
		if (result < 0) 
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

	Semaphore::~Semaphore()
	{
		DestroyHandleBy(vkDestroySemaphore);
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

	Event::~Event()
	{
		DestroyHandleBy(vkDestroyEvent);
	}

	void Event::CmdWait(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from, 
		VkPipelineStageFlags stage_to, 
		arrayRef<VkMemoryBarrier> memoryBarriers, 
		arrayRef<VkBufferMemoryBarrier> bufferMemoryBarriers,
		arrayRef<VkImageMemoryBarrier> imageMemoryBarriers) const
	{
		for (auto& i : memoryBarriers)
			i.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		for (auto& i : bufferMemoryBarriers)
			i.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		for (auto& i : imageMemoryBarriers)
			i.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		vkCmdWaitEvents(commandBuffer, 1, &handle, stage_from, stage_to,
			memoryBarriers.Count(), memoryBarriers.Pointer(),
			bufferMemoryBarriers.Count(), bufferMemoryBarriers.Pointer(),
			imageMemoryBarriers.Count(), imageMemoryBarriers.Pointer());
	}

	result_t Event::Set() const
	{
		VkResult result = vkSetEvent(VkBase::Base().Device(), handle);
		if (result)
			outStream << std::format("[ Event ] ERROR\nFailed to singal the Event!\nError code: {}\n", int32_t(result));
		return result;
	}

	result_t Event::Reset() const
	{
		VkResult result = vkResetEvent(VkBase::Base().Device(), handle);
		if (result)
			outStream << std::format("[ Event ] ERROR\nFailed to unsingal the Event!\nError code: {}\n", int32_t(result));
		return result;
	}

	result_t Event::Status() const
	{
		VkResult result = vkGetEventStatus(VkBase::Base().Device(), handle);
		if (result < 0) //vkGetEventStatus(...) success will return two results
			outStream << std::format("[ Event ] ERROR\nFailed to get the status of the Event!\nError code: {}\n", int32_t(result));
		return result;
	}

	result_t Event::Create(VkEventCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		VkResult result = vkCreateEvent(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			outStream << std::format("[ Event ] ERROR\nFailed to create a Event!\nError code: {}\n", int32_t(result));
		return result;
	}

	result_t Event::Create(VkEventCreateFlags flags)
	{
		VkEventCreateInfo createInfo = {
			.flags = flags
		};
		return Create(createInfo);
	}
}