#pragma once

#include <array>
#include <string>

#include <glm/glm.hpp>
#include <glfw/glfw3.h>

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 perspective;
};

#pragma region  Model

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 tex_coord0;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription out{};
		out.binding = 0;
		out.stride = sizeof(Vertex);
		out.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return out;
	}

	static std::array<VkVertexInputAttributeDescription, 4> GetVertexInputAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 4> out{};

		out[0].binding = 0;
		out[0].location = 0;
		out[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		out[0].offset = offsetof(Vertex, position);

		out[1].binding = 0;
		out[1].location = 1;
		out[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		out[1].offset = offsetof(Vertex, normal);

		out[2].binding = 0;
		out[2].location = 2;
		out[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		out[2].offset = offsetof(Vertex, tangent);

		out[3].binding = 0;
		out[3].location = 3;
		out[3].format = VK_FORMAT_R32G32_SFLOAT;
		out[3].offset = offsetof(Vertex, tex_coord0);

		return out;
	}
};

enum DRAW_MODE {
	POINTS,
	LINES,
	LINE_LOOP,
	LINE_STRIP,
	TRIANGLES,
	TRIANGLE_STRIP,
	TRIANGLE_FAN
};

struct Primitive {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	DRAW_MODE mode;
};

struct Mesh {
	std::string name;

	std::vector<Primitive> primitives;
};

#pragma endregion
