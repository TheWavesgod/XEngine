#pragma once

#include "VkBase.h"
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#pragma comment(lib, "glfw3.lib") 

namespace VK
{
	class Window
	{
	public:
		static Window* CreateGLFWWindow(VkExtent2D size, const std::string& title = "Vulkan Renderer", bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true);
		static Window* GetPointer() { return windowPtr; }

		void CloseWindow();

		bool Update();

	private:
		inline static Window* windowPtr = nullptr;

		double dt = 0.0;

		std::string title;

		GLFWwindow* pWindow = nullptr;
		GLFWmonitor* pMonitor = nullptr;

		Window() = default;
		bool Initialize(VkExtent2D size, const std::string& title, bool fullScreen, bool isResizable, bool limitFrameRate);
	};

	void CursorPosition_callback(GLFWwindow* window, double xpos, double ypos);
}
