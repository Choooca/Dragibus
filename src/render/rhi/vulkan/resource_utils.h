#pragma once

#include <utility>
#include <cstring>
#include <string>

#include <vulkan/vulkan_raii.hpp>
#include <render/rhi/vulkan/renderer.h>
#include <utils/debug_macro.h>

namespace Vulkan {

	class PrimitiveBuffer {
	public :
		PrimitiveBuffer(vk::raii::Buffer&& buffer, vk::raii::DeviceMemory&& device_memory);

		vk::Buffer GetBuffer() const;
		vk::DeviceMemory GetDeviceMemory() const;

	private:

		vk::raii::DeviceMemory _device_memory = nullptr;
		vk::raii::Buffer _buffer = nullptr;
	};

	uint32_t FindMemoryType(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties);

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const Renderer* renderer, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

	void CopyBuffer(const Renderer* renderer, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);

	template<typename T>
	PrimitiveBuffer CreatePrimitiveBuffer(const Renderer* renderer, const std::vector<T>& primitive_data, vk::BufferUsageFlags buffer_usage)
	{
		if (primitive_data.empty())
			THROW_INVALID_ARGUMENT("Primitive data can't be empty");

		vk::DeviceSize buffer_size = sizeof(primitive_data[0]) * primitive_data.size();

		auto [staging_buffer, staging_device_memory] = CreateBuffer(renderer, buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

		void* data = staging_device_memory.mapMemory(0, buffer_size);
		memcpy(data, primitive_data.data(), buffer_size);
		staging_device_memory.unmapMemory();

		auto[primitive_buffer, primitive_device_memory] = CreateBuffer(renderer, buffer_size, vk::BufferUsageFlagBits::eTransferDst | buffer_usage, vk::MemoryPropertyFlagBits::eDeviceLocal);

		CopyBuffer(renderer, staging_buffer, primitive_buffer, buffer_size);

		return PrimitiveBuffer(std::move(primitive_buffer), std::move(primitive_device_memory));
	}

	template<typename T> PrimitiveBuffer CreatePrimitiveBuffer(const Renderer* renderer, const T& primitive_data, vk::BufferUsageFlags buffer_usage) {
		std::vector<T> vec = {
			primitive_data
		};

		return CreatePrimitiveBuffer(renderer, vec, buffer_usage);
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const Renderer *renderer, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	vk::raii::ImageView CreateImageView(const Renderer* renderer, vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags);

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateTexture(const Renderer* renderer, const std::string& path, vk::Format format);
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateTexture(const Renderer* renderer, const std::vector<uint8_t>& pixels, uint32_t width, uint32_t height, uint32_t n_channels, vk::Format format);

	void CopyBufferToImage(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image &image, uint32_t width, uint32_t height);

	vk::raii::Sampler CreateSampler(const Renderer *renderer);
}