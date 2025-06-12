#include "Vulkan/Window.h"

#include "Renderer/Level.h"

int main()
{
	using namespace VK;
	using namespace LittleEngine;

	std::string test = "";
	bool em = test.empty();

	Window* w = Window::CreateGLFWWindow({ 1280, 720 });
	if (w == nullptr)
		return -1;

	Level level;
	level.Initialize();

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

		level.RenderFrame();
		level.UpdateObject(w->GetDeltaTime());
	}

	w->TerminateWindow();

	return 0;
}