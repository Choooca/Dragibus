#pragma once

#include <vector>

#include <vulkan/vulkan.h>

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

		VkDescriptorPool _descriptor_pool;
		Renderer* _renderer;

		std::vector<VkDescriptorSet> _descriptor_sets;

	};

}