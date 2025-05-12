#pragma once

#include "VkBase+.h"

#define STB_IMAGE_IMPLEMENTATION 
#include <stb_image.h>

namespace VK
{
	class Texture 
	{
	public:
		//Getter
		VkImageView ImageViewRef() const { return imageView; }
		VkImage ImageRef() const { return imageMemory.ImageRef(); }
		const VkImageView* AddressOfImageView() const { return imageView.Address(); }
		const VkImage* AddressOfImage() const { return imageMemory.AddressOfImage(); }

		//Const Function
		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const 
		{
			return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}

		//Static Function
		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const char* filepath, VkExtent2D& extent, formatInfo requiredFormatInfo) 
		{
			return LoadFile_Internal(filepath, 0, extent, requiredFormatInfo);
		}

		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const uint8_t* fileBinaries, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo) 
		{
			return LoadFile_Internal(fileBinaries, fileSize, extent, requiredFormatInfo);
		}

		std::pair<const uint8_t*, size_t> LoadResourceFromModule(int32_t resourceId, HMODULE hModule = NULL) 
		{
			if (HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA))
				if (HGLOBAL hData = LoadResource(hModule, hResource))
					if (const uint8_t* pData = static_cast<uint8_t*>(LockResource(hData)))
						return { pData, SizeofResource(hModule, hResource) };
			return {};
		}

		static uint32_t CalculateMipLevelCount(VkExtent2D extent)
		{
			return uint32_t(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
		}

		static void CopyBlitAndGenerateMipmap2d(VkBuffer buffer_copyFrom, VkImage image_copyTo, VkImage image_blitTo, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR)
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

		static void BlitAndGenerateMipmap2d(VkImage image_preinitialized, VkImage image_final, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR)
		{
			static constexpr ImageOperation::ImageMemoryBarrierParameterPack imbs[2] = {
				{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
			};

			// the precondition of generating mipmap is the level of mip should larger then 1
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

		static VkSamplerCreateInfo MakeSamplerCreateInfo()
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
				.maxAnisotropy = VkBase::Base().PhysicalDeviceProperties().limits.maxSamplerAnisotropy,
				.compareEnable = VK_FALSE,
				.compareOp = VK_COMPARE_OP_ALWAYS,
				.minLod = 0.f,
				.maxLod = VK_LOD_CLAMP_NONE,
				.borderColor = {},
				.unnormalizedCoordinates = VK_FALSE
			};
		}

	protected:
		ImageView imageView;
		ImageMemory imageMemory;

		//--------------------------
		Texture() = default;

		void CreateImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageCreateFlags flags = 0)
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

		void CreateImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageViewCreateFlags flags = 0) 
		{
			imageView.Create(imageMemory.ImageRef(), viewType, format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, arrayLayerCount }, flags);
		}

		static std::unique_ptr<uint8_t[]> LoadFile_Internal(
			const auto* address,             // string pointer or memory address of resource
			size_t fileSize,                 // size of file, only needed when the parameter is address of resource
			VkExtent2D& extent,              // size of image, define by stb_image function
			formatInfo requiredFormatInfo) 
		{ 
#ifndef NDEBUG
			if ( // if data need to be float, stb_image only support 32bit float
				(requiredFormatInfo.rawDataType == formatInfo::floatingPoint && requiredFormatInfo.sizePerComponent == 4) ||
				//  if data need to be int, stb_image only support 8bit or 16bit per channel
				(requiredFormatInfo.rawDataType == formatInfo::integer && requiredFormatInfo.sizePerComponent >= 1 && requiredFormatInfo.sizePerComponent <= 2))
				/* empty expression */;
			else
				outStream << std::format("[ Texture ] ERROR\nRequired format is not available for source image data!\n"),
				abort();
#endif

			int& width = reinterpret_cast<int&>(extent.width);
			int& height = reinterpret_cast<int&>(extent.height);
			int channelCount;
			void* pImageData = nullptr; 

			
			if constexpr (std::same_as<decltype(address), const char*>) 
			{
				if (requiredFormatInfo.rawDataType == formatInfo::integer)
					if (requiredFormatInfo.sizePerComponent == 1)
						pImageData = stbi_load(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
					else
						pImageData = stbi_load_16(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				else
					pImageData = stbi_loadf(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				if (!pImageData)
					outStream << std::format("[ texture ] ERROR\nFailed to load the file: {}\n", address);
			}

			if constexpr (std::same_as<decltype(address), const uint8_t*>) 
			{
				if (fileSize > INT32_MAX) 
				{
					outStream << std::format("[ texture ] ERROR\nFailed to load image data from the given address! Data size must be less than 2G!\n");
					return {};
				}
				if (requiredFormatInfo.rawDataType == formatInfo::integer)
					if (requiredFormatInfo.sizePerComponent == 1)
						pImageData = stbi_load_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
					else
						pImageData = stbi_load_16_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				else
					pImageData = stbi_loadf_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				if (!pImageData)
					outStream << std::format("[ texture ] ERROR\nFailed to load image data from the given address!\n");
			}

			return std::unique_ptr<uint8_t[]>(static_cast<uint8_t*>(pImageData));
		}
	};

	class Texture2d :public Texture
	{
	protected:
		VkExtent2D extent = {};
		//--------------------
		void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) 
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

	public:
		Texture2d() = default;

		Texture2d(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) 
		{
			Create(filepath, format_initial, format_final, generateMipmap);
		}

		Texture2d(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true)
		{
			Create(pImageData, extent, format_initial, format_final, generateMipmap);
		}

		//Getter
		VkExtent2D Extent() const { return extent; }
		uint32_t Width() const { return extent.width; }
		uint32_t Height() const { return extent.height; }

		//Non-const Function
		// read file from hard disk straightly
		void Create(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) 
		{
			VkExtent2D extent;
			formatInfo formatInfo = FormatInfo(format_initial); // Get the format information based on specific format_initial
			std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, extent, formatInfo);
			if (pImageData)
				Create(pImageData.get(), extent, format_initial, format_final, generateMipmap);
		}

		// read file from memory
		void Create(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) 
		{
			this->extent = extent;
			size_t imageDataSize = size_t(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
			StagingBuffer::BufferData_MainThread(pImageData, imageDataSize);// copy data to stage buffer
			Create_Internal(format_initial, format_final, generateMipmap);
		}
	};
}