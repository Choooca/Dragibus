#include "render_loop.h"

#include <array>
#include <chrono>

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/rhi/vulkan/renderer.h>
#include <utils/debug_macro.h>
#include <render/primitives.h>

namespace {

	void RecordCommandBuffer(const VkContext &vk_context, const SwapChain &swap_chain, const Renderer &renderer, const VkCommandBuffer &command_buffer, uint32_t current_frame, uint32_t image_index) {

		//VkCommandBufferBeginInfo command_begin_info{};
		//command_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//command_begin_info.flags = 0;
		//command_begin_info.pInheritanceInfo = nullptr;

		//if (vkBeginCommandBuffer(command_buffer, &command_begin_info) != VK_SUCCESS) {
		//	THROW_RUNTIME_ERROR("Failed to begin command buffer.");
		//}

		//std::array<VkClearValue, 2> clear_values{};
		//clear_values[0].color = { 0, 0, 0 };
		//clear_values[1].depthStencil = { 1.0f, 0 };

		//VkRenderPassBeginInfo render_begin_info{};
		//render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		//render_begin_info.framebuffer = swap_chain.m_framebuffers[image_index];
		//render_begin_info.renderPass = renderer.m_render_pass;
		//render_begin_info.renderArea.extent = swap_chain.m_swap_chain_extent;
		//render_begin_info.renderArea.offset = { 0, 0 };
		//render_begin_info.clearValueCount = clear_values.size();
		//render_begin_info.pClearValues = clear_values.data();

		//vkCmdBeginRenderPass(command_buffer, &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		//vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.m_graphics_pipeline);

		//VkBuffer vertex_buffers[] = {renderer.m_vertex_buffer};
		//VkDeviceSize offset = { 0 };
		//vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, &offset);
		//vkCmdBindIndexBuffer(command_buffer, renderer.m_indice_buffer, 0, VK_INDEX_TYPE_UINT16);

		//VkViewport viewport{};
		//viewport.x = 0;
		//viewport.y = 0;
		//viewport.width = swap_chain.m_swap_chain_extent.width;
		//viewport.height = swap_chain.m_swap_chain_extent.height;
		//viewport.minDepth = 0.0f;
		//viewport.maxDepth = 1.0f;
		//vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		//VkRect2D scissors{};
		//scissors.extent = swap_chain.m_swap_chain_extent;
		//scissors.offset = { 0, 0 };
		//vkCmdSetScissor(command_buffer, 0, 1, &scissors);

		//vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.m_pipeline_layout, 0, 1, &renderer.m_descriptor_sets[current_frame], 0, nullptr);
		//vkCmdDrawIndexed(command_buffer, 12, 1, 0, 0, 0);
		//vkCmdEndRenderPass(command_buffer);

		//vkEndCommandBuffer(command_buffer);
	}

	void UpdateUniformBuffer(const Renderer &renderer, const SwapChain &swap_chain, uint32_t current_frame) {
		//std::chrono::high_resolution_clock timer;

		//static std::chrono::steady_clock::time_point start = timer.now();
		//std::chrono::steady_clock::time_point now = timer.now();

		//float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();

		//UniformBufferObject ubo{};
		//ubo.model = glm::rotate(glm::mat4(1.0f), delta_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.perspective = glm::perspective(glm::radians(45.0f), swap_chain.m_swap_chain_extent.width / (float)swap_chain.m_swap_chain_extent.height, 0.1f, 10.0f);
		//ubo.perspective[1][1] *= -1;

		//memcpy(renderer.m_uniform_buffers_mapped_memory[current_frame], &ubo, sizeof(UniformBufferObject));
	}
}

void DrawFrame(const VkContext& vk_context, SwapChain& swap_chain, Renderer& renderer)
{
	
}
