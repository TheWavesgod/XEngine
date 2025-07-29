#include "Attachment.h"
#include "VkBase+.h"

namespace VK
{
	void ColorAttachment::Create(VkFormat format, VkExtent2D extent, 
		uint32_t layerCount, VkSampleCountFlagBits sampleCount, VkImageUsageFlags otherUsages)
	{
		VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = layerCount,
				.samples = sampleCount,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | otherUsages
		};
		imageMemory.Create(
			imageCreateInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);

		imageView.Create(
			imageMemory.ImageRef(),
			layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
			format,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount });
	}

	bool ColorAttachment::FormatAvailability(VkFormat format, bool supportBlending)
	{
		return FormatProperties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT << uint32_t(supportBlending);
	}

	void DepthStencilAttachment::Create(VkFormat format, VkExtent2D extent, 
		uint32_t layerCount, VkSampleCountFlagBits sampleCount, VkImageUsageFlags otherUsages, bool stencilOnly)
	{
		VkImageCreateInfo imageCreateInfo = {
		   .imageType = VK_IMAGE_TYPE_2D,
		   .format = format,
		   .extent = { extent.width, extent.height, 1 },
		   .mipLevels = 1,
		   .arrayLayers = layerCount,
		   .samples = sampleCount,
		   .tiling = VK_IMAGE_TILING_OPTIMAL,
		   .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | otherUsages,
		   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		if (result_t result = imageMemory.Create(
			imageCreateInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT))
		{
			outStream << std::format("[ DepthStencilAttachment ] ERROR\nFailed to create image memory for depthStencilAttachment!\nError code: {}\n", int32_t(result));
		}
		
		// Decide aspcet mask-------------------------
		VkImageAspectFlags aspectMask = (!stencilOnly) * VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format > VK_FORMAT_S8_UINT)
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		else if (format == VK_FORMAT_S8_UINT)
			aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		//----------------------------------------
		
		if (result_t result = imageView.Create(
			imageMemory.ImageRef(),
			layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
			format,
			{ aspectMask, 0, 1, 0, layerCount }))
		{
			outStream << std::format("[ DepthStencilAttachment ] ERROR\nFailed to create image view for depthStencilAttachment!\nError code: {}\n", int32_t(result));
		}
	}

	bool DepthStencilAttachment::FormatAvailability(VkFormat format)
	{
		return FormatProperties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;;
	}
}

