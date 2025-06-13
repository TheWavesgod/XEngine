#pragma once

#include <vector>
#include <array>
#include "glm.hpp"

#include "Vulkan/MemoryBuffers.h"

namespace LittleEngine
{
	using namespace VK;

	class Mesh
	{
	public:
		static Mesh* GenerateTriangle();
		static Mesh* GenerateQuad();
		static Mesh* GenerateCube();
		static Mesh* GenerateFloor();
		static Mesh* GenerateSphere();

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

		bool UseIndices() const { return !indices.empty(); }

		// Getter
		const std::array<VertexBuffer, MAXBUFFER>& GetVertexBuffers() const { return vertexBuffers; }
		const IndexBuffer& GetIndexBuffer() const { return indexBuffer; }
		uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
		uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }

		const std::vector<VkVertexInputBindingDescription>& GetVertexInputBindings() const { return bindingDescriptions; }
		const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributes() const { return attributeDescriptions; }

	protected:
		// Mesh attributes
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> biTangents;
		std::vector<glm::vec4> colors;
		std::vector<uint32_t> indices;

		std::array<VertexBuffer, MAXBUFFER> vertexBuffers;
		IndexBuffer indexBuffer;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

	private:
		void BufferData();
		void GenerateTangentCoords(int numPrimitive = 3);
	};
}