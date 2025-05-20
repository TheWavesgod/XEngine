#pragma once

#include "Image.h"

namespace VK
{
	class Attachment
	{
	protected:
		ImageView imageView;
		ImageMemory imageMemory;

		Attachment() = default;

	public:
		// Getter
		VkImageView ImageView() const { return imageView; }
		VkImage Image() const { return imageMemory.ImageRef(); }

		const VkImageView* AddressOfImageView() const { return imageView.Address(); }
		const VkImage* AddressOfImage() const { return imageMemory.AddressOfImage(); }

		// Const function
		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const {
			return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
	};

	class ColorAttachment : public Attachment
	{
	public:
		ColorAttachment() = default;
		ColorAttachment(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1,
			VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
			Create(format, extent, layerCount, sampleCount, otherUsages);
		}

		//Non-const Function
		void Create(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1,
			VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0);
		
		// Static Function
		// Used to check if a specific image can be treat as color attachment
		static bool FormatAvailability(VkFormat format, bool supportBlending = true);
	};

	class DepthStencilAttachment :public Attachment {
	public:
		DepthStencilAttachment() = default;
		DepthStencilAttachment(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1,
			VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0, bool stencilOnly = false) {
			Create(format, extent, layerCount, sampleCount, otherUsages, stencilOnly);
		}

		// Non-const Function
		void Create(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1,
			VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0, bool stencilOnly = false);

		// Static Function
		// Used to check if a specific image can be treat as color attachment
		static bool FormatAvailability(VkFormat format);
	};
}


