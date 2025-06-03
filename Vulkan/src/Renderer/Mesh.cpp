#include "Mesh.h"

namespace LittleEngine
{
	std::vector<std::shared_ptr<Mesh>> Mesh::defaultMeshes(DefaultMeshType::TYPENUM, nullptr);

	std::shared_ptr<Mesh> Mesh::GenerateTriangle()
	{
		if (defaultMeshes[TRIANGLE] != nullptr) return defaultMeshes[TRIANGLE];
		
		defaultMeshes[TRIANGLE] = std::make_shared<Mesh>();
		std::shared_ptr<Mesh> m(defaultMeshes[TRIANGLE]);

		m->vertices = {
			glm::vec3(-0.5f, -0.5f, 0.0f),
			glm::vec3(0.5f, -0.5f, 0.0f),
			glm::vec3(0.0f,  0.5f, 0.0f)
		};

		m->colors = {
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
		};

		m->texCoords = {
			glm::vec2(0.0f, 1.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(0.5f, 0.0f)
		};

		m->GenerateTangentCoords();

		return m;
	}

	std::shared_ptr<Mesh> Mesh::GenerateQuad()
	{
		if (defaultMeshes[QUAD] != nullptr) return defaultMeshes[QUAD];

		defaultMeshes[QUAD] = std::make_shared<Mesh>();
		std::shared_ptr<Mesh> m(defaultMeshes[QUAD]);

		m->vertices = {
			glm::vec3(-0.5f,  0.5f, 0.0f),  // top-left
			glm::vec3(0.5f,  0.5f, 0.0f),  // top-right
			glm::vec3(0.5f, -0.5f, 0.0f),  // bottom-right
			glm::vec3(-0.5f, -0.5f, 0.0f)   // bottom-left
		};

		m->colors = {
			glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)
		};

		m->texCoords = {
			glm::vec2(0.0f, 0.0f), // top-left
			glm::vec2(1.0f, 0.0f), // top-right
			glm::vec2(1.0f, 1.0f), // bottom-right
			glm::vec2(0.0f, 1.0f)  // bottom-left
		};

		m->indices = {
			0, 1, 2, // first triangle
			2, 3, 0  // second triangle
		};

		m->GenerateTangentCoords();

		return m;
	}

