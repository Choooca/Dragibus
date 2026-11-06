#include "resource_utils.h"
#include <utils/debug_macro.h>
#include <utils/build_macro.h>
#include <render/rhi/vulkan/command_utils.h>
#include <stb_image.h>

uint32_t Vulkan::FindMemoryType(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties mem_properties = physical_device.getMemoryProperties();

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find suitable memory type");
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Vulkan::CreateBuffer(const Context& context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	const QueueSharing& sharing = context.GetQueueSharing();

	vk::BufferCreateInfo buffer_info{
			.size = size,
			.usage = usage,
			.sharingMode = sharing.sharing_mode,
			.queueFamilyIndexCount = static_cast<uint32_t>(sharing.family_indices.size()),
			.pQueueFamilyIndices = sharing.family_indices.data()
	};
	vk::raii::Buffer buffer = vk::raii::Buffer(context.GetDevice(), buffer_info);
	vk::MemoryRequirements buffer_mem_requirement = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo mem_alloc_info = {
		.allocationSize = buffer_mem_requirement.size,
		.memoryTypeIndex = FindMemoryType(context.GetPhysicalDevice(), buffer_mem_requirement.memoryTypeBits, properties)
	};
	vk::raii::DeviceMemory device_memory = vk::raii::DeviceMemory(context.GetDevice(), mem_alloc_info);
	buffer.bindMemory(device_memory, 0);

	return { std::move(buffer), std::move(device_memory) };
}	

void Vulkan::CopyBuffer(const Context& context, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
	vk::raii::CommandBuffer command_buffer = Vulkan::CommandUtils::BeginSingleCommandBuffer(context, context.GetTransferCommandPool());
	command_buffer.copyBuffer(src, dst, vk::BufferCopy{ .srcOffset  = 0, .dstOffset = 0, .size = size});
	Vulkan::CommandUtils::EndSingleCommandBuffer(context.GetTransferQueue(), std::move(command_buffer));
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Vulkan::CreateTexture(const Context& context, const std::string& path, vk::Format format)
{
	int width, height, n_channel;
	stbi_uc* pixels = stbi_load((std::string(TEXTURES_DIR) + path).c_str(), &width, &height, &n_channel, STBI_rgb_alpha);

	if (!pixels) {
		THROW_RUNTIME_ERROR("Failed to load texture at {}", path);
	}

	std::vector<uint8_t> vec_pixel(pixels, pixels + height * width * 4);
	stbi_image_free(pixels);

	return  CreateTexture(context, vec_pixel, width, height, 4, format);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Vulkan::CreateTexture(const Context& context, const std::vector<uint8_t> &pixels, uint32_t width, uint32_t height, uint32_t n_channels, vk::Format format)
{
	vk::DeviceSize image_size = height * width * n_channels;
	auto [staging_buffer, staging_memory] = CreateBuffer(context, image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

	void* data = staging_memory.mapMemory(0, image_size);
	memcpy(data, pixels.data(), image_size);
	staging_memory.unmapMemory();

	auto [image, image_memory] = CreateImage(context, width, height, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::raii::CommandBuffer command_buffer = Vulkan::CommandUtils::BeginSingleCommandBuffer(context, context.GetGraphicsCommandPool());
	Vulkan::CommandUtils::TransitionImageLayout(command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	CopyBufferToImage(command_buffer, staging_buffer, image, width, height);
	Vulkan::CommandUtils::TransitionImageLayout(command_buffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	Vulkan::CommandUtils::EndSingleCommandBuffer(context.GetGraphicsQueue(), std::move(command_buffer));

	return { std::move(image), std::move(image_memory) };
}

vk::raii::ImageView Vulkan::CreateImageView(const Context& context, vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags)
{
	vk::ImageViewCreateInfo image_view_info = {
		.image = image,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
		.subresourceRange = { .aspectMask = aspect_flags, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
	};

	return vk::raii::ImageView(context.GetDevice(), image_view_info);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Vulkan::CreateImage(const Context& context, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	const QueueSharing& sharing = context.GetQueueSharing();

	vk::ImageCreateInfo image_info{
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent = { width, height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = sharing.sharing_mode,
		.queueFamilyIndexCount = static_cast<uint32_t>(sharing.family_indices.size()),
		.pQueueFamilyIndices = sharing.family_indices.data()
	};

	vk::raii::Image image = vk::raii::Image(context.GetDevice(), image_info);

	vk::MemoryRequirements mem_requirement = image.getMemoryRequirements();
	vk::MemoryAllocateInfo alloc_info = {
		.allocationSize = mem_requirement.size,
		.memoryTypeIndex = FindMemoryType(context.GetPhysicalDevice(), mem_requirement.memoryTypeBits, properties)
	};

	vk::raii::DeviceMemory memory = vk::raii::DeviceMemory(context.GetDevice(), alloc_info);
	image.bindMemory(memory, 0);

	return { std::move(image), std::move(memory) };

}

void Vulkan::CopyBufferToImage(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image &image, uint32_t width, uint32_t height)
{
	vk::BufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .layerCount = 1 },
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 },
	};

	command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

vk::raii::Sampler Vulkan::CreateSampler(const Context& context)
{
	vk::PhysicalDeviceProperties properties = context.GetPhysicalDevice().getProperties();

	vk::SamplerCreateInfo sampler_info{ 
		.magFilter = vk::Filter::eLinear,
		.minFilter = vk::Filter::eLinear,
		.mipmapMode = vk::SamplerMipmapMode::eLinear,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.mipLodBias = 0.0f,
		.anisotropyEnable = vk::True,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = vk::False,
		.compareOp = vk::CompareOp::eAlways };

	return vk::raii::Sampler(context.GetDevice(), sampler_info);
}

Vulkan::DeviceLocalBuffer::DeviceLocalBuffer(vk::raii::Buffer&& buffer, vk::raii::DeviceMemory&& device_memory)
	: _device_memory(std::move(device_memory)), _buffer(std::move(buffer))
{}

vk::Buffer Vulkan::DeviceLocalBuffer::GetBuffer() const { return _buffer; }

vk::DeviceMemory Vulkan::DeviceLocalBuffer::GetDeviceMemory() const { return _device_memory; }
