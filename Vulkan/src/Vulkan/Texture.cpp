
#define STB_IMAGE_IMPLEMENTATION

#include "Texture.h"

namespace VK
{
	std::unique_ptr<uint8_t[]> Texture::LoadFile(const char* filepath, VkExtent2D& extent, formatInfo requiredFormatInfo)
	{
		return LoadFile_Internal(filepath, 0, extent, requiredFormatInfo);
	}

	std::unique_ptr<uint8_t[]> Texture::LoadFile(const uint8_t* fileBinaries, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo)
	{
		return LoadFile_Internal(fileBinaries, fileSize, extent, requiredFormatInfo);
	}

#ifdef WIN32
	std::pair<const uint8_t*, size_t> Texture::LoadResourceFromModule(int32_t resourceId, HMODULE hModule)
	{
		if (HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA))
			if (HGLOBAL hData = LoadResource(hModule, hResource))
				if (const uint8_t* pData = static_cast<uint8_t*>(LockResource(hData)))
					return { pData, SizeofResource(hModule, hResource) };
		return {};
	}
#endif
	
	uint32_t Texture::CalculateMipLevelCount(VkExtent2D extent)
	{
		return uint32_t(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
	}

	void Texture::CopyBlitAndGenerateMipmap2d(VkBuffer buffer_copyFrom, VkImage image_copyTo, 
		VkImage image_blitTo, VkExtent2D imageExtent, uint32_t mipLevelCount, uint32_t layerCount, VkFilter minFilter)
	{
		static constexpr ImageOperation::ImageMemoryBarrierParameterPack imbs[2] = {
			{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
		};

		bool generateMipmap = mipLevelCount > 1;

		bool blitMipLevel0 = image_copyTo != image_blitTo;

		auto& commandBuffer = VkBase::Plus().CommandBuffer_Transfer();
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			VkBufferImageCopy region = {
				.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
				.imageExtent = { imageExtent.width, imageExtent.height, 1 }
			};

			ImageOperation::CmdCopyBufferToImage(commandBuffer, buffer_copyFrom, image_copyTo, region,
				{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap || blitMipLevel0]);

			if (blitMipLevel0)
			{
				VkImageBlit region = {
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
					{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
					{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
				};

				ImageOperation::CmdBlitImage(commandBuffer, image_copyTo, image_blitTo, region,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
			}

			if (generateMipmap)
			{
				ImageOperation::CmdGenerateMipmap2d(commandBuffer, image_blitTo, imageExtent, mipLevelCount, layerCount,
					{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
			}
		}
		commandBuffer.End();

		VkBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
	}

	void Texture::BlitAndGenerateMipmap2d(VkImage image_preinitialized, VkImage image_final, VkExtent2D imageExtent, uint32_t mipLevelCount, uint32_t layerCount, VkFilter minFilter)
	{
		static constexpr ImageOperation::ImageMemoryBarrierParameterPack imbs[2] = {
			{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
		};

		// the precondition of generating mipmap is the Scene of mip should larger then 1
		bool generateMipmap = mipLevelCount > 1;

		// The condition for the occurrence of blit is that the source image and the target image are different
		bool blitMipLevel0 = image_preinitialized != image_final;


		if (generateMipmap || blitMipLevel0)
		{
			// Record the command if the conditions are met
			auto& commandBuffer = VkBase::Plus().CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			{
				if (blitMipLevel0)
				{
					VkImageMemoryBarrier imageMemoryBarrier = {
						VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						nullptr,
						0,
						VK_ACCESS_TRANSFER_READ_BIT,
						VK_IMAGE_LAYOUT_PREINITIALIZED,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						image_preinitialized,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

					VkImageBlit region = {
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
						{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
						{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
					};

					ImageOperation::CmdBlitImage(commandBuffer, image_preinitialized, image_final, region,
						{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);

					if (generateMipmap)
					{
						ImageOperation::CmdGenerateMipmap2d(commandBuffer, image_final, imageExtent, mipLevelCount, layerCount,
							{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
					}
				}
			}

			commandBuffer.End();

			VkBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
		}
	}

	VkSamplerCreateInfo Texture::MakeSamplerCreateInfo()
	{
		return {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = VkBase::Base().PhysicalDevice().Properties().limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.f,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = {},
			.unnormalizedCoordinates = VK_FALSE
		};
	}

	void Texture::CreateImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, 
		uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageCreateFlags flags)
	{
		VkImageCreateInfo imageCreateInfo = {
		   .flags = flags,
		   .imageType = imageType,
		   .format = format,
		   .extent = extent,
		   .mipLevels = mipLevelCount,
		   .arrayLayers = arrayLayerCount,
		   .samples = VK_SAMPLE_COUNT_1_BIT,
		   .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		};
		imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void Texture::CreateImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageViewCreateFlags flags)
	{
		imageView.Create(imageMemory.ImageRef(), viewType, format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, arrayLayerCount }, flags);
	}

	void Texture2d::Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap)
	{
		uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;

		// Create image and allocate memory
		CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, 1);

		// Create image view
		CreateImageView(VK_IMAGE_VIEW_TYPE_2D, format_final, mipLevelCount, 1);

		if (format_initial == format_final)
		{
			CopyBlitAndGenerateMipmap2d(StagingBuffer::Buffer_MainThread(), imageMemory.ImageRef(), imageMemory.ImageRef(), extent, mipLevelCount, 1);
		}
		else
		{
			if (VkImage image_conversion = StagingBuffer::AliasedImage2d_MainThread(format_initial, extent))
			{
				BlitAndGenerateMipmap2d(image_conversion, imageMemory.ImageRef(), extent, mipLevelCount, 1);
			}
			else
			{
				VkImageCreateInfo imageCreateInfo = {
					.imageType = VK_IMAGE_TYPE_2D,
					.format = format_initial,
					.extent = { extent.width, extent.height, 1 },
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
				};

				ImageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				CopyBlitAndGenerateMipmap2d(StagingBuffer::Buffer_MainThread(), imageMemory_conversion.ImageRef(), imageMemory.ImageRef(), extent, mipLevelCount, 1);
			}
		}
	}

	Texture2d::Texture2d(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap)
	{
		Create(filepath, format_initial, format_final, generateMipmap);
	}

	Texture2d::Texture2d(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap)
	{
		Create(pImageData, extent, format_initial, format_final, generateMipmap);
	}

	void Texture2d::Create(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap)
	{
		VkExtent2D extent;
		formatInfo formatInfo = FormatInfo(format_initial); // Get the format information based on specific format_initial
		std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, extent, formatInfo);
		if (pImageData)
			Create(pImageData.get(), extent, format_initial, format_final, generateMipmap);
	}

	void Texture2d::Create(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap)
	{
		this->extent = extent;
		size_t imageDataSize = size_t(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
		StagingBuffer::BufferData_MainThread(pImageData, imageDataSize);// copy data to stage buffer
		Create_Internal(format_initial, format_final, generateMipmap);
	}
}