	std::shared_ptr<Mesh> Mesh::GenerateCube()
	{
		if (defaultMeshes[CUBE] != nullptr) return defaultMeshes[CUBE];

		defaultMeshes[CUBE] = std::make_shared<Mesh>();
		std::shared_ptr<Mesh> m(defaultMeshes[CUBE]);

		using glm::vec4;
		using glm::vec3;
		using glm::vec2;

		m->vertices = {
			// Front face
			vec3(-0.5f, -0.5f,  0.5f), // 0
			vec3(0.5f, -0.5f,  0.5f), // 1
			vec3(0.5f,  0.5f,  0.5f), // 2
			vec3(-0.5f,  0.5f,  0.5f), // 3

			// Back face
			vec3(0.5f, -0.5f, -0.5f), // 4
			vec3(-0.5f, -0.5f, -0.5f), // 5
			vec3(-0.5f,  0.5f, -0.5f), // 6
			vec3(0.5f,  0.5f, -0.5f), // 7

			// Left face
			vec3(-0.5f, -0.5f, -0.5f), // 8
			vec3(-0.5f, -0.5f,  0.5f), // 9
			vec3(-0.5f,  0.5f,  0.5f), // 10
			vec3(-0.5f,  0.5f, -0.5f), // 11

			// Right face
			vec3(0.5f, -0.5f,  0.5f), // 12
			vec3(0.5f, -0.5f, -0.5f), // 13
			vec3(0.5f,  0.5f, -0.5f), // 14
			vec3(0.5f,  0.5f,  0.5f), // 15

			// Top face
			vec3(-0.5f,  0.5f,  0.5f), // 16
			vec3(0.5f,  0.5f,  0.5f), // 17
			vec3(0.5f,  0.5f, -0.5f), // 18
			vec3(-0.5f,  0.5f, -0.5f), // 19

			// Bottom face
			vec3(-0.5f, -0.5f, -0.5f), // 20
			vec3(0.5f, -0.5f, -0.5f), // 21
			vec3(0.5f, -0.5f,  0.5f), // 22
			vec3(-0.5f, -0.5f,  0.5f)  // 23
		};

		m->normals = {
			vec3(0.0f,  0.0f,  1.0f), vec3(0.0f,  0.0f,  1.0f),
			vec3(0.0f,  0.0f,  1.0f), vec3(0.0f,  0.0f,  1.0f), // Front

			vec3(0.0f,  0.0f, -1.0f), vec3(0.0f,  0.0f, -1.0f),
			vec3(0.0f,  0.0f, -1.0f), vec3(0.0f,  0.0f, -1.0f), // Back

			vec3(-1.0f,  0.0f,  0.0f), vec3(-1.0f,  0.0f,  0.0f),
			vec3(-1.0f,  0.0f,  0.0f), vec3(-1.0f,  0.0f,  0.0f), // Left

			vec3(1.0f,  0.0f,  0.0f), vec3(1.0f,  0.0f,  0.0f),
			vec3(1.0f,  0.0f,  0.0f), vec3(1.0f,  0.0f,  0.0f), // Right

			vec3(0.0f,  1.0f,  0.0f), vec3(0.0f,  1.0f,  0.0f),
			vec3(0.0f,  1.0f,  0.0f), vec3(0.0f,  1.0f,  0.0f), // Top

			vec3(0.0f, -1.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f),
			vec3(0.0f, -1.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)  // Bottom
		};

		m->colors =
		{
			// Front face - red
			vec4(1.0f, 0.0f, 0.0f, 1.0f),
			vec4(1.0f, 0.0f, 0.0f, 1.0f),
			vec4(1.0f, 0.0f, 0.0f, 1.0f),
			vec4(1.0f, 0.0f, 0.0f, 1.0f),

			// Back face - green
			vec4(0.0f, 1.0f, 0.0f, 1.0f),
			vec4(0.0f, 1.0f, 0.0f, 1.0f),
			vec4(0.0f, 1.0f, 0.0f, 1.0f),
			vec4(0.0f, 1.0f, 0.0f, 1.0f),

			// Left face - blue
			vec4(0.0f, 0.0f, 1.0f, 1.0f),
			vec4(0.0f, 0.0f, 1.0f, 1.0f),
			vec4(0.0f, 0.0f, 1.0f, 1.0f),
			vec4(0.0f, 0.0f, 1.0f, 1.0f),

			// Right face - yellow
			vec4(1.0f, 1.0f, 0.0f, 1.0f),
			vec4(1.0f, 1.0f, 0.0f, 1.0f),
			vec4(1.0f, 1.0f, 0.0f, 1.0f),
			vec4(1.0f, 1.0f, 0.0f, 1.0f),

			// Top face - cyan
			vec4(0.0f, 1.0f, 1.0f, 1.0f),
			vec4(0.0f, 1.0f, 1.0f, 1.0f),
			vec4(0.0f, 1.0f, 1.0f, 1.0f),
			vec4(0.0f, 1.0f, 1.0f, 1.0f),

			// Bottom face - purple
			vec4(1.0f, 0.0f, 1.0f, 1.0f),
			vec4(1.0f, 0.0f, 1.0f, 1.0f),
			vec4(1.0f, 0.0f, 1.0f, 1.0f),
			vec4(1.0f, 0.0f, 1.0f, 1.0f)
		};

		m->texCoords = {
			vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), // Front
			vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), // Back
			vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), // Left
			vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), // Right
			vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), // Top
			vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f)  // Bottom
		};

		m->indices = {
			0, 1, 2, 2, 3, 0,        // Front
			4, 5, 6, 6, 7, 4,        // Back
			8, 9,10,10,11, 8,        // Left
			12,13,14,14,15,12,       // Right
			16,17,18,18,19,16,       // Top
			20,21,22,22,23,20        // Bottom
		};

		m->GenerateTangentCoords();

		m->BufferData();

		return m;
	}

	std::shared_ptr<Mesh> Mesh::GenerateFloor()
	{
		if (defaultMeshes[FLOOR] != nullptr) return defaultMeshes[FLOOR];

		defaultMeshes[FLOOR] = std::make_shared<Mesh>();
		std::shared_ptr<Mesh> m(defaultMeshes[FLOOR]);

		m->vertices = {
			glm::vec3(-100.0f, 0.0f, -100.0f),
			glm::vec3(100.0f, 0.0f, -100.0f),
			glm::vec3(100.0f, 0.0f,  100.0f),
			glm::vec3(-100.0f, 0.0f,  100.0f),
		};

		m->normals = {
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		};

		m->texCoords = {
			glm::vec2(0.0f, 0.0f),
			glm::vec2(50.0f, 0.0f),
			glm::vec2(50.0f, 50.0f),
			glm::vec2(0.0f, 50.0f)
		};

		m->indices = {
			0, 1, 2, // first triangle
			2, 3, 0  // second triangle
		};

		m->GenerateTangentCoords();

		return m;
	}

	std::shared_ptr<Mesh> Mesh::GenerateSphere()
	{
		if (defaultMeshes[SPHERE] != nullptr) return defaultMeshes[SPHERE];

		defaultMeshes[SPHERE] = std::make_shared<Mesh>();
		std::shared_ptr<Mesh> m(defaultMeshes[SPHERE]);

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;

		using glm::vec2;
		using glm::vec3;

		// Generate vertices
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = static_cast<float>(x) / X_SEGMENTS;
				float ySegment = static_cast<float>(y) / Y_SEGMENTS;

				float theta = xSegment * 2.0f * PI;
				float phi = ySegment * PI;

				float xPos = std::cos(theta) * std::sin(phi);
				float yPos = std::cos(phi);
				float zPos = std::sin(theta) * std::sin(phi);

				vec3 pos = vec3(xPos, yPos, zPos);
				m->vertices.push_back(pos);
				m->normals.push_back(glm::normalize(pos));
				m->texCoords.push_back(vec2(xSegment, ySegment));
			}
		}

		// Generate indices triangle list
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x < X_SEGMENTS; ++x)
			{
				uint32_t i0 = y * (X_SEGMENTS + 1) + x;
				uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
				uint32_t i2 = (y + 1) * (X_SEGMENTS + 1) + (x + 1);
				uint32_t i3 = y * (X_SEGMENTS + 1) + (x + 1);

				m->indices.push_back(i0);
				m->indices.push_back(i1);
				m->indices.push_back(i2);

				m->indices.push_back(i0);
				m->indices.push_back(i2);
				m->indices.push_back(i3);
			}
		}
		
		m->GenerateTangentCoords();

		return m;
	}
	
	Mesh::Mesh()
	{
	}

	//std::array<VkBuffer, Mesh::MAXBUFFER> Mesh::GetVertexBuffers() const
	//{
	//	std::array<VkBuffer, MAXBUFFER> buffers;
	//	for (size_t i = 0; i < MAXBUFFER; ++i)
	//	{
	//		buffers[i] = vertexBuffers[i];
	//	}
	//	return buffers;
	//}

	void Mesh::BufferData()
	{
		vertexBuffers[VERTEX].Create(vertices.size() * sizeof(glm::vec3));
		vertexBuffers[VERTEX].TransferData(vertices.data());
		if (!texCoords.empty())
		{
			vertexBuffers[TEXCOORD].Create(texCoords.size() * sizeof(glm::vec2));
			vertexBuffers[TEXCOORD].TransferData(texCoords.data());
		}
		if (!normals.empty())
		{
			vertexBuffers[NORMAL].Create(normals.size() * sizeof(glm::vec3));
			vertexBuffers[NORMAL].TransferData(normals.data());
		}
		if (!tangents.empty())
		{
			vertexBuffers[TANGENT].Create(tangents.size() * sizeof(glm::vec3));
			vertexBuffers[TANGENT].TransferData(tangents.data());
		}
		if (!biTangents.empty())
		{
			vertexBuffers[BiTANGENT].Create(biTangents.size() * sizeof(glm::vec3));
			vertexBuffers[BiTANGENT].TransferData(biTangents.data());
		}
		if (!colors.empty())
		{
			vertexBuffers[COLOUR].Create(colors.size() * sizeof(glm::vec4));
			vertexBuffers[COLOUR].TransferData(colors.data());
		}
		if (!indices.empty())
		{
			indexBuffer.Create(indices.size() * sizeof(unsigned int));
			indexBuffer.TransferData(indices.data());
		}

		bindingDescriptions = {
			{.binding = 0, .stride = sizeof(glm::vec3), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }, // position
		    {.binding = 1, .stride = sizeof(glm::vec2), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }, // Taxcoord	
		    {.binding = 2, .stride = sizeof(glm::vec3), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }, // normal
		    {.binding = 3, .stride = sizeof(glm::vec3), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }, // tangent
		    {.binding = 4, .stride = sizeof(glm::vec3), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }, // bitangent
			{.binding = 5, .stride = sizeof(glm::vec4), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }, // color
		};

		attributeDescriptions = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // position
			{ 1, 1, VK_FORMAT_R32G32_SFLOAT, 0 }, // texCoord
			{ 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // normal
			{ 3, 3, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // tangent
			{ 4, 4, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // bitangent
			{ 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT, 0 }, // color
		};
	}

	void Mesh::GenerateTangentCoords(int numPrimitive)
	{
		if (vertices.empty() || texCoords.empty()) return;

		tangents.resize(vertices.size(), glm::vec3());
		biTangents.resize(vertices.size(), glm::vec3());

		
		bool bUseIndex = !indices.empty();
		size_t n = bUseIndex ? indices.size() : vertices.size();
		for (size_t i = 0; i < n; i += numPrimitive)
		{
			// Positions
			glm::vec3 pos1 = bUseIndex ? vertices[indices[i]] : vertices[i];
			glm::vec3 pos2 = bUseIndex ? vertices[indices[i + 1]] : vertices[i + 1];
			glm::vec3 pos3 = bUseIndex ? vertices[indices[i + 2]] : vertices[i + 2];

			// Texture coordinates
			glm::vec2 uv1 = bUseIndex ? texCoords[indices[i]] : texCoords[i];
			glm::vec2 uv2 = bUseIndex ? texCoords[indices[i + 1]] : texCoords[i + 1];
			glm::vec2 uv3 = bUseIndex ? texCoords[indices[i + 2]] : texCoords[i + 2];

			// calculate tangent/bitangent vectors of this triangle
			glm::vec3 tangent, bitangent;

			glm::vec3 edge1 = pos2 - pos1;
			glm::vec3 edge2 = pos3 - pos1;
			glm::vec2 deltaUV1 = uv2 - uv1;
			glm::vec2 deltaUV2 = uv3 - uv1;

			float denominator = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
			if (denominator != 0)
			{
				float f = 1.0f / denominator;

				tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
				tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
				tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

				bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
				bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
				bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			}
			else
			{
				tangent = glm::vec3(1.0f, 0.0f, 0.0f);
				bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			
			for (size_t j = 0; j < numPrimitive; ++j)
			{
				if (bUseIndex)
				{
					tangents[indices[i + j]] += tangent;
					biTangents[indices[i + j]] += bitangent;
				}
				else
				{
					tangents[i + j] += tangent;
					biTangents[i + j] += bitangent;
				}
			}
		}
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			if (glm::length(tangents[i]) != 0) tangents[i] = glm::normalize(tangents[i]);
			if (glm::length(biTangents[i]) != 0) biTangents[i] = glm::normalize(biTangents[i]);
		}
	}
}


