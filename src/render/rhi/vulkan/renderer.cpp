#include "renderer.h"

#include <ranges>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <utils/files.h>
#include <utils/build_macro.h>
#include <utils/debug_macro.h>
#include <behavior/camera.h>
#include <render/primitives.h>
#include <render/rhi/vulkan/resource_utils.h>
#include <render/rhi/shader_constants.h>
#include <render/rhi/vulkan/command_utils.h>
#include <render/rhi/vulkan/frame_resources.h>
#include <render/rhi/vulkan/swap_chain_resources.h>

 Vulkan::Renderer::Renderer(GLFWwindow* window)
	 : _window(window)
{
	 // ----------------- Validation Layer -----------------

	 std::vector<char const*> required_layers;
	 if (enable_validation_layer) {
		 required_layers.assign(_validation_layers.begin(), _validation_layers.end());
	 }
	 auto layer_properties = _raii_context.enumerateInstanceLayerProperties();
	 auto unsupported_layer_it = std::ranges::find_if(required_layers,
		 [&layer_properties](auto const& required_layer) {
			 return std::ranges::none_of(layer_properties,
				 [required_layer](auto const& layer_property) {
					 return strcmp(layer_property.layerName, required_layer) == 0; });
				});

	 if (unsupported_layer_it != required_layers.end()) {
		 THROW_RUNTIME_ERROR("Required layer not supported : {}.", std::string(*unsupported_layer_it));
	 }

	// ----------------- Extensions -----------------

	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	_required_extensions = std::vector(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layer) {
		_required_extensions.push_back(vk::EXTDebugUtilsExtensionName);
	}

	std::vector<vk::ExtensionProperties> extension_properties = _raii_context.enumerateInstanceExtensionProperties();
	auto unsupported_extension = std::ranges::find_if(_required_extensions,
		[&extension_properties](auto const& required_extension) {
			return std::ranges::none_of(extension_properties,
				[&required_extension](auto const& extension_property) {
					return strcmp(required_extension, extension_property.extensionName) == 0;
				});
		}
	);
	if (unsupported_extension != _required_extensions.end()) {
		THROW_RUNTIME_ERROR("Asked extensions {} are not supported", *unsupported_extension);
	}

	// ----------------- Instance -----------------

	vk::DebugUtilsMessengerCreateInfoEXT debug_info{
	.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
	.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	.pfnUserCallback = &DebugCallback,
	.pUserData = nullptr
	};

	constexpr vk::ApplicationInfo app_info{
		.pApplicationName = "Dragibus",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Dragibus",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14
	};

	vk::InstanceCreateInfo instance_create_info{
		.pNext = enable_validation_layer ? &debug_info : nullptr,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
		.ppEnabledLayerNames = required_layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(_required_extensions.size()),
		.ppEnabledExtensionNames = _required_extensions.data()
	};

	_instance = vk::raii::Instance(_raii_context, instance_create_info);
	
	// ----------------- Debug -----------------

	if (enable_validation_layer) {
		_debug_messenger = _instance.createDebugUtilsMessengerEXT(debug_info);
	}

	// ----------------- Surface -----------------
	
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (glfwCreateWindowSurface(*_instance, _window, nullptr, &surface) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create window surface");
	}
	_surface = vk::raii::SurfaceKHR(_instance, surface);

	// ----------------- Physical Device -----------------

	std::vector<vk::raii::PhysicalDevice> physical_devices = _instance.enumeratePhysicalDevices();
	if (physical_devices.empty()) {
		THROW_RUNTIME_ERROR("Failed to find any GPUs with Vulkan support");
	}

	for (auto const & physical_device : physical_devices) {

		QueueFamilyIndices queue_family_indices = {};

		if(physical_device.getProperties().apiVersion < vk::ApiVersion13){
			continue;
		}

		std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

		for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {

			vk::QueueFamilyProperties queue_family_property = queue_family_properties[i];

			if (queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics && physical_device.getSurfaceSupportKHR(i, *_surface) && !queue_family_indices.graphics_family)
				queue_family_indices.graphics_family = i;

			if (queue_family_property.queueFlags & vk::QueueFlagBits::eTransfer && !(queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics) && !queue_family_indices.transfer_family)
				queue_family_indices.transfer_family = i;
		}

		if (!queue_family_indices.transfer_family.has_value())
			queue_family_indices.transfer_family = queue_family_indices.graphics_family;

		if (!queue_family_indices.IsComplete())
			continue;
		
		std::vector<vk::ExtensionProperties> available_physical_device_extensions = physical_device.enumerateDeviceExtensionProperties();
		bool support_all_extensions_required = std::ranges::all_of(
			_required_device_extensions,
			[&available_physical_device_extensions](auto const& required_device_extension) {
				return std::ranges::any_of(available_physical_device_extensions,
					[required_device_extension](auto const& available_physical_device_extension) {
						return strcmp(available_physical_device_extension.extensionName, required_device_extension) == 0;
					});
			}
		);

		if (!support_all_extensions_required)
			continue;

		auto features = physical_device.getFeatures2<vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features>();

		bool supports_required_features = features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
			features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy;

		if (!supports_required_features)
			continue;

		std::vector<vk::SurfaceFormatKHR> available_formats = physical_device.getSurfaceFormatsKHR(*_surface);
		std::vector<vk::PresentModeKHR> available_present_modes = physical_device.getSurfacePresentModesKHR(*_surface);

		if (available_formats.empty() || available_present_modes.empty())
			continue;

		_available_formats = std::move(available_formats);
		_available_present_modes = std::move(available_present_modes);
		_surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*_surface);
		_physical_device = physical_device;
		_queue_family_indices = queue_family_indices;
		break;
	}

	if (_physical_device == nullptr) {
		THROW_RUNTIME_ERROR("Failed to find a suitable physical device");
	}
	
	// ----------------- Sharing Mode -----------------

	if (_queue_family_indices.graphics_family.value() == _queue_family_indices.transfer_family.value()) {
		_queue_sharing = {
			.sharing_mode = vk::SharingMode::eExclusive
		};
	}
	else {
		_queue_sharing = {
			.sharing_mode = vk::SharingMode::eConcurrent,
			.family_indices = { _queue_family_indices.graphics_family.value(), _queue_family_indices.transfer_family.value() }
		};
	}

	// ----------------- Logical Device -----------------

	float queue_priority = 1.0f;
	std::set<uint32_t> unique_families = {
		_queue_family_indices.graphics_family.value(),
		_queue_family_indices.transfer_family.value(), };
	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
	for (uint32_t family : unique_families)
		queue_create_infos.push_back({ .queueFamilyIndex = family, .queueCount = 1, .pQueuePriorities = &queue_priority });

	vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features>
		feature_chain = {
			{.features = {.samplerAnisotropy = true}},
			{.shaderDrawParameters = true},
			{.synchronization2 = true, .dynamicRendering = true}
	};

	vk::DeviceCreateInfo device_create_info{
		.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
		.pQueueCreateInfos = queue_create_infos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(_required_device_extensions.size()),
		.ppEnabledExtensionNames = _required_device_extensions.data()
	};

	_device = vk::raii::Device(_physical_device, device_create_info);

	// ----------------- Queue -----------------

	_graphics_queue = vk::raii::Queue(_device, _queue_family_indices.graphics_family.value(), 0);
	_transfer_queue = vk::raii::Queue(_device, _queue_family_indices.transfer_family.value(), 0);

	// ----------------- Command Pool -----------------

	vk::CommandPoolCreateInfo graphics_pool_info = {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = _queue_family_indices.graphics_family.value()
	};
	_graphics_command_pool = vk::raii::CommandPool(_device, graphics_pool_info);

	vk::CommandPoolCreateInfo transfer_pool_info = {
		.flags = vk::CommandPoolCreateFlagBits::eTransient,
		.queueFamilyIndex = _queue_family_indices.transfer_family.value()
	};
	_transfer_command_pool = vk::raii::CommandPool(_device, transfer_pool_info);

	// ----------------- Swap Chain -----------------

	auto format_it = std::ranges::find_if(_available_formats,
		[](auto const &available_format) {
			return available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	_surface_format = format_it != _available_formats.end() ? *format_it : _available_formats[0];
	_depth_format = FindDepthFormat(_physical_device);

	_swap_chain_ressources = std::make_unique<SwapChainResources>(this);

	// ----------------- Shaders -----------------

	auto shader_code = ReadFile(std::string(SHADERS_DIR) + "simple_shader.spv");
	vk::ShaderModuleCreateInfo shader_module_create_info{ .codeSize = shader_code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(shader_code.data()) };
	vk::raii::ShaderModule shader_module = vk::raii::ShaderModule(_device, shader_module_create_info);

	vk::PipelineShaderStageCreateInfo vert_shader_stage_info{
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = shader_module,
		.pName = ShaderEntryPoints::VERTEX
	};

	vk::PipelineShaderStageCreateInfo frag_shader_stage_info{
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = shader_module,
		.pName = ShaderEntryPoints::FRAGMENT
	};

	vk::PipelineShaderStageCreateInfo shader_stage_infos[] = {
		vert_shader_stage_info,
		frag_shader_stage_info
	};

	// ----------------- Descriptor Set -----------------
	
	std::array<vk::DescriptorSetLayoutBinding, 2> descriptor_set_layout_bindings{
		{ 
			{ .binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex },
			{ .binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment } 
		}
	};

	vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
		.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size()),
		.pBindings = descriptor_set_layout_bindings.data()
	};
	_descriptor_set_layout = vk::raii::DescriptorSetLayout(_device, descriptor_set_layout_create_info);

	// ----------------- Graphics Pipeline -----------------

	std::vector<vk::DynamicState> dynamic_state = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamic_state_info{ .dynamicStateCount = static_cast<uint32_t>(dynamic_state.size()), .pDynamicStates = dynamic_state.data() };

	auto binding_description = Vertex::GetBindingDescription();
	auto attribute_description = Vertex::GetAttributeDescription();
	vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding_description,
		.vertexAttributeDescriptionCount = attribute_description.size(),
		.pVertexAttributeDescriptions = attribute_description.data()
	};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{ .topology = vk::PrimitiveTopology::eTriangleList };
	vk::PipelineViewportStateCreateInfo viewport_state_info{ .viewportCount = 1, .scissorCount = 1 };
	vk::PipelineRasterizationStateCreateInfo rasterizer_info{ 
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = vk::False,
		.lineWidth = 1.0f };
	vk::PipelineMultisampleStateCreateInfo multisampling_info{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };
	vk::PipelineColorBlendAttachmentState color_blend_attachment_info{
		.blendEnable = vk::False,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
	vk::PipelineColorBlendStateCreateInfo color_blending_info{
		.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &color_blend_attachment_info };
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{
		.depthTestEnable = vk::True,
		.depthWriteEnable = vk::True,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = vk::False,
		.stencilTestEnable = vk::False 
	};
	vk::PipelineLayoutCreateInfo pipeline_layout_info{ .setLayoutCount = 1, .pSetLayouts = &*_descriptor_set_layout , .pushConstantRangeCount = 0 };
	_pipeline_layout = vk::raii::PipelineLayout(_device, pipeline_layout_info);

	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_info_chain = {
	{ .stageCount = 2,
	 .pStages = shader_stage_infos,
	 .pVertexInputState = &vertex_input_info,
	 .pInputAssemblyState = &input_assembly_info,
	 .pViewportState = &viewport_state_info,
	 .pRasterizationState = &rasterizer_info,
	 .pMultisampleState = &multisampling_info,
	 .pDepthStencilState = &depth_stencil_info,
	 .pColorBlendState = &color_blending_info,
	 .pDynamicState = &dynamic_state_info,
	 .layout = _pipeline_layout,
	 .renderPass = nullptr },
	{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &_surface_format.format, .depthAttachmentFormat = _depth_format } };
	_graphics_pipeline = vk::raii::Pipeline(_device, nullptr, pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>());

	// ----------------- Sync -----------------

	_render_finish_semaphore.reserve(_swap_chain_ressources->GetSwapChainImageCount());
	for (int i = 0; i < _swap_chain_ressources->GetSwapChainImageCount(); ++i) {
		_render_finish_semaphore.emplace_back(_device, vk::SemaphoreCreateInfo());
	}

	std::tie(_texture_image, _texture_memory) = CreateTexture(this, "eliasdridi.jpg");
	_texture_view = CreateImageView(this, _texture_image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	_texture_sampler = CreateSampler(this);

	_frame_resources = std::make_unique<FrameResources>(this, _texture_view, _texture_sampler);

	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, FramebufferResizedCallback);

}

