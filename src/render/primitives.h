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
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription out{};
		out.binding = 0;
		out.stride = sizeof(Vertex);
		out.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return out;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetVertexInputAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 2> out{};

		out[0].binding = 0;
		out[0].location = 0;
		out[0].format = VK_FORMAT_R32G32_SFLOAT;
		out[0].offset = offsetof(Vertex, position);

		out[1].binding = 0;
		out[1].location = 1;
		out[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		out[1].offset = offsetof(Vertex, color);

		return out;
	}

};