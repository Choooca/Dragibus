#include "vk_render.h"

#include <array>

#include <render/vk_context.h>
#include <render/vk_render_context.h>
#include <utils/debug_macro.h>

#pragma region Constructors

VkRender::VkRender(VkRenderContext* render_context) : m_render_context(render_context) {

	CreateUniformBuffer(m_uniform_buffers, m_uniform_buffers_memory, m_uniform_buffers_mapped_memory);

	m_descriptor_pool = CreateDescriptorPool();
	m_descriptor_sets = CreateDescriptorSets(m_uniform_buffers);
}

VkRender::~VkRender()
{
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

