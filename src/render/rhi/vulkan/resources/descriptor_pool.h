#pragma once

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	class Renderer;
	class UniformBuffer;

	class DescriptorPool {

	public:

		DescriptorPool(Renderer *renderer, const int descriptor_count);
		~DescriptorPool();

		VkDescriptorPool Get();

		void CreateDescriptorSet(const std::vector<UniformBuffer>& uniform_buffers, const VkImageView& image_view, const VkSampler& sampler, int32_t count);

		VkDescriptorSet GetDescriptorSet(int index);

	private:

		VkDescriptorPool m_descriptor_pool;
		Renderer* m_renderer;

		std::vector<VkDescriptorSet> m_descriptor_sets;

	};

}