Vulkan::Renderer::~Renderer()
{

}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Vulkan::Renderer::DebugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
	vk::DebugUtilsMessageTypeFlagsEXT              type,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		spdlog::error("validation layer: {}", pCallbackData->pMessage);
	else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		spdlog::warn("validation layer: {}", pCallbackData->pMessage);
	else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
		spdlog::debug("validation layer: {}", pCallbackData->pMessage);

	return VK_FALSE;
}

vk::raii::SurfaceKHR &Vulkan::Renderer::GetSurface() { return _surface; }

vk::SurfaceFormatKHR Vulkan::Renderer::GetSurfaceFormat() { return _surface_format; }

std::vector<vk::PresentModeKHR> &Vulkan::Renderer::GetAvailablePresentModes() { return _available_present_modes; }

vk::SurfaceCapabilitiesKHR Vulkan::Renderer::GetSurfaceCapabilities() { return _surface_capabilities; }

vk::Format Vulkan::Renderer::GetDepthFormat() { return _depth_format; }

Vulkan::QueueFamilyIndices Vulkan::Renderer::GetQueueFamilyIndices() const { return _queue_family_indices; }

vk::Queue Vulkan::Renderer::GetGraphicsQueue() const { return _graphics_queue; }

vk::Queue Vulkan::Renderer::GetTransferQueue() const { return _transfer_queue; }

