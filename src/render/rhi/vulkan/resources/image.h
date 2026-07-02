#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;
	class CommandPool;

	class Image {

	public:

		Image(Renderer *renderer, uint32_t width, uint32_t height, const VkFormat& format, const VkImageTiling& tilling, VkImageUsageFlags usage);
		~Image();

		VkImage Get();

		void TransitionImageLayout(CommandPool *command_pool, const VkQueue queue, const VkFormat& format, const VkImageLayout& old_layout, const VkImageLayout& new_layout);
		void CopyBufferToImage(const VkBuffer& buffer, uint32_t width, uint32_t height);

	private:

		VkImage m_image;
		Renderer* m_renderer;

		bool HasStencilComponent(const VkFormat& format);


	};

}