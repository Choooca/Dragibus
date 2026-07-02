#include "uniform_buffer.h"

#include <render/rhi/vulkan/resources/buffer.h>
#include <render/rhi/vulkan/resources/device_memory.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::UniformBuffer::UniformBuffer(Renderer* renderer, const VkDeviceSize& size)
	: m_renderer(renderer)
{
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_buffer = std::make_unique<Buffer>(m_renderer, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, properties);
	
	VkMemoryRequirements mem_requirements{};
	vkGetBufferMemoryRequirements(m_renderer->GetDevice(), m_buffer->Get(), &mem_requirements);
	m_device_memory = std::make_unique<DeviceMemory>(m_renderer, mem_requirements, properties);
	vkBindBufferMemory(m_renderer->GetDevice(), m_buffer->Get(), m_device_memory->Get(), 0);
	vkMapMemory(m_renderer->GetDevice(), m_device_memory->Get(), 0, size, 0, &m_mapped_memory);
}

Vulkan::UniformBuffer::~UniformBuffer(){
	vkUnmapMemory(m_renderer->GetDevice(), m_device_memory->Get());
}

Vulkan::UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept = default;

Vulkan::UniformBuffer& Vulkan::UniformBuffer::operator=(UniformBuffer&& other) noexcept = default;

VkBuffer Vulkan::UniformBuffer::GetBuffer() const
{
	return m_buffer->Get();
}

void* Vulkan::UniformBuffer::GetMappedMemory()
{
	return m_mapped_memory;
}