vk::CommandPool Vulkan::Renderer::GetGraphicsCommandPool() const { return _graphics_command_pool; }

vk::CommandPool Vulkan::Renderer::GetTransferCommandPool() const { return _transfer_command_pool; }

vk::Format Vulkan::Renderer::FindSupportedFormat(vk::PhysicalDevice physical_device, const std::vector<vk::Format> candidates, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& features)
{
	for (const auto format : candidates) {
		vk::FormatProperties properties = physical_device.getFormatProperties(format);

		if (((tiling == vk::ImageTiling::eLinear) && ((properties.linearTilingFeatures & features) == features)) ||
			((tiling == vk::ImageTiling::eOptimal) && ((properties.optimalTilingFeatures & features) == features)))
		{
			return format;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find supported format");
}

vk::Format Vulkan::Renderer::FindDepthFormat(vk::PhysicalDevice physical_device)
{
	return FindSupportedFormat(physical_device, { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void Vulkan::Renderer::RecreateSwapChainResources()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	_device.waitIdle();

	_swap_chain_ressources = std::make_unique<SwapChainResources>(this);
}

void Vulkan::Renderer::FramebufferResizedCallback(GLFWwindow* window, int width, int height)
{
	Vulkan::Renderer* renderer = reinterpret_cast<Vulkan::Renderer*>(glfwGetWindowUserPointer(window));
	renderer->_frame_buffer_resized = true;
}

void Vulkan::Renderer::UpdateUniformBuffer(uint32_t current_frame, Camera* camera)
{
	static std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();

	float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();

	Render::UniformBufferObject ubo{};
	ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
	ubo.view = glm::lookAt(camera->GetPosition(), camera->GetPosition() + camera->GetForward(), glm::vec3(0, 1, 0));
	ubo.perspective = glm::perspective(glm::radians(45.0f), _swap_chain_ressources->GetSwapChainExtent().width / (float)_swap_chain_ressources->GetSwapChainExtent().height, 0.001f, 1000.0f);

	memcpy(_frame_resources->GetUniformBufferMappedMemory(current_frame), &ubo, sizeof(Render::UniformBufferObject));
}

void Vulkan::Renderer::RecordCommandBuffer(uint32_t current_frame, uint32_t swap_chain_image_index)
{
	vk::raii::CommandBuffer& command_buffer = _frame_resources->GetCommandBuffer(current_frame);
	command_buffer.begin({});

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetSwapChainImage(swap_chain_image_index),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		{}, vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor);

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetDepthImage(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::ImageAspectFlagBits::eDepth);

	vk::ClearValue clear_color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	vk::ClearValue clear_depth = vk::ClearDepthStencilValue{ 1.0, 0 };
	vk::RenderingAttachmentInfo attachment_info = {
		.imageView = _swap_chain_ressources->GetSwapChainImageView(swap_chain_image_index),
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = clear_color
	};

	vk::RenderingAttachmentInfo depth_attachment_info = {
		.imageView = _swap_chain_ressources->GetDepthImageView(),
		.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.clearValue = clear_depth
	};

	vk::RenderingInfo rendering_info = {
		.renderArea = { .offset = { 0, 0 }, .extent = _swap_chain_ressources->GetSwapChainExtent() },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachment_info,
		.pDepthAttachment = &depth_attachment_info
	};

	command_buffer.beginRendering(rendering_info);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphics_pipeline);

	command_buffer.setViewport(0,
		vk::Viewport(
			0.0f, static_cast<float>(_swap_chain_ressources->GetSwapChainExtent().height),
			static_cast<float>(_swap_chain_ressources->GetSwapChainExtent().width), -static_cast<float>(_swap_chain_ressources->GetSwapChainExtent().height),
			0.0f, 1.0f
		));
	command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swap_chain_ressources->GetSwapChainExtent()));

	vk::DescriptorSet descriptor_set = _frame_resources->GetDescriptorSet(current_frame);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, descriptor_set, nullptr);

	for (const GPUPrimitive& gpu_primitive : _gpu_primitives) {
		vk::Buffer vertex_buffers[] = { gpu_primitive._vertex_buffer->GetBuffer() };
		vk::DeviceSize offset = { 0 };
		command_buffer.bindVertexBuffers(0, vertex_buffers, offset);
		command_buffer.bindIndexBuffer(gpu_primitive._index_buffer->GetBuffer(), 0, vk::IndexType::eUint32);
		command_buffer.drawIndexed(gpu_primitive._index_count, 1, 0, 0, 0);
	}

	command_buffer.endRendering();

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetSwapChainImage(swap_chain_image_index),
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite, {},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe
		, vk::ImageAspectFlagBits::eColor);

	command_buffer.end();
}

