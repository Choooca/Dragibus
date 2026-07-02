#pragma once

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	class Renderer;

	class Sampler {

	public:

		Sampler(Renderer* renderer);
		~Sampler();

		VkSampler Get();

	private:

		VkSampler m_sampler;
		Renderer* m_renderer;

	};

}