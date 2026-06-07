#include "vk_render_context.h"

#include <algorithm>
#include <array>

#include <render/vk_context.h>
#include <render/vk_utils.h>
#include <utils/debug_macro.h>
#include <utils/files.h>
#include <utils/build_macro.h>
#include <render/primitives.h>

namespace {

#pragma region Resources

	VkImageView CreateImageView(const VkContext& ctx, const VkImage& image, const VkFormat format, VkImageAspectFlags aspect_flags) {

		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image = image;
		image_view_create_info.format = format;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

		image_view_create_info.subresourceRange.aspectMask = aspect_flags;
		image_view_create_info.subresourceRange.layerCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseMipLevel = 0;

		VkImageView out;
		if (vkCreateImageView(ctx.m_device, &image_view_create_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Image View");
		}

		return out;
	}

#pragma endregion

#pragma region Swap Chain

	VkSurfaceFormatKHR ChooseSwapChainImageFormat(const SwapChainSupportDetails &swap_chain_support_details) {

		const std::vector<VkSurfaceFormatKHR>& available_formats = swap_chain_support_details.formats;

		VkSurfaceFormatKHR image_format = available_formats[0];
		for (size_t i = 1; i < available_formats.size(); ++i) {
			const VkSurfaceFormatKHR& available_format = available_formats[i];
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				image_format = available_format;
				break;
			}
		}

		return image_format;
	}

	VkPresentModeKHR ChooseSwapChainPresentMode(const SwapChainSupportDetails& swap_chain_support_details) {

		const std::vector<VkPresentModeKHR>& available_present_modes = swap_chain_support_details.present_modes;

		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (const VkPresentModeKHR& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				present_mode = available_present_mode;
			}
		}

