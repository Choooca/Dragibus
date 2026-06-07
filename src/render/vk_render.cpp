#include "vk_render.h"

#include <array>

#include <render/vk_context.h>
#include <utils/debug_macro.h>
#include <render/primitives.h>

#pragma region Constructors

VkRender::VkRender(VkRenderContext* render_context) : m_render_context(render_context) {

	std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	CreateUniformBuffer(m_uniform_buffers, m_uniform_buffers_memory, m_uniform_buffers_mapped_memory);

	m_descriptor_pool = CreateDescriptorPool();
	m_descriptor_sets = CreateDescriptorSets(m_uniform_buffers);
	m_graphics_command_pool = CreateCommandPool(m_render_context->m_vk_context->m_indices.graphics_family.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	m_transfer_command_pool = CreateCommandPool(m_render_context->m_vk_context->m_indices.transfer_family.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	m_graphics_command_buffer = CreateCommandBuffer(m_graphics_command_pool, MAX_FRAMES_IN_FLIGHT);
	CreatePrimitiveBuffer<Vertex>(vertices, m_vertex_buffers, m_vertex_buffers_memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	m_image_available_semaphore = CreateSemaphore(MAX_FRAMES_IN_FLIGHT);
	m_render_finish_semaphore = CreateSemaphore(m_render_context->m_swap_chain_images.size());
	m_in_flight_fence = CreateFence(MAX_FRAMES_IN_FLIGHT, VK_FENCE_CREATE_SIGNALED_BIT);
}

VkRender::~VkRender()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(m_render_context->m_vk_context->m_device, m_image_available_semaphore[i], nullptr);
		vkDestroyFence(m_render_context->m_vk_context->m_device, m_in_flight_fence[i], nullptr);
	}

	for (size_t i = 0; i < m_render_context->m_swap_chain_images.size(); ++i) {
		vkDestroySemaphore(m_render_context->m_vk_context->m_device, m_render_finish_semaphore[i], nullptr);
	}

	vkDestroyCommandPool(m_render_context->m_vk_context->m_device, m_graphics_command_pool, nullptr);
	vkDestroyCommandPool(m_render_context->m_vk_context->m_device, m_transfer_command_pool, nullptr);

	vkDestroyBuffer(m_render_context->m_vk_context->m_device, m_vertex_buffers, nullptr);
	vkFreeMemory(m_render_context->m_vk_context->m_device, m_vertex_buffers_memory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroyBuffer(m_render_context->m_vk_context->m_device, m_uniform_buffers[i], nullptr);
		vkFreeMemory(m_render_context->m_vk_context->m_device, m_uniform_buffers_memory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_render_context->m_vk_context->m_device, m_descriptor_pool, nullptr);
}

#pragma endregion

#pragma region Memory & Buffer

void VkRender::CreateBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	uint32_t shared_queue_family_index[] = { m_render_context->m_vk_context->m_indices.graphics_family.value(), m_render_context->m_vk_context->m_indices.transfer_family.value() };

	if (shared_queue_family_index[0] != shared_queue_family_index[1]) {
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = 2;
		buffer_info.pQueueFamilyIndices = shared_queue_family_index;
	}
	else {
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateBuffer(m_render_context->m_vk_context->m_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create buffer");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(m_render_context->m_vk_context->m_device, buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_render_context->m_vk_context->m_device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to allocate buffer memory")
	}

	vkBindBufferMemory(m_render_context->m_vk_context->m_device, buffer, buffer_memory, 0);
}

void VkRender::CopyBuffer(const VkBuffer& src_buffer, const VkBuffer& dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer transfer_command_buffer = BeginSingleCommandBuffer(m_transfer_command_pool);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(transfer_command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	EndSingleCommandBuffer(transfer_command_buffer, m_transfer_command_pool, m_render_context->m_vk_context->m_transfer_queue);
}

uint32_t VkRender::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(m_render_context->m_vk_context->m_physical_device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find suitable memory type");
}

#pragma endregion

#pragma region Primitives Memory

void VkRender::CreateUniformBuffer(std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory> &device_memory, std::vector<void*> &mapped_memory)
{
	VkDeviceSize size = sizeof(UniformBufferObject);

	buffers.resize(MAX_FRAMES_IN_FLIGHT);
	device_memory.resize(MAX_FRAMES_IN_FLIGHT);
	mapped_memory.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, buffers[i], device_memory[i]);
		vkMapMemory(m_render_context->m_vk_context->m_device, device_memory[i], 0, size, 0, &mapped_memory[i]);
	}
}

