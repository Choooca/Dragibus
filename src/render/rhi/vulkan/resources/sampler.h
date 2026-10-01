#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;

	class Sampler {

	public:

		Sampler(Renderer* renderer);
		~Sampler();

		VkSampler Get();

	private:

		VkSampler _sampler;
		Renderer* _renderer;

	};

}