		return present_mode;
	}

	VkExtent2D GetSwapChainExtent(const SwapChainSupportDetails& swap_chain_support_details, const VkRenderContext &render_ctx) {
		VkExtent2D image_extent;
		const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			image_extent = capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(render_ctx.m_vk_context->m_window, &width, &height);

			image_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			image_extent.width = std::clamp(image_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			image_extent.height = std::clamp(image_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		return image_extent;
	}

	VkSwapchainKHR CreateSwapChain(const VkRenderContext &render_ctx, const SwapChainSupportDetails& swap_chain_support_details) {

		const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

		uint32_t image_count = capabilities.minImageCount;
		if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < image_count) {
			image_count = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swap_chain_info{};
		swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_info.surface = render_ctx.m_vk_context->m_surface;
		swap_chain_info.minImageCount = image_count;
		swap_chain_info.imageFormat = render_ctx.m_swap_chain_image_format.format;
		swap_chain_info.imageColorSpace = render_ctx.m_swap_chain_image_format.colorSpace;
		swap_chain_info.imageExtent = render_ctx.m_swap_chain_extent;
		swap_chain_info.imageArrayLayers = 1;
		swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swap_chain_info.presentMode = render_ctx.m_swap_chain_present_mode;

		uint32_t queue_family_indices[] = { render_ctx.m_vk_context->m_indices.graphics_family.value(), render_ctx.m_vk_context->m_indices.present_family.value()};

		if (render_ctx.m_vk_context->m_indices.graphics_family.value() != render_ctx.m_vk_context->m_indices.present_family.value()) {
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swap_chain_info.queueFamilyIndexCount = 2;
			swap_chain_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swap_chain_info.preTransform = capabilities.currentTransform;
		swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swap_chain_info.clipped = VK_TRUE;

		VkSwapchainKHR out;
		if (vkCreateSwapchainKHR(render_ctx.m_vk_context->m_device, &swap_chain_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create swap chain");
		}

		return out;
	}

	std::vector<VkImage> RetrieveSwapChainImage(const VkContext &ctx, const VkRenderContext &render_ctx) {
		
		uint32_t swap_chain_image_count; 
		vkGetSwapchainImagesKHR(ctx.m_device, render_ctx.m_swap_chain, &swap_chain_image_count, nullptr);

		std::vector<VkImage> swap_chain_images(swap_chain_image_count);
		vkGetSwapchainImagesKHR(ctx.m_device, render_ctx.m_swap_chain, &swap_chain_image_count, swap_chain_images.data());

		return swap_chain_images;

	}

	std::vector<VkImageView> CreateSwapChainImageViews(const VkRenderContext &render_context) {

		size_t swap_chain_image_count = render_context.m_swap_chain_images.size();
		std::vector<VkImageView> swap_chain_image_views(swap_chain_image_count);

		for (size_t i = 0; i < swap_chain_image_count; ++i) {
			swap_chain_image_views[i] = CreateImageView(*render_context.m_vk_context, render_context.m_swap_chain_images[i], render_context.m_swap_chain_image_format.format, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		return swap_chain_image_views;
	}

#pragma endregion

#pragma region Render Pass 

	VkRenderPass CreateRenderPass(const VkRenderContext &render_context) {

		VkAttachmentDescription	color_attachment{};
		color_attachment.format = render_context.m_swap_chain_image_format.format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 1> attachments = { color_attachment };
		VkRenderPassCreateInfo render_pass_create_info{};
		render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_create_info.pAttachments = attachments.data();
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = &subpass;
		render_pass_create_info.dependencyCount = 1;
		render_pass_create_info.pDependencies = &dependency;

		VkRenderPass render_pass;
		if (vkCreateRenderPass(render_context.m_vk_context->m_device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Render Pass");
		}

		return render_pass;
	}

#pragma endregion 

#pragma region Graphics Pipeline

	VkShaderModule CreateShaderModule(const VkRenderContext &render_context, const std::string& shader_file_name) {

		std::vector<char> shader_code = ReadFile(std::string(SHADERS_DIR) + shader_file_name);

		VkShaderModuleCreateInfo module_info{};
		module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_info.codeSize = shader_code.size();
		module_info.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());

		VkShaderModule out;
		if (vkCreateShaderModule(render_context.m_vk_context->m_device, &module_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create shader module " + shader_file_name);
		}

		return out;
	}

	VkPipelineLayout CreatePipelineLayout(const VkRenderContext &render_ctx) {
		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &render_ctx.m_descriptor_set_layout;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		VkPipelineLayout pipeline_layout;
		if (vkCreatePipelineLayout(render_ctx.m_vk_context->m_device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Pipeline Layout");
		}

		return pipeline_layout;
	}

	VkPipeline CreateGraphicsPipeline(const VkRenderContext &render_ctx, const std::string &vert_shader_name, const std::string &frag_shader_name) {

		VkShaderModule vert_module = CreateShaderModule(render_ctx, vert_shader_name);
		VkShaderModule frag_module = CreateShaderModule(render_ctx, frag_shader_name);

		VkPipelineShaderStageCreateInfo vert_shader_info{};
		vert_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_info.module = vert_module;
		vert_shader_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_info{};
		frag_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_info.module = frag_module;
		frag_shader_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stage_infos[] = { vert_shader_info, frag_shader_info };

		const std::vector<VkDynamicState> dynamic_state = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_info{};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = dynamic_state.size();
		dynamic_state_info.pDynamicStates = dynamic_state.data();
	
		VkVertexInputBindingDescription binding_description = Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 2> attributes_descriptions = Vertex::GetVertexInputAttributeDescription();

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = attributes_descriptions.data();
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(render_ctx.m_swap_chain_extent.width);
		viewport.height = static_cast<float>(render_ctx.m_swap_chain_extent.height);
		viewport.maxDepth = 1.0f;
		viewport.minDepth = 0.0f;

		VkRect2D scissors{};
		scissors.offset = { 0, 0};
		scissors.extent = render_ctx.m_swap_chain_extent;

		VkPipelineViewportStateCreateInfo viewport_info{};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.scissorCount = 1;
		viewport_info.pScissors = &scissors;
		viewport_info.viewportCount = 1;
		viewport_info.pViewports = &viewport;

		VkPipelineRasterizationStateCreateInfo rasterization_info{};
		rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_info.depthClampEnable = VK_FALSE;
		rasterization_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_info.lineWidth = 1.0f;
		rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization_info.depthBiasEnable = VK_FALSE;
		rasterization_info.depthBiasConstantFactor = 0.0f;
		rasterization_info.depthBiasClamp = 0.0f;
		rasterization_info.depthBiasSlopeFactor = 0.0f;
		
		VkPipelineMultisampleStateCreateInfo multisampling_info{};
		multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_info.sampleShadingEnable = VK_FALSE;
		multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling_info.minSampleShading = 1.0f;
		multisampling_info.pSampleMask = nullptr;
		multisampling_info.alphaToCoverageEnable = VK_FALSE;
		multisampling_info.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending_info{};
		color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending_info.logicOpEnable = VK_FALSE;
		color_blending_info.attachmentCount = 1;
		color_blending_info.pAttachments = &color_blend_attachment;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
		depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_info.depthTestEnable = VK_FALSE;
		depth_stencil_info.depthWriteEnable = VK_FALSE;
		depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_info.stencilTestEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
		graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphics_pipeline_info.stageCount = 2;
		graphics_pipeline_info.pStages = shader_stage_infos;
		graphics_pipeline_info.pVertexInputState = &vertex_input_info;
		graphics_pipeline_info.pInputAssemblyState = &input_assembly_info;
		graphics_pipeline_info.pViewportState = &viewport_info;
		graphics_pipeline_info.pRasterizationState = &rasterization_info;
		graphics_pipeline_info.pMultisampleState = &multisampling_info;
		graphics_pipeline_info.pDepthStencilState = &depth_stencil_info;
		graphics_pipeline_info.pColorBlendState = &color_blending_info;
		graphics_pipeline_info.pDynamicState = &dynamic_state_info;
		graphics_pipeline_info.layout = render_ctx.m_pipeline_layout;
		graphics_pipeline_info.renderPass = render_ctx.m_render_pass;
		graphics_pipeline_info.subpass = 0;
		graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		graphics_pipeline_info.basePipelineIndex = -1;

		VkPipeline out;
		if (vkCreateGraphicsPipelines(render_ctx.m_vk_context->m_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Pipeline");
		}

		vkDestroyShaderModule(render_ctx.m_vk_context->m_device, vert_module, nullptr);
		vkDestroyShaderModule(render_ctx.m_vk_context->m_device, frag_module, nullptr);

		return out;
	}

#pragma endregion
	
#pragma region Framebuffer

	std::vector<VkFramebuffer> CreateFramebuffer(const VkRenderContext &render_context) {
		
		size_t size = render_context.m_swap_chain_image_views.size();
		std::vector<VkFramebuffer> framebuffers(size);

		for (size_t i = 0; i < size; i++) {
			std::array<VkImageView, 1> attachments{
				render_context.m_swap_chain_image_views[i]
			};

			VkFramebufferCreateInfo framebuffer_info{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = render_context.m_render_pass;
			framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = render_context.m_swap_chain_extent.width;
			framebuffer_info.height = render_context.m_swap_chain_extent.height;
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(render_context.m_vk_context->m_device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
				THROW_RUNTIME_ERROR("Failed to create Framebuffer.");
			}
		}

		return framebuffers;
	}

#pragma endregion

#pragma region Descriptors

	VkDescriptorSetLayout CreateDescriptorSetLayout(const VkRenderContext& render_context) {
		VkDescriptorSetLayoutBinding ubo_layout_binding{};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = { ubo_layout_binding };
		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();

		VkDescriptorSetLayout out;
		if (vkCreateDescriptorSetLayout(render_context.m_vk_context->m_device, &layout_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Descriptor Set Layout");
		}

		return out;
	}

#pragma endregion

#pragma region Destruction

	void DestroySwapChain(const VkRenderContext &render_ctx) {
		
		for (VkFramebuffer framebuffer : render_ctx.m_framebuffer) {
			vkDestroyFramebuffer(render_ctx.m_vk_context->m_device, framebuffer, nullptr);
		}
		
		for (const VkImageView& image_view : render_ctx.m_swap_chain_image_views) {
			vkDestroyImageView(render_ctx.m_vk_context->m_device, image_view, nullptr);
		}

		vkDestroySwapchainKHR(render_ctx.m_vk_context->m_device, render_ctx.m_swap_chain, nullptr);
	}

#pragma endregion

}

#pragma region Render Context

VkRenderContext CreateRenderContext(VkContext &ctx)
{
	VkRenderContext out;
	out.m_vk_context = &ctx;

	SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(out.m_vk_context->m_physical_device, out.m_vk_context->m_surface);

	out.m_swap_chain_image_format = ChooseSwapChainImageFormat(swap_chain_support_details);
	out.m_swap_chain_present_mode = ChooseSwapChainPresentMode(swap_chain_support_details);
	out.m_swap_chain_extent = GetSwapChainExtent(swap_chain_support_details, out);
	out.m_swap_chain = CreateSwapChain(out, swap_chain_support_details);
	out.m_swap_chain_images = RetrieveSwapChainImage(ctx, out);
	out.m_swap_chain_image_views = CreateSwapChainImageViews(out);
	out.m_render_pass = CreateRenderPass(out);
	out.m_descriptor_set_layout = CreateDescriptorSetLayout(out);
	out.m_pipeline_layout = CreatePipelineLayout(out);
	out.m_graphics_pipeline = CreateGraphicsPipeline(out, "simple_shader_vert.spv", "simple_shader_frag.spv");
	out.m_framebuffer = CreateFramebuffer(out);

	return out;
}

void DestroyRenderContext(const VkRenderContext& render_ctx)
{
	vkDestroyDescriptorSetLayout(render_ctx.m_vk_context->m_device, render_ctx.m_descriptor_set_layout, nullptr);
	vkDestroyPipeline(render_ctx.m_vk_context->m_device, render_ctx.m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(render_ctx.m_vk_context->m_device, render_ctx.m_pipeline_layout, nullptr);
	vkDestroyRenderPass(render_ctx.m_vk_context->m_device, render_ctx.m_render_pass, nullptr);
	DestroySwapChain(render_ctx);
}

#pragma endregion