void Vulkan::Renderer::Loop(Camera* camera)
{
	vk::Fence in_flight_fence = _frame_resources->GetInFlightFence(_current_frame);
	auto fence_result = _device.waitForFences(in_flight_fence, VK_TRUE, UINT64_MAX);
	if (fence_result != vk::Result::eSuccess) {
		THROW_RUNTIME_ERROR("Failed to wait for fence");
	}

	auto [result, swap_chain_image_index] = _swap_chain_ressources->GetSwapchain().acquireNextImage(UINT64_MAX, _frame_resources->GetImageAvailableSemaphore(_current_frame), nullptr);

	if (result == vk::Result::eErrorOutOfDateKHR) {
		RecreateSwapChainResources();
		return;
	}
	else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		THROW_RUNTIME_ERROR("Failed to acquire swap chain image");
	}

	_device.resetFences(in_flight_fence);

	vk::CommandBuffer command_buffer = _frame_resources->GetCommandBuffer(_current_frame);
	RecordCommandBuffer(_current_frame, swap_chain_image_index);

	UpdateUniformBuffer(_current_frame, camera);

	vk::Semaphore wait_semaphore[] = { _frame_resources->GetImageAvailableSemaphore(_current_frame) };
	vk::PipelineStageFlags wait_stages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore signal_semaphore[] = { _render_finish_semaphore[swap_chain_image_index] };

	const vk::SubmitInfo submit_info{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphore,
		.pWaitDstStageMask = &wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphore
	};

	_graphics_queue.submit(submit_info, in_flight_fence);

	vk::SwapchainKHR swap_chains[] = { _swap_chain_ressources->GetSwapchain() };

	vk::PresentInfoKHR present_info{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphore,
		.swapchainCount = 1,
		.pSwapchains = swap_chains,
		.pImageIndices = &swap_chain_image_index
	};

	result = _graphics_queue.presentKHR(present_info);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _frame_buffer_resized) {
		_frame_buffer_resized = false;
		RecreateSwapChainResources();
	}
	else if (result != vk::Result::eSuccess){
		THROW_RUNTIME_ERROR("Failed to present swap chain image");
	}

	_current_frame = (_current_frame + 1) % _frame_resources->FRAME_IN_FLIGHT;
}

