#include "command_pool.h"
#include <utils/debug_macro.h>

Vulkan::CommandPool::CommandPool(VkDevice *device, uint32_t queue_family_index, VkCommandPoolCreateFlagBits flags)
	: m_device(device)
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = flags;
	pool_info.queueFamilyIndex = queue_family_index;

	if (vkCreateCommandPool(*m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create command pool.");
	}
}

Vulkan::CommandPool::~CommandPool()
{
	vkDestroyCommandPool(*m_device, m_command_pool, nullptr);
}

std::vector<VkCommandBuffer> Vulkan::CommandPool::CreateCommandBufferGroup(uint32_t count, const std::string& key)
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandBufferCount = count;
	alloc_info.commandPool = m_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	std::vector<VkCommandBuffer> command_buffer(count);
	if (vkAllocateCommandBuffers(*m_device, &alloc_info, command_buffer.data()) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create command buffer");
	}

	m_command_buffers_groups[key] = command_buffer;

	return m_command_buffers_groups[key];
}

void Vulkan::CommandPool::DestroyCommandBufferGroup(const std::string& key)
{
	if (m_command_buffers_groups.find(key) == m_command_buffers_groups.end()) {
		THROW_RUNTIME_ERROR("Given key : " + key + " isn't valid");
	}

	std::vector<VkCommandBuffer> command_buffer_group = std::move(m_command_buffers_groups[key]);
	vkFreeCommandBuffers(*m_device, m_command_pool, command_buffer_group.size(), command_buffer_group.data());
	m_command_buffers_groups.erase(key);
}


VkCommandPool Vulkan::CommandPool::Get()
{
	return m_command_pool;
}

VkCommandBuffer Vulkan::CommandPool::GetCommandBuffer(const std::string& key, uint32_t index)
{
	return m_command_buffers_groups[key][index];
}
