#include "render_loop.h"

#include <array>

#include <glfw/glfw3.h>

#include <render/vk_context.h>
#include <render/swap_chain.h>
#include <render/renderer.h>
#include <utils/debug_macro.h>
#include <render/primitives.h>

namespace {

	void RecordCommandBuffer(const VkContext &vk_context, const SwapChain &swap_chain, const Renderer &renderer, const VkCommandBuffer &command_buffer, uint32_t current_frame) {

		VkCommandBufferBeginInfo command_begin_info{};
		command_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_begin_info.flags = 0;
		command_begin_info.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(command_buffer, &command_begin_info) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to begin command buffer.");
		}

		std::array<VkClearValue, 1> clear_values{};
		clear_values[0].color = { 0, 0, 0 };

		VkRenderPassBeginInfo render_begin_info{};
		render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_begin_info.framebuffer = swap_chain.m_framebuffers[current_frame];
		render_begin_info.renderPass = renderer.m_render_pass;
		render_begin_info.renderArea.extent = swap_chain.m_swap_chain_extent;
		render_begin_info.renderArea.offset = { 0, 0 };
		render_begin_info.clearValueCount = 1;
		render_begin_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffer, &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.m_graphics_pipeline);

		VkBuffer vertex_buffers[] = {renderer.m_vertex_buffer};
		VkDeviceSize offset = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, &offset);
		vkCmdBindIndexBuffer(command_buffer, renderer.m_indice_buffer, 0, VK_INDEX_TYPE_UINT16);

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = swap_chain.m_swap_chain_extent.width;
		viewport.height = swap_chain.m_swap_chain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissors{};
		scissors.extent = swap_chain.m_swap_chain_extent;
		scissors.offset = { 0, 0 };
		vkCmdSetScissor(command_buffer, 0, 1, &scissors);

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.m_pipeline_layout, 0, 1, &renderer.m_descriptor_sets[current_frame], 0, nullptr);
		vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);
		vkCmdEndRenderPass(command_buffer);

		vkEndCommandBuffer(command_buffer);
	}

	void UpdateUniformBuffer(const Renderer &renderer, uint32_t current_frame) {
		UniformBufferObject ubo{};

		memcpy(renderer.m_uniform_buffers_mapped_memory[current_frame], &ubo, sizeof(UniformBufferObject));
	}
}

void DrawFrame(const VkContext& vk_context, SwapChain& swap_chain, Renderer& renderer)
{
	vkWaitForFences(vk_context.m_device, 1, &renderer.m_in_flight_fence[renderer.m_current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(vk_context.m_device, swap_chain.m_swap_chain, UINT64_MAX, renderer.m_image_available_semaphore[renderer.m_current_frame], VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain(vk_context, swap_chain, renderer.m_render_pass);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		THROW_RUNTIME_ERROR("Failed to acquire swap chain image");
	}

	vkResetFences(vk_context.m_device, 1, &renderer.m_in_flight_fence[renderer.m_current_frame]);

	vkResetCommandBuffer(renderer.m_graphics_command_buffer[renderer.m_current_frame], 0);
	RecordCommandBuffer(vk_context, swap_chain, renderer, renderer.m_graphics_command_buffer[renderer.m_current_frame], image_index);

	UpdateUniformBuffer(renderer, renderer.m_current_frame);

	VkSemaphore wait_semaphores[] = { renderer.m_image_available_semaphore[renderer.m_current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signal_semaphore[] = { swap_chain.m_render_finish_semaphore[image_index] };

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &renderer.m_graphics_command_buffer[renderer.m_current_frame];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphore;

	if (vkQueueSubmit(vk_context.m_graphics_queue, 1, &submit_info, renderer.m_in_flight_fence[renderer.m_current_frame]) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to submit queue");
	}

	VkSwapchainKHR swap_chains[] = { swap_chain.m_swap_chain };

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(vk_context.m_present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || swap_chain.m_frame_buffer_resized) {
		swap_chain.m_frame_buffer_resized = false;
		RecreateSwapChain(vk_context, swap_chain, renderer.m_render_pass);
	}
	else if (result != VK_SUCCESS){
		THROW_RUNTIME_ERROR("Failed to present swap chain image");
	}

	renderer.m_current_frame = (renderer.m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
