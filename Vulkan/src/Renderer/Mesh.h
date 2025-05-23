#pragma once

#include <vector>
#include <memory>
#include "glm.hpp"

#include "../Vulkan/MemoryBuffers.h"

namespace LittleEngine
{
	using namespace VK;

	class Mesh
	{
	public:
		static std::shared_ptr<Mesh> GenerateTriangle();
		static std::shared_ptr<Mesh> GenerateQuad();
		static std::shared_ptr<Mesh> GenerateCube();
		static std::shared_ptr<Mesh> GenerateFloor();
		static std::shared_ptr<Mesh> GenerateSphere();

		Mesh();

		enum MeshBufferType
		{
			VERTEX = 0,
			TEXCOORD,
			NORMAL,
			TANGENT,
			BiTANGENT,
			COLOUR,

			MAXBUFFER
		};

	protected:
		// Mesh attributes
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> biTangents;
		std::vector<glm::vec4> colors;
		std::vector<unsigned int> indices;

		VertexBuffer vertexBuffers[MAXBUFFER];
		IndexBuffer indexBuffer;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		enum DefaultMeshType
		{
			TRIANGLE = 0,
			QUAD,
			CUBE,
			FLOOR,
			SPHERE,

			TYPENUM
		};
		static std::vector<std::shared_ptr<Mesh>> defaultMeshes;

	private:
		void BufferData();
		void GenerateTangentCoords(int numPrimitive = 3);
	};
}