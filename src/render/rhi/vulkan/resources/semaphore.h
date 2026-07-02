#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;

	class Semaphore {

	public:

		Semaphore(Renderer *renderer, const VkSemaphoreCreateFlags flags);
		~Semaphore();

		VkSemaphore Get();

	private:

		VkSemaphore m_semaphore;
		Renderer *m_renderer;

	};

}