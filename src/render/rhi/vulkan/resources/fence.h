#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;

	class Fence {

	public:

		Fence(Renderer* renderer, const VkFenceCreateFlags& flags);
		~Fence();

		VkFence Get();

	private:

		VkFence m_fence;
		Renderer* m_renderer;
	};

}