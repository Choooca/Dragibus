#include "frame_resources.h"

#include <render/primitives.h>
#include <render/rhi/vulkan/resources/descriptor_pool.h>
#include <render/rhi/vulkan/resources/uniform_buffer.h>
#include <render/rhi/vulkan/resources/command_pool.h>
#include <render/rhi/vulkan/resources/semaphore.h>
#include <render/rhi/vulkan/resources/fence.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::FrameResources::FrameResources(Renderer* renderer, const VkImageView& texture_image_view, const VkSampler& sampler)
	: m_renderer(renderer)
{
	m_renderer->GetGraphicsCommandPool()->CreateCommandBufferGroup(FRAME_IN_FLIGHT, command_buffer_key);

	m_uniform_buffers.reserve(FRAME_IN_FLIGHT);
	m_in_flight_fences.reserve(FRAME_IN_FLIGHT);
	m_image_available_semaphore.reserve(FRAME_IN_FLIGHT);
	for (int i = 0; i < FRAME_IN_FLIGHT; ++i) {
		m_uniform_buffers.emplace_back(m_renderer, sizeof(UniformBufferObject));
		m_image_available_semaphore.emplace_back(m_renderer, 0);
		m_in_flight_fences.emplace_back(m_renderer, VK_FENCE_CREATE_SIGNALED_BIT);
	}

	m_descriptor_pool = std::make_unique<DescriptorPool>(m_renderer, FRAME_IN_FLIGHT);
	m_descriptor_pool->CreateDescriptorSet(m_uniform_buffers, texture_image_view, sampler, FRAME_IN_FLIGHT);
}

Vulkan::FrameResources::~FrameResources() {}

VkFence Vulkan::FrameResources::GetInFlightFence(int frame) { return m_in_flight_fences[frame].Get(); }

VkSemaphore Vulkan::FrameResources::GetImageAvailableSemaphore(int frame) { return m_image_available_semaphore[frame].Get(); }

VkCommandBuffer Vulkan::FrameResources::GetCommandBuffer(int frame) { return m_renderer->GetGraphicsCommandPool()->GetCommandBuffer(command_buffer_key, frame); }

Vulkan::UniformBuffer *Vulkan::FrameResources::GetUniformBuffer(int frame) { return &m_uniform_buffers[frame]; }

VkDescriptorSet Vulkan::FrameResources::GetDescriptorSet(int frame) { return m_descriptor_pool->GetDescriptorSet(frame); }
