#pragma once

#include <vulkan/vulkan.hpp>


namespace Vulkan {

	class Renderer;

	class ImageView {

	public:
		ImageView(Renderer *renderer, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspect_flags);
		~ImageView();

		ImageView(const ImageView& other) = delete;
		ImageView& operator=(const ImageView& other) = delete;

		ImageView(ImageView&& other) = default;
		ImageView& operator=(ImageView&& other) = default;

		VkImageView Get();

	private:

		VkImageView m_image_view;
		Renderer* m_renderer;
	};

}