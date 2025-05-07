#pragma once

#include "VkBase.h"
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#pragma comment(lib, "glfw3.lib") 


// Pointer of window
inline GLFWwindow* pWindow = nullptr;

// Pointer of monitor
inline GLFWmonitor* pMonitor = nullptr;

inline const char* windowTitle = "Vulkan Renderer";

inline bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) 
{
	if (!glfwInit())
	{
		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // with no API for vulkan

	glfwWindowHint(GLFW_RESIZABLE, isResizable);


	// Get Necessary Extensions 
	uint32_t extensionCount = 0;
	const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
	if (!extensionNames)
	{
		std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
		glfwTerminate();
		return false;
	}
	for (size_t i = 0; i < extensionCount; i++)
	{
		VK::VkBase::Base().AddInstanceExtension(extensionNames[i]);
	}

	pMonitor = glfwGetPrimaryMonitor();

	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);

	pWindow = fullScreen ?
		glfwCreateWindow(pMode->width, pMode->height, windowTitle, nullptr, nullptr) :
		glfwCreateWindow((int)size.width, size.height, windowTitle, nullptr, nullptr);

	if (!pWindow)
	{
		std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
		glfwTerminate();
		return false;
	}

#ifdef _WIN32
	VK::VkBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
	VK::VkBase::Base().AddInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);                                                    
#else
	uint32_t extensionCount = 0;
	const char** extensionNames;
	extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
	if (!extensionNames) {
		std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
		glfwTerminate();
		return false;
	}
	for (size_t i = 0; i < extensionCount; i++)
	{
		VK::VkBase::Base().AddInstanceExtension(extensionNames[i]);
	}
#endif
	VK::VkBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	VK::VkBase::Base().UseLatestApiVersion();
	if (VK::VkBase::Base().CreateInstance()) return false;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (VkResult result = glfwCreateWindowSurface(VK::VkBase::Base().Instance(), pWindow, nullptr, &surface))
	{
		std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
		glfwTerminate();
		return false;
	}
	VK::VkBase::Base().Surface(surface);

	if (
		VK::VkBase::Base().GetPhysicalDevices() ||
		VK::VkBase::Base().DeterminePhysicalDevice(0, true, false) ||
		VK::VkBase::Base().CreateDevice()) 
	{
		return false;
	}

	if (VK::VkBase::Base().CreateSwapchain(limitFrameRate)) return false;
	
	return true;
}

inline void TerminateWindow() 
{
	VK::VkBase::Base().WaitIdle();
	glfwTerminate();
}

inline void TitleFps() {
	static double preTime = glfwGetTime();
	static double curTime;
	static double dt;
	static int dframe = -1;
	static std::stringstream info;
	curTime = glfwGetTime();
	dframe++;
	if ((dt = curTime - preTime) >= 1)
	{
		info.precision(1);
		info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
		glfwSetWindowTitle(pWindow, info.str().c_str());
		info.str("");//别忘了在设置完窗口标题后清空所用的stringstream
		preTime = curTime;
		dframe = 0;
	}
}
