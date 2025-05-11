#pragma once

#include "VkBase+.h"

namespace VK
{
	class texture {
	protected:
		static std::unique_ptr<uint8_t[]> LoadFile_Internal(
			const auto* address,             // string pointer or memory address of resource
			size_t fileSize,                 // size of file, only needed when the parameter is address of resource
			VkExtent2D& extent,              // size of image, define by stb_image function
			formatInfo requiredFormatInfo) 
		{ 
#ifndef NDEBUG
			if (//若要求数据为浮点数，stb_image只支持32位浮点数
				(requiredFormatInfo.rawDataType == formatInfo::floatingPoint && requiredFormatInfo.sizePerComponent == 4) ||
				//若要求数据为整形，stb_image只支持8位或16位每通道
				(requiredFormatInfo.rawDataType == formatInfo::integer && requiredFormatInfo.sizePerComponent >= 1 && requiredFormatInfo.sizePerComponent <= 2))
				/*空表达式*/;
			else
				outStream << std::format("[ texture ] ERROR\nRequired format is not available for source image data!\n"),
				abort();
#endif

			int& width = reinterpret_cast<int&>(extent.width);
			int& height = reinterpret_cast<int&>(extent.height);
			int channelCount;
			void* pImageData = nullptr;//用于接收读取到的图像数据

			//编译期分支：若传入的address是文件路径（字符串）
			if constexpr (std::same_as<decltype(address), const char*>) {
				/*待填充*/
			}

			//编译期分支：若传入的address是内存地址
			if constexpr (std::same_as<decltype(address), const uint8_t*>) {
				/*待填充*/
			}

			return std::unique_ptr<uint8_t[]>(static_cast<uint8_t*>(pImageData));
		}
	};
}