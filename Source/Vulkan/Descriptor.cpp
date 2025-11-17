#include "Descriptor.h"

#include "VkBase.h"

namespace VK
{
	DescriptorSetLayout::~DescriptorSetLayout()
	{
		DestroyHandleBy(vkDestroyDescriptorSetLayout);
	}

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::SetFlags(VkDescriptorSetLayoutCreateFlags flags)
	{
		flags_ = flags;
		return *this;
	}

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddBinding(
		uint32_t binding, VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags, uint32_t descriptorCount,
		const VkSampler* pImmutableSamplers)
	{
		VkDescriptorSetLayoutBinding b{};
		b.binding = binding;
		b.descriptorType = descriptorType;
		b.descriptorCount = descriptorCount;
		b.stageFlags = stageFlags;
		b.pImmutableSamplers = pImmutableSamplers;
		bindings_.push_back(b);
		return *this;
	}

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddUniformBuffer(
		uint32_t binding, VkShaderStageFlags stageFlags, uint32_t descriptorCount)
	{
		return AddBinding(binding,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			stageFlags,
			descriptorCount);
	}

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddUniformBufferDynamic(
		uint32_t binding, VkShaderStageFlags stageFlags, uint32_t descriptorCount)
	{
		return AddBinding(binding,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			stageFlags,
			descriptorCount);
	}

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddCombinedImageSampler(
		uint32_t binding, VkShaderStageFlags stageFlags, uint32_t descriptorCount, const VkSampler* pImmutableSamplers)
	{
		return AddBinding(binding,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			stageFlags,
			descriptorCount,
			pImmutableSamplers);
	}

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddStorageBuffer(
		uint32_t binding, VkShaderStageFlags stageFlags, uint32_t descriptorCount)
	{
		return AddBinding(binding,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			stageFlags,
			descriptorCount);
	}

	DescriptorSetLayout DescriptorSetLayout::Builder::Build()
	{
		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.flags = flags_;
		info.bindingCount = static_cast<uint32_t>(bindings_.size());
		info.pBindings = bindings_.data();

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		if (vkCreateDescriptorSetLayout(device_, &info, nullptr, &layout) != VK_SUCCESS)
			throw std::runtime_error("DescriptorSetLayout::Builder: failed to create VkDescriptorSetLayout.");

		return DescriptorSetLayout(layout);
	}

	DescriptorPool::~DescriptorPool()
	{
		DestroyHandleBy(vkDestroyDescriptorPool);
	}

	void DescriptorPool::Reset() const
	{
		if (handle != VK_NULL_HANDLE) 
		{
			vkResetDescriptorPool(device, handle, 0);
		}
	}

	DescriptorPool::Builder& DescriptorPool::Builder::SetFlags(VkDescriptorPoolCreateFlags flags)
	{
		flags_ = flags;
		return *this;
	}

	DescriptorPool::Builder& DescriptorPool::Builder::SetMaxSets(uint32_t maxSets)
	{
		maxSets_ = maxSets;
		return *this;
	}

	DescriptorPool::Builder& DescriptorPool::Builder::AddPoolSize(VkDescriptorType type, uint32_t count) {
		VkDescriptorPoolSize size{};
		size.type = type;
		size.descriptorCount = count;
		poolSizes_.push_back(size);
		return *this;
	}

	DescriptorPool DescriptorPool::Builder::Build()
	{
		if (maxSets_ == 0)
			throw std::runtime_error("DescriptorPool::Builder: maxSets is 0.");

		if (poolSizes_.empty()) 
			throw std::runtime_error("DescriptorPool::Builder: no pool sizes specified.");

		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.flags = flags_;
		info.maxSets = maxSets_;
		info.poolSizeCount = static_cast<uint32_t>(poolSizes_.size());
		info.pPoolSizes = poolSizes_.data();

		VkDescriptorPool pool = VK_NULL_HANDLE;
		if (vkCreateDescriptorPool(device_, &info, nullptr, &pool) != VK_SUCCESS)
			throw std::runtime_error("DescriptorPool::Builder: failed to create VkDescriptorPool.");

		return DescriptorPool(pool, device_);
	}

