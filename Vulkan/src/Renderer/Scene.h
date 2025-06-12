#pragma once

#include <vector>

#include "RenderObject.h"
#include "Camera.h"

namespace LittleEngine
{
	using namespace VK;

	class Scene
	{
	public:

		void Initialize();
		void UpdateObject(float dt);

		// Getter
		Camera& GetCam() { return cam; }
		std::vector<RenderObject>& GetRenderObjects() { return renderObjects; }

	private:
		Camera cam;
		std::vector<RenderObject> renderObjects;

	private:
		void InitialScene();
	};
}


