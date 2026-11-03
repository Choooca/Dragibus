#pragma once 

#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {

	class Renderer;

	class FrameResources {

	public:

		FrameResources(Renderer *renderer, vk::ImageView texture_image_view, vk::Sampler sampler);
		~FrameResources();

		static constexpr uint32_t FRAME_IN_FLIGHT = 3;

		vk::Fence GetInFlightFence(int frame);
		vk::Semaphore GetImageAvailableSemaphore(int frame);
		vk::raii::CommandBuffer& GetCommandBuffer(int frame);
		void* GetUniformBufferMappedMemory(int frame);

		vk::DescriptorSet GetDescriptorSet(int frame) const;

	private:


		Renderer* _renderer;

		std::vector<vk::raii::CommandBuffer> _in_flight_command_buffer;
		std::vector<vk::raii::Fence> _in_flight_fences;
		std::vector<vk::raii::Semaphore> _image_available_semaphore;

		std::vector<vk::raii::Buffer> _uniform_buffers;
		std::vector<vk::raii::DeviceMemory> _uniform_buffers_memory;
		std::vector<void*> _uniform_buffers_mapped;

		vk::raii::DescriptorPool _descriptor_pool = nullptr;

		std::vector<vk::raii::DescriptorSet> _descriptor_sets;
	};

}