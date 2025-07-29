#include "Window.h"

namespace VK
{
	static bool firstMouse = true;
	static float lastMouseX;
	static float lastMouseY;

	Window* Window::CreateGLFWWindow(VkExtent2D size, const std::string& title, bool fullScreen, bool isResizable, bool limitFrameRate)
	{
		if (windowPtr != nullptr) return windowPtr;

		windowPtr = new Window();
		if (!windowPtr->Initialize(size, title, fullScreen, isResizable, limitFrameRate))
		{
			delete windowPtr;
			windowPtr = nullptr;
		}
		return windowPtr;
	}

	bool Window::Initialize(VkExtent2D size, const std::string& title, bool fullScreen, bool isResizable, bool limitFrameRate)
	{
		if (!glfwInit())
		{
			std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
			return false;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // with no API for vulkan
		glfwWindowHint(GLFW_RESIZABLE, isResizable);

		// Get Necessary Extensions 
		extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
		if (!extensionNames)
		{
			std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
			glfwTerminate();
			return false;
		}

		pMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);

		pWindow = fullScreen ?
			glfwCreateWindow(pMode->width, pMode->height, title.data(), nullptr, nullptr) :
			glfwCreateWindow((int)size.width, size.height, title.data(), nullptr, nullptr);

		this->title = title;
		this->limitFrameRate = limitFrameRate;

		if (!pWindow)
		{
			std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
			glfwTerminate();
			return false;
		}

		// Set callback function
		glfwSetCursorPosCallback(pWindow, CursorPosition_callback);
		glfwSetMouseButtonCallback(pWindow, MouseButton_callback);
		glfwSetScrollCallback(pWindow, Scroll_callback);

		return true;
	}

	result_t Window::CreateSuface()
	{
		VkResult result = glfwCreateWindowSurface(VK::VkBase::Base().Instance(), pWindow, nullptr, &VK::VkBase::Base().SurfaceRef());
		if (result)
		{
			std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
		}
		return result;
	}


	void Window::TerminateWindow()
	{
		VK::VkBase::Base().WaitIdle();
		glfwTerminate();
	}

	bool Window::Update()
	{
		if (glfwWindowShouldClose(pWindow)) return false;

		static double preFrameTime = glfwGetTime();
		static double curFrameTime;
		static int dframe = -1;
		static std::stringstream info;

		curFrameTime = glfwGetTime();
		++dframe;

		if ((dt = curFrameTime - preFrameTime) >= 1)
		{
			info.precision(1);
			info << title << "    " << std::fixed << dframe / dt << " FPS";
			glfwSetWindowTitle(pWindow, info.str().c_str());
			info.str(""); // clear the buffer when set the fps
			preFrameTime = curFrameTime;
			dframe = 0;
		}

		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED)) glfwWaitEvents();

		mInputX = 0.0f;
		mInputY = 0.0f;
		mScroll = 0.0f;

		glfwPollEvents();

		return true;
	}

	int Window::GetInputState(int key)
	{
		return glfwGetKey(windowPtr->pWindow, key);
	}

	void Window::SwitchMouseInputMode()
	{
		if (enableMouseCursor)
		{
			firstMouse = true;
			glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		enableMouseCursor = !enableMouseCursor;
	}

	void Window::SetWindowShouldClose(bool bShouldClose)
	{
		glfwSetWindowShouldClose(pWindow, bShouldClose);
	}

	

	void CursorPosition_callback(GLFWwindow* window, double xpos, double ypos)
	{
		if (Window::enableMouseCursor) return;

		if (firstMouse)
		{
			lastMouseX = xpos;
			lastMouseY = ypos;
			firstMouse = false;
		}

		Window::mInputX = xpos - lastMouseX;
		Window::mInputY = lastMouseY - ypos; // reversed since y-coordinates range from bottom to top

		lastMouseX = xpos;
		lastMouseY = ypos;
	}

	void MouseButton_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{

		}
	}

	void Scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		Window::mScroll = yoffset;
	}
}