GLFWwindow* Vulkan::Renderer::GetWindow() { return _window; }

vk::PhysicalDevice Vulkan::Renderer::GetPhysicalDevice() const { return _physical_device; }

const Vulkan::QueueSharing& Vulkan::Renderer::GetQueueSharing() const { return _queue_sharing;  }

const vk::raii::Device &Vulkan::Renderer::GetDevice() const { return _device; }


vk::DescriptorSetLayout Vulkan::Renderer::GetDescriptorSetLayout() { return _descriptor_set_layout; }

Vulkan::GPUPrimitive::GPUPrimitive(Renderer* renderer, const Render::Primitive& primitive) {
	_vertex_buffer = std::make_unique<PrimitiveBuffer>(CreatePrimitiveBuffer(renderer, primitive.vertices, vk::BufferUsageFlagBits::eVertexBuffer));
	_index_buffer = std::make_unique<PrimitiveBuffer>(CreatePrimitiveBuffer(renderer, primitive.indices, vk::BufferUsageFlagBits::eIndexBuffer));
	_index_count = primitive.indices.size();
}

void Vulkan::Renderer::AddScene(const std::vector<Render::Mesh>& meshes) {
	for (const Render::Mesh& mesh : meshes) {
		for (const Render::Primitive& primitive : mesh.primitives) {
			_gpu_primitives.emplace_back(this, primitive);
		}
	}
}