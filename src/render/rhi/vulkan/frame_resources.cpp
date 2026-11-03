#include "frame_resources.h"

#include <render/primitives.h>
#include <render/rhi/vulkan/resource_utils.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::FrameResources::FrameResources(Renderer* renderer, vk::ImageView texture_image_view, vk::Sampler sampler)
	: _renderer(renderer)
{
	vk::CommandBufferAllocateInfo alloc_info{ .commandPool = renderer->GetGraphicsCommandPool(), .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = FRAME_IN_FLIGHT };
	_in_flight_command_buffer = vk::raii::CommandBuffers(_renderer->GetDevice(), alloc_info);

	std::array<vk::DescriptorPoolSize, 2> descriptor_pool_sizes = {
		{
			{ .type = vk::DescriptorType::eUniformBuffer, .descriptorCount = FRAME_IN_FLIGHT },
			{ .type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = FRAME_IN_FLIGHT }
		}
	};

	vk::DescriptorPoolCreateInfo descriptor_pool_info = {
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = FRAME_IN_FLIGHT,
		.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size()),
		.pPoolSizes = descriptor_pool_sizes.data()
	};

	_descriptor_pool = vk::raii::DescriptorPool(renderer->GetDevice(), descriptor_pool_info);

	std::vector<vk::DescriptorSetLayout> layouts(FRAME_IN_FLIGHT, renderer->GetDescriptorSetLayout());
	vk::DescriptorSetAllocateInfo descriptor_set_alloc_info{
		.descriptorPool = _descriptor_pool,
		.descriptorSetCount = static_cast<uint32_t>(FRAME_IN_FLIGHT),
		.pSetLayouts = layouts.data()
	};

	_descriptor_sets = renderer->GetDevice().allocateDescriptorSets(descriptor_set_alloc_info);

	_uniform_buffers.reserve(FRAME_IN_FLIGHT);
	_uniform_buffers_memory.reserve(FRAME_IN_FLIGHT);
	_uniform_buffers_mapped.reserve(FRAME_IN_FLIGHT);
	_in_flight_fences.reserve(FRAME_IN_FLIGHT);
	_image_available_semaphore.reserve(FRAME_IN_FLIGHT);

	for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {

		vk::DeviceSize size = sizeof(Render::UniformBufferObject);
		auto [buffer, memory] = CreateBuffer(renderer, size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		_uniform_buffers.emplace_back(std::move(buffer));
		_uniform_buffers_memory.emplace_back(std::move(memory));
		_uniform_buffers_mapped.emplace_back(_uniform_buffers_memory[i].mapMemory(0, size));

		vk::DescriptorBufferInfo buffer_info = { .buffer = _uniform_buffers[i], .offset = 0, .range = sizeof(Render::UniformBufferObject) };
		vk::DescriptorImageInfo image_info = { .sampler = sampler, .imageView = texture_image_view, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

		std::array<vk::WriteDescriptorSet, 2> descriptor_writes = {
			{
				{
					.dstSet = _descriptor_sets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pBufferInfo = &buffer_info
				},
				{
					.dstSet = _descriptor_sets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &image_info
				}
			}
		};

		renderer->GetDevice().updateDescriptorSets(descriptor_writes, {});

		_image_available_semaphore.emplace_back(renderer->GetDevice(), vk::SemaphoreCreateInfo());
		_in_flight_fences.emplace_back(renderer->GetDevice(), vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
	}
}

Vulkan::FrameResources::~FrameResources() {}

vk::Fence Vulkan::FrameResources::GetInFlightFence(int frame) { return _in_flight_fences[frame]; }

vk::Semaphore Vulkan::FrameResources::GetImageAvailableSemaphore(int frame) { return _image_available_semaphore[frame]; }

vk::raii::CommandBuffer& Vulkan::FrameResources::GetCommandBuffer(int frame) { return _in_flight_command_buffer[frame]; }

void* Vulkan::FrameResources::GetUniformBufferMappedMemory(int frame) { return _uniform_buffers_mapped[frame]; }

vk::DescriptorSet Vulkan::FrameResources::GetDescriptorSet(int frame) const { return _descriptor_sets[frame]; }
