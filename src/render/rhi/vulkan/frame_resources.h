#pragma once 

#include <vector>
#include <memory>
#include <string>

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	class Renderer;
	class UniformBuffer;
	class DescriptorPool;
	class CommandPool;
	class Fence;
	class Semaphore;
	struct QueueFamilyIndices;

	class FrameResources {

	public:

		FrameResources(Renderer *renderer, const VkImageView& texture_image_view, const VkSampler& sampler);
		~FrameResources();

		const int FRAME_IN_FLIGHT = 3;

		VkFence GetInFlightFence(int frame);
		VkSemaphore GetImageAvailableSemaphore(int frame);
		VkCommandBuffer GetCommandBuffer(int frame);
		UniformBuffer *GetUniformBuffer(int frame);

		VkDescriptorSet GetDescriptorSet(int frame);

	private:

		const std::string command_buffer_key = "in_flight";

		Renderer* m_renderer;

		std::unique_ptr<DescriptorPool> m_descriptor_pool;
		std::vector<UniformBuffer> m_uniform_buffers;
		std::vector<Fence> m_in_flight_fences;
		std::vector<Semaphore> m_image_available_semaphore;
	};

}