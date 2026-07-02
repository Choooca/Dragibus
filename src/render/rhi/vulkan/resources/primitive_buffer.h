#pragma once

#include <vulkan/vulkan.h>

#include <render/rhi/vulkan/resources/buffer.h>
#include <render/rhi/vulkan/resources/device_memory.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/renderer.h>

namespace Vulkan {

	class CommandPool;

	class PrimitiveBuffer {

	public:

		template<typename T>
		PrimitiveBuffer(Renderer *renderer, const std::vector<T> &primitive_data, VkBufferUsageFlags buffer_usage)
			: m_renderer(renderer)
		{
			VkDeviceSize buffer_size = sizeof(primitive_data[0]) * primitive_data.size();

			VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			Buffer staging_buffer = Buffer(m_renderer, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, properties);

			VkMemoryRequirements staging_mem_requirements{};
			vkGetBufferMemoryRequirements(m_renderer->GetDevice(), staging_buffer.Get(), &staging_mem_requirements);
			DeviceMemory staging_device_memory = DeviceMemory(m_renderer, staging_mem_requirements, properties);
			vkBindBufferMemory(m_renderer->GetDevice(), staging_buffer.Get(), staging_device_memory.Get(), 0);

			void* data;
			vkMapMemory(m_renderer->GetDevice(), staging_device_memory.Get(), 0, buffer_size, 0, &data);
			memcpy(data, primitive_data.data(), buffer_size);
			vkUnmapMemory(m_renderer->GetDevice(), staging_device_memory.Get());

			m_buffer = std::make_unique<Buffer>(m_renderer, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | buffer_usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkMemoryRequirements buffer_mem_requirements{};
			vkGetBufferMemoryRequirements(m_renderer->GetDevice(), m_buffer->Get(), &buffer_mem_requirements);
			m_device_memory = std::make_unique<DeviceMemory>(m_renderer, buffer_mem_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkBindBufferMemory(m_renderer->GetDevice(), m_buffer->Get(), m_device_memory->Get(), 0);

			m_buffer->CopyBuffer(staging_buffer.Get(), buffer_size);
		}

		~PrimitiveBuffer() = default;

		VkBuffer GetBuffer();

	private:

		std::unique_ptr<Buffer> m_buffer;
		std::unique_ptr<DeviceMemory> m_device_memory;
		Renderer* m_renderer;
	};
}