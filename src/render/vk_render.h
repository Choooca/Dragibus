#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct VkRenderContext;
struct VkContext;

const int MAX_FRAMES_IN_FLIGHT = 2;

class VkRender {

public:

	VkRender(VkRenderContext *render_context);
	~VkRender();

private:

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 perspective;
	};

	VkRenderContext* m_render_context;

	//Utils Buffer & Memory
	void CreateBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
	uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

	//Primitives Buffers 
	std::vector<VkBuffer> m_uniform_buffers;
	std::vector<VkDeviceMemory> m_uniform_buffers_memory;
	std::vector<void*> m_uniform_buffers_mapped_memory;
	void CreateUniformBuffer(std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory> &device_memory, std::vector<void*> &mapped_memory);

	//Descriptors
	VkDescriptorPool m_descriptor_pool;
	std::vector<VkDescriptorSet> m_descriptor_sets;

	VkDescriptorPool CreateDescriptorPool();
	std::vector<VkDescriptorSet> CreateDescriptorSets(const std::vector<VkBuffer> &uniform_buffers);
};

