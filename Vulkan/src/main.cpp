#include "Vulkan/Window.h"



int main()
{
	using namespace VK;

	Window* w = Window::CreateGLFWWindow({ 1280, 720 });
	if (w == nullptr)
		return -1;

	while (w->Update())
	{

	}

	w->CloseWindow();
}