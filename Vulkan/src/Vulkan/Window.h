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

		void TerminateWindow();
		bool Update();

		static inline float mInputX = 0.0f;
		static inline float mInputY = 0.0f;
		static inline float mScroll = 0.0f;
		static inline bool enableMouseCursor = true;

		static int GetInputState(int key);

		void SwitchMouseInputMode();

		double GetDeltaTime() const { return dt; }

		void SetWindowShouldClose(bool bShouldClose = true);

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
	void MouseButton_callback(GLFWwindow* window, int button, int action, int mods);
	void Scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
}
