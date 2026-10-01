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
	: _renderer(renderer)
{
	_renderer->GetGraphicsCommandPool()->CreateCommandBufferGroup(FRAME_IN_FLIGHT, command_buffer_key);

	_uniform_buffers.reserve(FRAME_IN_FLIGHT);
	_in_flight_fences.reserve(FRAME_IN_FLIGHT);
	_image_available_semaphore.reserve(FRAME_IN_FLIGHT);
	for (int i = 0; i < FRAME_IN_FLIGHT; ++i) {
		_uniform_buffers.emplace_back(_renderer, sizeof(UniformBufferObject));
		_image_available_semaphore.emplace_back(_renderer, 0);
		_in_flight_fences.emplace_back(_renderer, VK_FENCE_CREATE_SIGNALED_BIT);
	}

	_descriptor_pool = std::make_unique<DescriptorPool>(_renderer, FRAME_IN_FLIGHT);
	_descriptor_pool->CreateDescriptorSet(_uniform_buffers, texture_image_view, sampler, FRAME_IN_FLIGHT);
}

Vulkan::FrameResources::~FrameResources() {}

VkFence Vulkan::FrameResources::GetInFlightFence(int frame) { return _in_flight_fences[frame].Get(); }

VkSemaphore Vulkan::FrameResources::GetImageAvailableSemaphore(int frame) { return _image_available_semaphore[frame].Get(); }

VkCommandBuffer Vulkan::FrameResources::GetCommandBuffer(int frame) { return _renderer->GetGraphicsCommandPool()->GetCommandBuffer(command_buffer_key, frame); }

Vulkan::UniformBuffer *Vulkan::FrameResources::GetUniformBuffer(int frame) { return &_uniform_buffers[frame]; }

VkDescriptorSet Vulkan::FrameResources::GetDescriptorSet(int frame) { return _descriptor_pool->GetDescriptorSet(frame); }
