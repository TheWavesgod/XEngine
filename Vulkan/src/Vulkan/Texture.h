#pragma once

#include "VkBase+.h"
#include "MemoryBuffers.h"

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
		static std::unique_ptr<uint8_t[]> LoadFile(const char* filepath, VkExtent2D& extent, formatInfo requiredFormatInfo);

		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const uint8_t* fileBinaries, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo);
		

		std::pair<const uint8_t*, size_t> LoadResourceFromModule(int32_t resourceId, HMODULE hModule = NULL);


		static uint32_t CalculateMipLevelCount(VkExtent2D extent);


		static void CopyBlitAndGenerateMipmap2d(VkBuffer buffer_copyFrom, VkImage image_copyTo, VkImage image_blitTo, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR);
		

		static void BlitAndGenerateMipmap2d(VkImage image_preinitialized, VkImage image_final, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR);
		

		static VkSamplerCreateInfo MakeSamplerCreateInfo();
		

	protected:
		ImageView imageView;
		ImageMemory imageMemory;

		//--------------------------
		Texture() = default;

		void CreateImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, 
			uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageCreateFlags flags = 0);

		void CreateImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevelCount,
			uint32_t arrayLayerCount, VkImageViewCreateFlags flags = 0);
		
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
				(requiredFormatInfo.rawDataType == formatInfo::integer && requiredFormatInfo.sizePerComponent >= 1
					&& requiredFormatInfo.sizePerComponent <= 2))
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
		void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap);

	public:
		Texture2d() = default;

		Texture2d(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true);
		Texture2d(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true);
		

		//Getter
		VkExtent2D Extent() const { return extent; }
		uint32_t Width() const { return extent.width; }
		uint32_t Height() const { return extent.height; }

		//Non-const Function
		// read file from hard disk straightly
		void Create(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true);

		// read file from memory
		void Create(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true);
	};
}