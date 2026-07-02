#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;

	class DescriptorSetLayout {

	public:

		DescriptorSetLayout(Renderer *renderer);
		~DescriptorSetLayout();

		VkDescriptorSetLayout Get();

	private:

		VkDescriptorSetLayout m_descriptor_set_layout;
		Renderer *m_renderer;

	};


}