#include "Vulkan/Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/Scene.h"

int main()
{
	using namespace VK;
	using namespace LittleEngine;

	std::string test = "";
	bool em = test.empty();

	Window* w = Window::CreateGLFWWindow({ 1280, 720 });
	if (w == nullptr)
		return -1;

	if (!Renderer::Get().Initialize()) return -1;

	Scene Scene;
	Scene.Initialize();

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

		Scene.UpdateObject(w->GetDeltaTime());
		Renderer::Get().RenderFrame(Scene);
	}

	w->TerminateWindow();

	return 0;
}