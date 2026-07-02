#pragma once

#include <memory>

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;
	class Buffer;
	class DeviceMemory;
	struct QueueFamilyIndices;

	class UniformBuffer {

	public:

		UniformBuffer(Renderer *renderer, const VkDeviceSize& size);
		~UniformBuffer();

		UniformBuffer(const UniformBuffer& other) = delete;
		UniformBuffer& operator=(const UniformBuffer& other) = delete;

		UniformBuffer(UniformBuffer&& other) noexcept;
		UniformBuffer& operator=(UniformBuffer&& other) noexcept;

		VkBuffer GetBuffer() const;
		void* GetMappedMemory();

	private:

		Renderer* m_renderer;

		std::unique_ptr<Buffer> m_buffer;
		std::unique_ptr<DeviceMemory> m_device_memory;
		void* m_mapped_memory;

	};

}
