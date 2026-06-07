#pragma once

#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <render/vk_render_context.h>
#include <render/vk_context.h>

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
	void CopyBuffer(const VkBuffer& src_buffer, const VkBuffer& dst_buffer, VkDeviceSize size);
	
	uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

	//Command Buffers
	VkCommandPool m_graphics_command_pool;
	VkCommandPool m_transfer_command_pool;
	std::vector<VkCommandBuffer> m_graphics_command_buffer;

	VkCommandPool CreateCommandPool(uint32_t queue_family_index, VkCommandPoolCreateFlagBits flags);
	std::vector<VkCommandBuffer> CreateCommandBuffer(const VkCommandPool& command_pool, uint32_t count);

	VkCommandBuffer BeginSingleCommandBuffer(const VkCommandPool &command_pool);
	void EndSingleCommandBuffer(const VkCommandBuffer& command_buffer, const VkCommandPool& command_pool, const VkQueue &queue);

	//Primitives Buffers 
	std::vector<VkBuffer> m_uniform_buffers;
	std::vector<VkDeviceMemory> m_uniform_buffers_memory;
	std::vector<void*> m_uniform_buffers_mapped_memory;

	void CreateUniformBuffer(std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory> &device_memory, std::vector<void*> &mapped_memory);

	template<typename T>
	void CreatePrimitiveBuffer(std::vector<T> primitive_data, VkBuffer& buffer, VkDeviceMemory& device_memory, VkBufferUsageFlags buffer_usage) {
		
		VkDeviceSize buffer_size = sizeof(primitive_data[0]) * primitive_data.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
	
		void* data;
		vkMapMemory(m_render_context->m_vk_context->m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, primitive_data.data(), buffer_size);
		vkUnmapMemory(m_render_context->m_vk_context->m_device, staging_buffer_memory);

		CreateBuffer(buffer_size, VK_IMAGE_USAGE_TRANSFER_DST_BIT | buffer_usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, device_memory);

		CopyBuffer(staging_buffer, buffer, buffer_size);

		vkDestroyBuffer(m_render_context->m_vk_context->m_device, staging_buffer, nullptr);
		vkFreeMemory(m_render_context->m_vk_context->m_device, staging_buffer_memory, nullptr);
	}

	VkBuffer m_vertex_buffers;
	VkDeviceMemory m_vertex_buffers_memory;

	void CreateVertexBuffer(std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& device_memory, std::vector<void*>& mapped_memory);

	//Descriptors
	VkDescriptorPool m_descriptor_pool;
	std::vector<VkDescriptorSet> m_descriptor_sets;

	VkDescriptorPool CreateDescriptorPool();
	std::vector<VkDescriptorSet> CreateDescriptorSets(const std::vector<VkBuffer> &uniform_buffers);
};

