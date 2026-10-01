#include "uniform_buffer.h"

#include <render/rhi/vulkan/resources/buffer.h>
#include <render/rhi/vulkan/resources/device_memory.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::UniformBuffer::UniformBuffer(Renderer* renderer, const VkDeviceSize& size)
	: _renderer(renderer)
{
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	_buffer = std::make_unique<Buffer>(_renderer, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, properties);
	
	VkMemoryRequirements mem_requirements{};
	vkGetBufferMemoryRequirements(_renderer->GetDevice(), _buffer->Get(), &mem_requirements);
	_device_memory = std::make_unique<DeviceMemory>(_renderer, mem_requirements, properties);
	vkBindBufferMemory(_renderer->GetDevice(), _buffer->Get(), _device_memory->Get(), 0);
	vkMapMemory(_renderer->GetDevice(), _device_memory->Get(), 0, size, 0, &_mapped_memory);
}

Vulkan::UniformBuffer::~UniformBuffer(){
	vkUnmapMemory(_renderer->GetDevice(), _device_memory->Get());
}

Vulkan::UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept = default;

Vulkan::UniformBuffer& Vulkan::UniformBuffer::operator=(UniformBuffer&& other) noexcept = default;

VkBuffer Vulkan::UniformBuffer::GetBuffer() const
{
	return _buffer->Get();
}

void* Vulkan::UniformBuffer::GetMappedMemory()
{
	return _mapped_memory;
}
