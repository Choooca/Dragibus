#include "buffer.h"

#include <utils/debug_macro.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/resources/command_pool.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Buffer::Buffer(Renderer* renderer, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties)
	: m_renderer(renderer) {
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	uint32_t shared_queue_family_index[] = { 
		m_renderer->GetQueueFamilyIndices().graphics_family.value(),
		m_renderer->GetQueueFamilyIndices().transfer_family.value()
	};

	if (shared_queue_family_index[0] != shared_queue_family_index[1]) {
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = 2;
		buffer_info.pQueueFamilyIndices = shared_queue_family_index;
	}
	else {
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateBuffer(m_renderer->GetDevice(), &buffer_info, nullptr, &m_buffer) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create buffer");
	}
}

Vulkan::Buffer::~Buffer()
{
	vkDestroyBuffer(m_renderer->GetDevice(), m_buffer, nullptr);
}

VkBuffer Vulkan::Buffer::Get()
{
	return m_buffer;
}

void Vulkan::Buffer::CopyBuffer(const VkBuffer& src_buffer, VkDeviceSize size)
{
	const std::string copy_buffer_group_key = "copy_buffer";
	VkCommandBuffer command_buffer = m_renderer->GetTransferCommandPool()->CreateCommandBufferGroup(1, copy_buffer_group_key)[0];

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;

	vkCmdCopyBuffer(command_buffer, src_buffer, m_buffer, 1, &copy_region);

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(m_renderer->GetTransferQueue(), 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_renderer->GetTransferQueue());

	m_renderer->GetTransferCommandPool()->DestroyCommandBufferGroup(copy_buffer_group_key);
}
