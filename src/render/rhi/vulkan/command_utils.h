#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
	class Context;
}

namespace Vulkan::CommandUtils {

	void TransitionImageLayout(
		const vk::raii::CommandBuffer& command_buffer, vk::Image image,
		vk::ImageLayout old_layout, vk::ImageLayout new_layout,
		vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
		vk::ImageAspectFlags aspect_flags);

	void TransitionImageLayout(
		const vk::raii::CommandBuffer& command_buffer, vk::Image image,
		vk::ImageLayout old_layout, vk::ImageLayout new_layout);

	vk::raii::CommandBuffer BeginSingleCommandBuffer(const Context &context, vk::CommandPool command_pool);
	void EndSingleCommandBuffer(vk::Queue queue, vk::raii::CommandBuffer&& command_buffer);

};