	DescriptorSet DescriptorSet::Allocate(VkDevice device, const VkDescriptorPool& pool, VkDescriptorSetLayout layout)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet set;
		if (vkAllocateDescriptorSets(device, &allocInfo, &set) != VK_SUCCESS)
			throw std::runtime_error("DescriptorSet allocated failed!");

		return DescriptorSet(set);
	}

	DescriptorSet& DescriptorSet::WriteBuffer(uint32_t binding, VkDescriptorType type, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t arrayElement)
	{
		BufferWrite w{};
		w.binding = binding;
		w.arrayElement = arrayElement;
		w.type = type;
		w.info.buffer = buffer;
		w.info.offset = offset;
		w.info.range = range;
		bufferWrites.push_back(w);
		return *this;
	}

	DescriptorSet& DescriptorSet::WriteBuffer(uint32_t binding, VkDescriptorType type, const VkDescriptorBufferInfo& info, uint32_t arrayElement)
	{
		BufferWrite w{};
		w.binding = binding;
		w.arrayElement = arrayElement;
		w.type = type;
		w.info = info;
		bufferWrites.push_back(w);
		return *this;
	}

	DescriptorSet& DescriptorSet::WriteImage(uint32_t binding, VkDescriptorType type, VkImageView imageView, VkSampler sampler, VkImageLayout layout, uint32_t arrayElement)
	{
		VkDescriptorImageInfo info{};
		info.imageView = imageView;
		info.sampler = sampler;
		info.imageLayout = layout;
		return WriteImage(binding, type, info, arrayElement);
	}

	DescriptorSet& DescriptorSet::WriteImage(uint32_t binding, VkDescriptorType type, const VkDescriptorImageInfo& info, uint32_t arrayElement)
	{
		ImageWrite w{};
		w.binding = binding;
		w.arrayElement = arrayElement;
		w.type = type;
		w.info = info;
		imageWrites.push_back(w);
		return *this;
	}

	DescriptorSet& DescriptorSet::WriteImageArray(uint32_t binding, VkDescriptorType type, const std::vector<VkDescriptorImageInfo>& infos, uint32_t firstArrayElement)
	{
		if (infos.empty()) return *this;
		for (uint32_t i = 0; i < infos.size(); ++i) 
		{
			ImageWrite w{};
			w.binding = binding;
			w.arrayElement = firstArrayElement + i;
			w.type = type;
			w.info = infos[i];
			imageWrites.push_back(w);
		}
		return *this;
	}

	DescriptorSet& DescriptorSet::WriteCombinedImageSampler(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, uint32_t arrayElement)
	{
		return WriteImage(
			binding,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			imageView, sampler, layout,
			arrayElement
		);
	}

	DescriptorSet& DescriptorSet::WriteUniformBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t arrayElement)
	{
		return WriteBuffer(
			binding,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			buffer, offset, range,
			arrayElement
		);
	}

	DescriptorSet& DescriptorSet::WriteStorageBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t arrayElement)
	{
		return WriteBuffer(
			binding,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			buffer, offset, range,
			arrayElement
		);
	}

	void DescriptorSet::Update(VkDevice device)
	{
		if (handle == VK_NULL_HANDLE) 
			throw std::runtime_error("DescriptorSet::Update: descriptor set is null.");
		
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo>  imageInfos;
		bufferInfos.reserve(bufferWrites.size());
		imageInfos.reserve(imageWrites.size());

		std::vector<VkWriteDescriptorSet> writes;
		writes.reserve(bufferWrites.size() + imageWrites.size());

		for (const auto& bw : bufferWrites) 
		{
			bufferInfos.push_back(bw.info);
			VkWriteDescriptorSet w{};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = handle;
			w.dstBinding = bw.binding;
			w.dstArrayElement = bw.arrayElement;
			w.descriptorType = bw.type;
			w.descriptorCount = 1;
			w.pBufferInfo = &bufferInfos.back();
			writes.push_back(w);
		}

		for (const auto& iw : imageWrites) 
		{
			imageInfos.push_back(iw.info);
			VkWriteDescriptorSet w{};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = handle;
			w.dstBinding = iw.binding;
			w.dstArrayElement = iw.arrayElement;
			w.descriptorType = iw.type;
			w.descriptorCount = 1;
			w.pImageInfo = &imageInfos.back();
			writes.push_back(w);
		}

		if (!writes.empty())
		{
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		}

		bufferWrites.clear();
		imageWrites.clear();
	}

}
	