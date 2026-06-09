#pragma once

#include<array>

#include <glm/glm.hpp>
#include <glfw/glfw3.h>

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 perspective;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 tex_coords;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription out{};
		out.binding = 0;
		out.stride = sizeof(Vertex);
		out.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return out;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetVertexInputAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 3> out{};

		out[0].binding = 0;
		out[0].location = 0;
		out[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		out[0].offset = offsetof(Vertex, position);

		out[1].binding = 0;
		out[1].location = 1;
		out[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		out[1].offset = offsetof(Vertex, color);

		out[2].binding = 0;
		out[2].location = 2;
		out[2].format = VK_FORMAT_R32G32_SFLOAT;
		out[2].offset = offsetof(Vertex, tex_coords);

		return out;
	}

};