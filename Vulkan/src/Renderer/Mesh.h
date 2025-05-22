#pragma once

#include <vector>
#include "glm.hpp"

namespace LittleEngine
{
	enum class MeshBuffer
	{
		VERTEX = 0,
		COLOUR,
		TEXCOORD,
		NORMAL,
		TANGENT,
		BiTANGENT,

		MAXBUFFER
	};

	class Mesh
	{
	public:
		static Mesh* GenerateTriangle();
		static Mesh* GenerateQuad();
		static Mesh* GenerateCube();
		static Mesh* GenerateFloor();
		static Mesh* GenerateSphere();

		Mesh();

	protected:
		// Mesh attributes
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec4> colors;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> biTangents;
		std::vector<unsigned int> indices;
	};
}