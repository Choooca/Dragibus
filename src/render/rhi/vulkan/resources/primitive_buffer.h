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
			: _renderer(renderer)
		{
			VkDeviceSize buffer_size = sizeof(primitive_data[0]) * primitive_data.size();

			VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			Buffer staging_buffer = Buffer(_renderer, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, properties);

			VkMemoryRequirements staging_mem_requirements{};
			vkGetBufferMemoryRequirements(_renderer->GetDevice(), staging_buffer.Get(), &staging_mem_requirements);
			DeviceMemory staging_device_memory = DeviceMemory(_renderer, staging_mem_requirements, properties);
			vkBindBufferMemory(_renderer->GetDevice(), staging_buffer.Get(), staging_device_memory.Get(), 0);

			void* data;
			vkMapMemory(_renderer->GetDevice(), staging_device_memory.Get(), 0, buffer_size, 0, &data);
			memcpy(data, primitive_data.data(), buffer_size);
			vkUnmapMemory(_renderer->GetDevice(), staging_device_memory.Get());

			_buffer = std::make_unique<Buffer>(_renderer, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | buffer_usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkMemoryRequirements buffer_mem_requirements{};
			vkGetBufferMemoryRequirements(_renderer->GetDevice(), _buffer->Get(), &buffer_mem_requirements);
			_device_memory = std::make_unique<DeviceMemory>(_renderer, buffer_mem_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkBindBufferMemory(_renderer->GetDevice(), _buffer->Get(), _device_memory->Get(), 0);

			_buffer->CopyBuffer(staging_buffer.Get(), buffer_size);
		}

		~PrimitiveBuffer() = default;

		VkBuffer GetBuffer();

	private:

		std::unique_ptr<Buffer> _buffer;
		std::unique_ptr<DeviceMemory> _device_memory;
		Renderer* _renderer;
	};
}