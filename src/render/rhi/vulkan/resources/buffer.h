#pragma once

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	class Renderer;

	class Buffer {

	public:

		Buffer(Renderer *renderer, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);
		~Buffer();

		VkBuffer Get();

		void CopyBuffer(const VkBuffer& src_buffer, VkDeviceSize size);

	private:

		VkBuffer m_buffer;
		Renderer* m_renderer;
	};
}