#pragma endregion

#pragma region Command Buffers

VkCommandPool VkRender::CreateCommandPool(uint32_t queue_family_index, VkCommandPoolCreateFlagBits flags)
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = flags;
	pool_info.queueFamilyIndex = queue_family_index;

	VkCommandPool out;
	if (vkCreateCommandPool(m_render_context->m_vk_context->m_device, &pool_info, nullptr, &out) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create command pool.");
	}

	return out;
}

std::vector<VkCommandBuffer> VkRender::CreateCommandBuffer(const VkCommandPool& command_pool, uint32_t count)
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandBufferCount = count;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	std::vector<VkCommandBuffer> out(count);
	if (vkAllocateCommandBuffers(m_render_context->m_vk_context->m_device, &alloc_info, out.data()) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create command buffer");
	}

	return out;
}

VkCommandBuffer VkRender::BeginSingleCommandBuffer(const VkCommandPool& command_pool)
{
	VkCommandBuffer out = CreateCommandBuffer(command_pool, 1)[0];

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(out, &begin_info);

	return out;
}

void VkRender::EndSingleCommandBuffer(const VkCommandBuffer& command_buffer, const VkCommandPool& command_pool, const VkQueue& queue)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(m_render_context->m_vk_context->m_device, command_pool, 1, &command_buffer);
}

#pragma endregion

#pragma region Descriptors

VkDescriptorPool VkRender::CreateDescriptorPool() {

	std::array<VkDescriptorPoolSize, 1> pool_sizes{};
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorPoolCreateInfo  pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPool descriptor_pool;
	if (vkCreateDescriptorPool(m_render_context->m_vk_context->m_device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Descriptor Pool");
	}

	return descriptor_pool;
}

std::vector<VkDescriptorSet> VkRender::CreateDescriptorSets(const std::vector<VkBuffer>& uniform_buffers) {

	std::vector<VkDescriptorSetLayout> descriptor_set_layouts(MAX_FRAMES_IN_FLIGHT, m_render_context->m_descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pool;
	alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	alloc_info.pSetLayouts = descriptor_set_layouts.data();

	std::vector<VkDescriptorSet> descriptor_sets(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_render_context->m_vk_context->m_device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to allocate Descriptor Sets");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		std::array<VkWriteDescriptorSet, 1> descriptor_writes{};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = descriptor_sets[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(m_render_context->m_vk_context->m_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}

	return descriptor_sets;
}

#pragma endregion

#pragma region Synchronisation

std::vector<VkSemaphore> VkRender::CreateSemaphore(uint32_t count, VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.flags = flags;

	std::vector<VkSemaphore> out(count);

	for (size_t i = 0; i < count; ++i) {
		if (vkCreateSemaphore(m_render_context->m_vk_context->m_device, &semaphore_info, nullptr, &out[i]) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Semaphore.");
		}
	}

	return out;
}

std::vector<VkFence> VkRender::CreateFence(uint32_t count, VkFenceCreateFlags flags)
{
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = flags;
	
	std::vector<VkFence> out(count);
	for (size_t i = 0; i < count; ++i) {
		if (vkCreateFence(m_render_context->m_vk_context->m_device, &fence_info, nullptr, &out[i]) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Fence.");
		}
	}

	return out;
}

#pragma endregion