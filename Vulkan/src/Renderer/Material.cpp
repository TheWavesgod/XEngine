#include "Material.h"

namespace LittleEngine
{
	void Material::LoadTexture(const std::string& path)
	{
		VkDescriptorSetLayoutBinding materialBindings[] = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // Albedo
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // Normal
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // AO
		};

		VkDescriptorSetLayoutCreateInfo materialLayoutCreateInfo = {
			.bindingCount = 6,
			.pBindings = materialBindings
		};
		descriptorSetLayout.Create(materialLayoutCreateInfo);

		// Update DescriptorSet
		VkDescriptorImageInfo imageInfo = {
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		
		for (size_t i = 0; i < static_cast<size_t>(TextureType::COUNT); ++i)
		{
			if (loadPaths[i].empty()) continue;

			imageInfo.sampler = samplers[i];
			imageInfo.imageView = textures[i].ImageViewRef();
			desciptorSet.Write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
		}
	}
}