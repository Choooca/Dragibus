#include "frame_resources.h"

#include <render/primitives.h>
#include <render/rhi/vulkan/resource_utils.h>
#include <render/rhi/vulkan/context.h>
#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/vulkan/model.h>
#include <render/rhi/vulkan/descriptor_layouts.h>

Vulkan::FrameResources::FrameResources(const Context& context, const Renderer& renderer)
{
	vk::CommandBufferAllocateInfo alloc_info{ .commandPool = context.GetGraphicsCommandPool(), .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = FRAME_IN_FLIGHT };
	_in_flight_command_buffer = vk::raii::CommandBuffers(context.GetDevice(), alloc_info);
	
	_uniform_buffers.reserve(FRAME_IN_FLIGHT);
	_uniform_buffers_memory.reserve(FRAME_IN_FLIGHT);
	_uniform_buffers_mapped.reserve(FRAME_IN_FLIGHT);
	_in_flight_fences.reserve(FRAME_IN_FLIGHT);
	_image_available_semaphore.reserve(FRAME_IN_FLIGHT);

	std::vector<std::vector<DescriptorBindingResource>> resources;
	resources.reserve(FRAME_IN_FLIGHT);

	for (uint32_t i = 0; i < FRAME_IN_FLIGHT; ++i) {

		vk::DeviceSize size = sizeof(Render::GlobalTransformation);
		auto [buffer, memory] = CreateBuffer(context, size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		_uniform_buffers.emplace_back(std::move(buffer));
		_uniform_buffers_memory.emplace_back(std::move(memory));
		_uniform_buffers_mapped.emplace_back(_uniform_buffers_memory[i].mapMemory(0, size));

		_image_available_semaphore.emplace_back(context.GetDevice(), vk::SemaphoreCreateInfo());
		_in_flight_fences.emplace_back(context.GetDevice(), vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });

		resources.push_back({ 
			std::vector{ 
				vk::DescriptorBufferInfo{ .buffer = _uniform_buffers[i], .offset = 0, .range = sizeof(Render::GlobalTransformation) } 
			}
		});
	}

	_obj_descriptor_sets = CreateDescriptorSetGroup(
		context, 
		{
			.descriptor_set_layout = renderer.GetObjDescriptorSetLayout(),
			.bindings = OBJ_BINDINGS,
			.resources = std::move(resources)
		}
	);
}

Vulkan::FrameResources::~FrameResources() {}

vk::Fence Vulkan::FrameResources::GetInFlightFence(int frame) { return _in_flight_fences[frame]; }

vk::Semaphore Vulkan::FrameResources::GetImageAvailableSemaphore(int frame) { return _image_available_semaphore[frame]; }

vk::raii::CommandBuffer& Vulkan::FrameResources::GetCommandBuffer(int frame) { return _in_flight_command_buffer[frame]; }

void* Vulkan::FrameResources::GetUniformBufferMappedMemory(int frame) { return _uniform_buffers_mapped[frame]; }

vk::DescriptorSet Vulkan::FrameResources::GetDescriptorSet(int frame) const { return _obj_descriptor_sets.sets[frame]; }
