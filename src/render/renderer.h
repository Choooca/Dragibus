#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <render/swap_chain.h>
#include <render/vk_context.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

struct Renderer {

	//Render Pass
	VkRenderPass m_render_pass;

	//Pipeline
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline;

	//Command Buffers
	VkCommandPool m_graphics_command_pool;
	VkCommandPool m_transfer_command_pool;
	std::vector<VkCommandBuffer> m_graphics_command_buffer;

	//Uniform Buffers
	std::vector<VkBuffer> m_uniform_buffers;
	std::vector<VkDeviceMemory> m_uniform_buffers_memory;
	std::vector<void*> m_uniform_buffers_mapped_memory;

	//Primitive Buffer
	VkBuffer m_vertex_buffers;
	VkDeviceMemory m_vertex_buffers_memory;

	//Descriptors
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	std::vector<VkDescriptorSet> m_descriptor_sets;

	//Synchronisation
	std::vector<VkSemaphore> m_image_available_semaphore;
	std::vector<VkFence> m_in_flight_fence;

	//Loop
	uint32_t m_current_frame = 0;
};

Renderer *CreateRenderer(const VkContext& vk_context);
void DestroyRenderer(const VkContext& vk_context, Renderer *renderer);

template<typename T>
void CreatePrimitiveBuffer(const VkContext vk_context, const VkCommandPool &command_pool, std::vector<T> primitive_data, VkBuffer& buffer, VkDeviceMemory& device_memory, VkBufferUsageFlags buffer_usage) {
		
	VkDeviceSize buffer_size = sizeof(primitive_data[0]) * primitive_data.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	CreateBuffer(vk_context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
	
	void* data;
	vkMapMemory(vk_context.m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, primitive_data.data(), buffer_size);
	vkUnmapMemory(vk_context.m_device, staging_buffer_memory);

	CreateBuffer(vk_context, buffer_size, VK_IMAGE_USAGE_TRANSFER_DST_BIT | buffer_usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, device_memory);

	CopyBuffer(vk_context, command_pool, staging_buffer, buffer, buffer_size);

	vkDestroyBuffer(vk_context.m_device, staging_buffer, nullptr);
	vkFreeMemory(vk_context.m_device, staging_buffer_memory, nullptr);
}

void Loop();