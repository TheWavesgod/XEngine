#include "Descriptor.h"

#include "VkBase.h"

namespace VK
{
	DescriptorSetLayout::~DescriptorSetLayout()
	{
		DestroyHandleBy(vkDestroyDescriptorSetLayout);
	}

	result_t DescriptorSetLayout::Create(VkDescriptorSetLayoutCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		VkResult result = vkCreateDescriptorSetLayout(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			outStream << std::format("[ descriptorSetLayout ] ERROR\nFailed to create a descriptor set layout!\nError code: {}\n", int32_t(result));
		return result;
	}

	void DescriptorSet::Write(arrayRef<const VkDescriptorImageInfo> descriptorInfos, VkDescriptorType descriptorType, 
		uint32_t dstBinding, uint32_t dstArrayElement) const
	{
		VkWriteDescriptorSet writeDescriptorSet = {
			.dstSet = handle,
			.dstBinding = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = uint32_t(descriptorInfos.Count()),
			.descriptorType = descriptorType,
			.pImageInfo = descriptorInfos.Pointer()
		};
		Update(writeDescriptorSet);
	}

	void DescriptorSet::Write(arrayRef<const VkDescriptorBufferInfo> descriptorInfos, VkDescriptorType descriptorType, 
		uint32_t dstBinding, uint32_t dstArrayElement) const 
	{
		VkWriteDescriptorSet writeDescriptorSet = {
			.dstSet = handle,
			.dstBinding = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = uint32_t(descriptorInfos.Count()),
			.descriptorType = descriptorType,
			.pBufferInfo = descriptorInfos.Pointer()
		};
		Update(writeDescriptorSet);
	}

	void DescriptorSet::Write(arrayRef<const VkBufferView> descriptorInfos, VkDescriptorType descriptorType, 
		uint32_t dstBinding, uint32_t dstArrayElement) const 
	{
		VkWriteDescriptorSet writeDescriptorSet = {
			.dstSet = handle,
			.dstBinding = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = uint32_t(descriptorInfos.Count()),
			.descriptorType = descriptorType,
			.pTexelBufferView = descriptorInfos.Pointer()
		};
		Update(writeDescriptorSet);
	}

	void DescriptorSet::Write(arrayRef<const BufferView> descriptorInfos, VkDescriptorType descriptorType,
		uint32_t dstBinding, uint32_t dstArrayElement) const 
	{
		Write({ descriptorInfos[0].Address(), descriptorInfos.Count() }, descriptorType, dstBinding, dstArrayElement);
	}

	void DescriptorSet::Update(arrayRef<VkWriteDescriptorSet> writes, arrayRef<VkCopyDescriptorSet> copies)
	{
		for (auto& i : writes)
		{
			i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		}
		for (auto& i : copies)
		{
			i.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;

		}
		vkUpdateDescriptorSets(VkBase::Base().Device(), writes.Count(), writes.Pointer(), copies.Count(), copies.Pointer());
	}

	DescriptorPool::~DescriptorPool()
	{
		DestroyHandleBy(vkDestroyDescriptorPool);
	}

	result_t DescriptorPool::AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const
	{
		if (sets.Count() != setLayouts.Count())
		{
			if (sets.Count() < setLayouts.Count()) 
			{
				outStream << std::format("[ descriptorPool ] ERROR\nFor each descriptor set, must provide a corresponding layout!\n");
				return VK_RESULT_MAX_ENUM;
			}
			else
				outStream << std::format("[ descriptorPool ] WARNING\nProvided layouts are more than sets!\n");
		}

		VkDescriptorSetAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = handle,
			.descriptorSetCount = uint32_t(sets.Count()),
			.pSetLayouts = setLayouts.Pointer()
		};

		VkResult result = vkAllocateDescriptorSets(VkBase::Base().Device(), &allocateInfo, sets.Pointer());
		if (result)
			outStream << std::format("[ descriptorPool ] ERROR\nFailed to allocate descriptor sets!\nError code: {}\n", int32_t(result));
		return result;
	}

	result_t DescriptorPool::AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const DescriptorSetLayout> setLayouts) const 
	{
		return AllocateSets(
			sets,
			{ setLayouts[0].Address(), setLayouts.Count() }
		);
	}

	result_t DescriptorPool::AllocateSets(arrayRef<DescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const 
	{
		return AllocateSets(
			{ &sets[0].handle, sets.Count() },
			setLayouts);
	}

	result_t DescriptorPool::AllocateSets(arrayRef<DescriptorSet> sets, arrayRef<const DescriptorSetLayout> setLayouts) const 
	{
		return AllocateSets(
			{ &sets[0].handle, sets.Count() },
			{ setLayouts[0].Address(), setLayouts.Count() }
		);
	}

	result_t DescriptorPool::FreeSets(arrayRef<VkDescriptorSet> sets) const 
	{
		VkResult result = vkFreeDescriptorSets(VkBase::Base().Device(), handle, sets.Count(), sets.Pointer());
		memset(sets.Pointer(), 0, sets.Count() * sizeof(VkDescriptorSet));
		return result;//Though vkFreeDescriptorSets(...) can only return VK_SUCCESS
	}

	result_t DescriptorPool::FreeSets(arrayRef<DescriptorSet> sets) const 
	{
		return FreeSets({ &sets[0].handle, sets.Count() });
	}
	
	result_t DescriptorPool::Create(VkDescriptorPoolCreateInfo& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkResult result = vkCreateDescriptorPool(VkBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			outStream << std::format("[ descriptorPool ] ERROR\nFailed to create a descriptor pool!\nError code: {}\n", int32_t(result));
		return result;
	}

	result_t DescriptorPool::Create(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags)
	{
		VkDescriptorPoolCreateInfo createInfo = {
			.flags = flags,
			.maxSets = maxSetCount,
			.poolSizeCount = uint32_t(poolSizes.Count()),
			.pPoolSizes = poolSizes.Pointer()
		};
		return Create(createInfo);
	}
}
