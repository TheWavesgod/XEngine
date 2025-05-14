#pragma once

#include "VKEasyHeader.h"

namespace VK
{
	class Surface
	{
		VkSurfaceKHR handle = VK_NULL_HANDLE;

	public:
		Surface() = default;

		// Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;

		inline VkSurfaceKHR& Ref() { return handle; }
	};
}


