#include "Vulkan/Window.h"



int main()
{
	using namespace VK;

	Window* w = Window::CreateGLFWWindow({ 1280, 720 });
	if (w == nullptr)
		return -1;

	while (w->Update())
	{
		if (w->GetInputState(GLFW_KEY_TAB) == GLFW_PRESS)
		{
			w->SwitchMouseInputMode();
		}
		if (w->GetInputState(GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			w->SetWindowShouldClose();
		}


	}

	w->TerminateWindow();
}