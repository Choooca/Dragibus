#include "vk_resources.h"

#include <sstream>

#include <utils/debug_macro.h>
#include <render/renderer.h>

#pragma region Synchronisation

std::vector<VkSemaphore> CreateSemaphores(const VkContext& context, uint32_t count, VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.flags = flags;

	std::vector<VkSemaphore> out(count);

	for (size_t i = 0; i < count; ++i) {
		if (vkCreateSemaphore(context.m_device, &semaphore_info, nullptr, &out[i]) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Semaphore.");
		}
	}

	return out;
}

void DestroySemaphores(const VkContext& vk_context, std::vector<VkSemaphore> &semaphores)
{
	for (size_t i = 0; i < semaphores.size(); ++i) {
		vkDestroySemaphore(vk_context.m_device, semaphores[i], nullptr);
	}

	semaphores.clear();
}

std::vector<VkFence> CreateFences(const VkContext& context, uint32_t count, VkFenceCreateFlags flags)
{
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = flags;

	std::vector<VkFence> out(count);
	for (size_t i = 0; i < count; ++i) {
		if (vkCreateFence(context.m_device, &fence_info, nullptr, &out[i]) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Fence.");
		}
	}

	return out;
}

void DestroyFences(const VkContext& vk_context, std::vector<VkFence> &fences)
{
	for (size_t i = 0; i < fences.size(); ++i) {
		vkDestroyFence(vk_context.m_device, fences[i], nullptr);
	}

	fences.clear();
}

#pragma endregion

#pragma region Memory & Buffer

uint32_t FindMemoryType(const VkContext& context, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(context.m_physical_device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find suitable memory type");
}

void CreateBuffer(const VkContext& vk_context, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	uint32_t shared_queue_family_index[] = { vk_context.m_indices.graphics_family.value(), vk_context.m_indices.transfer_family.value() };

	if (shared_queue_family_index[0] != shared_queue_family_index[1]) {
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = 2;
		buffer_info.pQueueFamilyIndices = shared_queue_family_index;
	}
	else {
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateBuffer(vk_context.m_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create buffer");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(vk_context.m_device, buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(vk_context, mem_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vk_context.m_device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to allocate buffer memory")
	}

	vkBindBufferMemory(vk_context.m_device, buffer, buffer_memory, 0);
}

void CopyBuffer(const VkContext& vk_context, const VkCommandPool& command_pool, const VkBuffer& src_buffer, const VkBuffer& dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer transfer_command_buffer = BeginSingleCommandBuffer(vk_context, command_pool);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(transfer_command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	EndSingleCommandBuffer(vk_context, transfer_command_buffer, command_pool, vk_context.m_transfer_queue);
}

#pragma endregion

#pragma region Command Buffers

VkCommandPool CreateCommandPool(const VkContext& vk_context, uint32_t queue_family_index, VkCommandPoolCreateFlagBits flags)
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = flags;
	pool_info.queueFamilyIndex = queue_family_index;

	VkCommandPool out;
	if (vkCreateCommandPool(vk_context.m_device, &pool_info, nullptr, &out) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create command pool.");
	}

	return out;
}

std::vector<VkCommandBuffer> CreateCommandBuffer(const VkContext& vk_context, const VkCommandPool& command_pool, uint32_t count)
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandBufferCount = count;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	std::vector<VkCommandBuffer> out(count);
	if (vkAllocateCommandBuffers(vk_context.m_device, &alloc_info, out.data()) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create command buffer");
	}

	return out;
}

VkCommandBuffer BeginSingleCommandBuffer(const VkContext& vk_context, const VkCommandPool& command_pool)
{
	VkCommandBuffer out = CreateCommandBuffer(vk_context, command_pool, 1)[0];

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(out, &begin_info);

	return out;
}

void EndSingleCommandBuffer(const VkContext& vk_context, const VkCommandBuffer& command_buffer, const VkCommandPool& command_pool, const VkQueue& queue)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(vk_context.m_device, command_pool, 1, &command_buffer);
}

#pragma endregion

#pragma region Image

void CreateImage(const VkContext &vk_context, uint32_t width, uint32_t height, const VkFormat &format, const VkImageTiling &tilling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory)
{
	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tilling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	if (vkCreateImage(vk_context.m_device, &image_info, nullptr, &image) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Image");
	}

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(vk_context.m_device, image, &mem_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(vk_context, mem_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vk_context.m_device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to allocate image memory");
	}

	vkBindImageMemory(vk_context.m_device, image, image_memory, 0);
}

void TransitionImageLayout(const VkContext vk_context, const Renderer &renderer, VkImage& image, const VkFormat& format, const VkImageLayout& old_layout, const VkImageLayout& new_layout)
{
	VkCommandBuffer command_buffer = BeginSingleCommandBuffer(vk_context, renderer.m_graphics_command_pool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.newLayout = new_layout;
	barrier.oldLayout = old_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	VkPipelineStageFlags src_stage{};
	VkPipelineStageFlags dst_stage{};

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL  && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} 
	else {
		std::stringstream s_msg;
		s_msg << "old_layout " << old_layout << " to new layout " << new_layout << " transition is not supported.";
		THROW_INVALID_ARGUMENT(s_msg.str())
	}

	vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	EndSingleCommandBuffer(vk_context, command_buffer, renderer.m_graphics_command_pool, vk_context.m_graphics_queue);
}

#pragma endregion

void CreateTextureImage()
{

}

#pragma endregion