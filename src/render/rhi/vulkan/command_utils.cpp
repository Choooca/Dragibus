#include "command_utils.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

void Vulkan::CommandUtils::TransitionImageLayout(
	const vk::raii::CommandBuffer& command_buffer, vk::Image image,
	vk::ImageLayout old_layout, vk::ImageLayout new_layout,
	vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
	vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
	vk::ImageAspectFlags aspect_flags) 
{
	
	vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
				.aspectMask = aspect_flags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1 } 
	};

	vk::DependencyInfo dependency_info = {
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier };

	command_buffer.pipelineBarrier2(dependency_info);
}

void Vulkan::CommandUtils::TransitionImageLayout(const vk::raii::CommandBuffer& command_buffer, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
	vk::ImageMemoryBarrier2 barrier = {
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		.image = image,
		.subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1 }
	};

	if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;

		barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
	}
	else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;

		barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
	}
	else {
		THROW_INVALID_ARGUMENT("Unsupported image layout transition");
	}

	vk::DependencyInfo dependency_info = {
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier };

	command_buffer.pipelineBarrier2(dependency_info);
}

vk::raii::CommandBuffer Vulkan::CommandUtils::BeginSingleCommandBuffer(const Renderer* renderer, vk::CommandPool command_pool) {
	vk::CommandBufferAllocateInfo command_buffer_alloc_info{
		.commandPool = command_pool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};
	vk::raii::CommandBuffer command_buffer = std::move(vk::raii::CommandBuffers(renderer->GetDevice(), command_buffer_alloc_info).front());

	vk::CommandBufferBeginInfo begin_info{
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
	};

	command_buffer.begin(begin_info);

	return command_buffer;
}

void Vulkan::CommandUtils::EndSingleCommandBuffer(vk::Queue queue, vk::raii::CommandBuffer&& command_buffer) {
	command_buffer.end();

	vk::SubmitInfo submit_info{
		.commandBufferCount = 1,
		.pCommandBuffers = &*command_buffer
	};

	queue.submit(submit_info, nullptr);
	queue.waitIdle();
}
