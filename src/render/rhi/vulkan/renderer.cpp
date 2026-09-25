#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/primitives.h>
#include <render/rhi/vulkan/frame_resources.h>
#include <render/rhi/vulkan/swap_chain_resources.h>
#include <render/rhi/vulkan/handles/instance.h>
#include <render/rhi/vulkan/handles/debug_messenger.h>
#include <render/rhi/vulkan/handles/surface.h>
#include <render/rhi/vulkan/handles/device.h>
#include <render/rhi/vulkan/handles/render_pass.h>
#include <render/rhi/vulkan/handles/descriptor_set_layout.h>
#include <render/rhi/vulkan/handles/graphics_pipeline.h>
#include <render/rhi/vulkan/resources/primitive_buffer.h>
#include <render/rhi/vulkan/resources/uniform_buffer.h>
#include <render/rhi/vulkan/resources/texture.h>
#include <render/rhi/vulkan/resources/image_view.h>
#include <render/rhi/vulkan/resources/semaphore.h>
#include <render/rhi/vulkan/resources/sampler.h>
#include <render/rhi/vulkan/resources/descriptor_pool.h>
#include <render/rhi/vulkan/resources/command_pool.h>
#include <utils/debug_macro.h>
#include <spdlog/spdlog.h>
#include <set>

 Vulkan::Renderer::Renderer(GLFWwindow* window)
	 : m_window(window)
{
	const std::vector<Vertex> vertices = {
	};

	const std::vector<uint16_t> indices = {
	};

	if (!CheckValidationLayerSupport(m_validation_layers)) {
		THROW_RUNTIME_ERROR("Asked validation layers are not supported.");
	}

	//Extensions 
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	m_extensions = std::vector(glfw_extensions, glfw_extensions + glfw_extension_count);
	m_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	if (!CheckExtensionSupport(m_extensions)) {
		THROW_RUNTIME_ERROR("Asked extensions are not supported");
	}

	m_debug_info = {};
	m_debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	m_debug_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	m_debug_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	m_debug_info.pfnUserCallback = DebugCallback;
	m_debug_info.pUserData = nullptr;

	m_instance = std::make_unique<Instance>(m_validation_layers, m_extensions, m_debug_info);
	m_debug_messenger = std::make_unique<DebugMessenger>(m_instance->Get(), m_debug_info);
	m_surface = std::make_unique<Surface>(m_window, m_instance->Get());
	m_physical_device = PickPhysicalDevice(m_instance->Get(), m_surface->Get(), m_device_extensions, m_queue_family_indices);
	m_device = std::make_unique<Device>(m_physical_device, m_device_extensions, m_validation_layers, m_queue_family_indices);
	m_depth_format = FindDepthFormat(m_physical_device);
	vkGetDeviceQueue(m_device->Get(), m_queue_family_indices.graphics_family.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device->Get(), m_queue_family_indices.present_family.value(), 0, &m_present_queue);
	vkGetDeviceQueue(m_device->Get(), m_queue_family_indices.transfer_family.value(), 0, &m_transfer_queue);
	m_surface_format = ChooseSwapChainImageFormat(GetSwapChainSupportDetails(m_physical_device, m_surface->Get()));
	m_graphics_command_pool = std::make_unique<CommandPool>(m_device->GetPtr(), m_queue_family_indices.graphics_family.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	m_transfer_command_pool = std::make_unique<CommandPool>(m_device->GetPtr(), m_queue_family_indices.transfer_family.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	m_render_pass = std::make_unique<RenderPass>(this);
	m_swap_chain_ressources = std::make_unique<SwapChainResources>(this);
	m_descriptor_set_layout = std::make_unique<DescriptorSetLayout>(this);
	m_graphics_pipeline = std::make_unique<GraphicsPipeline>(this, "simple_shader_vert.spv", "simple_shader_frag.spv");
	m_vertex_buffer = std::make_unique<PrimitiveBuffer>(this, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	m_index_buffer = std::make_unique<PrimitiveBuffer>(this, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	m_texture = std::make_unique<Texture>(this, "eliasdridi.jpg");
	m_texture_view = std::make_unique<ImageView>(this, m_texture->GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	m_sampler = std::make_unique<Sampler>(this);
	m_frame_resources = std::make_unique<FrameResources>(this, m_texture_view->Get(), m_sampler->Get());
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizedCallback);
	m_render_finish_semaphore.reserve(m_swap_chain_ressources->GetSwapChainImageCount());
	for (int i = 0; i < m_swap_chain_ressources->GetSwapChainImageCount(); ++i) {
		m_render_finish_semaphore.emplace_back(this, 0);
	}
}

Vulkan::Renderer::~Renderer()
{

}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::Renderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		spdlog::error("validation layer: {}", pCallbackData->pMessage);
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		spdlog::warn("validation layer: {}", pCallbackData->pMessage);
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		spdlog::debug("validation layer: {}", pCallbackData->pMessage);
	else
		spdlog::info("validation layer : {}", pCallbackData->pMessage);

	return VK_FALSE;
}

Vulkan::QueueFamilyIndices Vulkan::Renderer::FindQueueFamily(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface)
{
	uint32_t queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families_properties(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families_properties.data());

	QueueFamilyIndices indices = {};
	for (size_t i = 0; i < queue_family_count; ++i) {
		VkQueueFamilyProperties queue_family_properties = queue_families_properties[i];

		if (queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		if (queue_family_properties.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.transfer_family = i;
		}

		VkBool32 support_surface = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &support_surface);

		if (support_surface) {
			indices.present_family = i;
		}
	}

	if (!indices.transfer_family.has_value()) {
		indices.transfer_family = indices.graphics_family;
	}

	return indices;
}

VkPhysicalDevice Vulkan::Renderer::PickPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface, const std::vector<const char*> device_extensions, QueueFamilyIndices& indices)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0) {
		THROW_RUNTIME_ERROR("No device found");
	}

	std::vector<VkPhysicalDevice> physical_devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());

	indices = {};

	for (VkPhysicalDevice physical_device : physical_devices) {

		indices = FindQueueFamily(physical_device, surface);

		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
		for (const VkExtensionProperties& available_extension : available_extensions) {
			required_extensions.erase(available_extension.extensionName);
		}

		bool extensions_supported = required_extensions.empty();

		SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(physical_device, surface);

		bool swap_chain_adequate = false;
		if (extensions_supported) {
			swap_chain_adequate = !swap_chain_support_details.formats.empty() && !swap_chain_support_details.present_modes.empty();
		}

		VkPhysicalDeviceFeatures supported_features;
		vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

		if (indices.IsComplete() && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy) {
			return physical_device;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find suitable physical device");
}

VkSurfaceKHR Vulkan::Renderer::GetSurface() { return m_surface->Get(); }

VkSurfaceFormatKHR Vulkan::Renderer::GetSurfaceFormat() { return m_surface_format; }

VkFormat Vulkan::Renderer::GetDepthFormat() { return m_depth_format; }

Vulkan::QueueFamilyIndices Vulkan::Renderer::GetQueueFamilyIndices() { return m_queue_family_indices; }

Vulkan::SwapChainSupportDetails Vulkan::Renderer::GetSwapChainSupportDetails(const VkPhysicalDevice &physical_device, const VkSurfaceKHR &surface)
{
	Vulkan::SwapChainSupportDetails swap_chain_support_details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface->Get(), &swap_chain_support_details.capabilities);

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface->Get(), &format_count, nullptr);
	swap_chain_support_details.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface->Get(), &format_count, swap_chain_support_details.formats.data());

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface->Get(), &present_mode_count, nullptr);
	swap_chain_support_details.present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface->Get(), &present_mode_count, swap_chain_support_details.present_modes.data());

	return swap_chain_support_details;
}

VkQueue Vulkan::Renderer::GetGraphicsQueue() { return m_graphics_queue; }

VkQueue Vulkan::Renderer::GetTransferQueue() { return m_transfer_queue; }

Vulkan::CommandPool *Vulkan::Renderer::GetGraphicsCommandPool() { return m_graphics_command_pool.get(); }

Vulkan::CommandPool* Vulkan::Renderer::GetTransferCommandPool() { return m_transfer_command_pool.get(); }

VkFormat Vulkan::Renderer::FindSupportedFormat(const VkPhysicalDevice& physical_device, const std::vector<VkFormat> candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features)
{
	for (const VkFormat& format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find supported format");
}

VkFormat Vulkan::Renderer::FindDepthFormat(const VkPhysicalDevice& physical_device)
{
	return FindSupportedFormat(
		physical_device,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void Vulkan::Renderer::RecreateSwapChainResources()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_device->Get());

	m_swap_chain_ressources = std::make_unique<SwapChainResources>(this);
}

void Vulkan::Renderer::FramebufferResizedCallback(GLFWwindow* window, int width, int height)
{
	Vulkan::Renderer* renderer = reinterpret_cast<Vulkan::Renderer*>(glfwGetWindowUserPointer(window));
	renderer->m_frame_buffer_resized = true;
}

void Vulkan::Renderer::UpdateUniformBuffer(uint32_t current_frame)
{
	std::chrono::high_resolution_clock timer;

	static std::chrono::steady_clock::time_point start = timer.now();
	std::chrono::steady_clock::time_point now = timer.now();

	float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), delta_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.perspective = glm::perspective(glm::radians(45.0f), m_swap_chain_ressources->GetSwapchainExtent().width / (float)m_swap_chain_ressources->GetSwapchainExtent().height, 0.001f, 100.0f);
	ubo.perspective[1][1] *= -1;

	memcpy(m_frame_resources->GetUniformBuffer(current_frame)->GetMappedMemory(), &ubo, sizeof(UniformBufferObject));
}

void Vulkan::Renderer::RecordCommandBuffer(uint32_t current_frame, uint32_t swap_chain_image_index)
{
	VkCommandBufferBeginInfo command_begin_info{};
	command_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_begin_info.flags = 0;
	command_begin_info.pInheritanceInfo = nullptr;

	VkCommandBuffer command_buffer = m_frame_resources->GetCommandBuffer(current_frame);
	if (vkBeginCommandBuffer(command_buffer, &command_begin_info) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to begin command buffer.");
	}

	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color = { 0, 0, 0 };
	clear_values[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo render_begin_info{};
	render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_begin_info.framebuffer = m_swap_chain_ressources->GetFramebuffer(swap_chain_image_index);
	render_begin_info.renderPass = m_render_pass->Get();
	render_begin_info.renderArea.extent = m_swap_chain_ressources->GetSwapchainExtent();
	render_begin_info.renderArea.offset = { 0, 0 };
	render_begin_info.clearValueCount = clear_values.size();
	render_begin_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(command_buffer, &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline->Get());

	VkBuffer vertex_buffers[] = {m_vertex_buffer->GetBuffer()};
	VkDeviceSize offset = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, &offset);
	vkCmdBindIndexBuffer(command_buffer, m_index_buffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = m_swap_chain_ressources->GetSwapchainExtent().width;
	viewport.height = m_swap_chain_ressources->GetSwapchainExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissors{};
	scissors.extent = m_swap_chain_ressources->GetSwapchainExtent();
	scissors.offset = { 0, 0 };
	vkCmdSetScissor(command_buffer, 0, 1, &scissors);

	VkDescriptorSet descriptor_set = m_frame_resources->GetDescriptorSet(current_frame);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline->GetPipelineLayout(), 0, 1, &descriptor_set, 0, nullptr);
	vkCmdDrawIndexed(command_buffer, 12, 1, 0, 0, 0);
	vkCmdEndRenderPass(command_buffer);

	vkEndCommandBuffer(command_buffer);
}

void Vulkan::Renderer::Loop()
{
	VkFence in_flight_fence = m_frame_resources->GetInFlightFence(m_current_frame);
	vkWaitForFences(m_device->Get(), 1, &in_flight_fence, VK_TRUE, UINT64_MAX);

	uint32_t swap_chain_image_index;
	VkResult result = vkAcquireNextImageKHR(m_device->Get(), m_swap_chain_ressources->GetSwapchain(), UINT64_MAX, m_frame_resources->GetImageAvailableSemaphore(m_current_frame), VK_NULL_HANDLE, &swap_chain_image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChainResources();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		THROW_RUNTIME_ERROR("Failed to acquire swap chain image");
	}

	vkResetFences(m_device->Get(), 1, &in_flight_fence);

	VkCommandBuffer command_buffer = m_frame_resources->GetCommandBuffer(m_current_frame);
	vkResetCommandBuffer(command_buffer, 0);
	RecordCommandBuffer(m_current_frame, swap_chain_image_index);

	UpdateUniformBuffer(m_current_frame);

	VkSemaphore wait_semaphores[] = { m_frame_resources->GetImageAvailableSemaphore(m_current_frame) };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signal_semaphore[] = { m_render_finish_semaphore[swap_chain_image_index].Get()};

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphore;

	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to submit queue");
	}

	VkSwapchainKHR swap_chains[] = { m_swap_chain_ressources->GetSwapchain() };

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &swap_chain_image_index;
	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(m_present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frame_buffer_resized) {
		m_frame_buffer_resized = false;
		RecreateSwapChainResources();
	}
	else if (result != VK_SUCCESS){
		THROW_RUNTIME_ERROR("Failed to present swap chain image");
	}

	m_current_frame = (m_current_frame + 1) % m_frame_resources->FRAME_IN_FLIGHT;
}

GLFWwindow* Vulkan::Renderer::GetWindow() { return m_window; }

VkPhysicalDevice Vulkan::Renderer::GetPhysicalDevice() { return m_physical_device; }

VkDevice Vulkan::Renderer::GetDevice() { return m_device->Get(); }

VkRenderPass Vulkan::Renderer::GetRenderPass() { return m_render_pass->Get(); }

VkDescriptorSetLayout Vulkan::Renderer::GetDescriptorSetLayout() { return m_descriptor_set_layout->Get(); }

bool Vulkan::Renderer::CheckValidationLayerSupport(const std::vector<const char*>& validation_layer)
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layer) {
		bool layer_found = false;

		for (const VkLayerProperties& available_layer : available_layers) {
			if (strcmp(available_layer.layerName, layer_name) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	return true;
}

bool Vulkan::Renderer::CheckExtensionSupport(std::vector<const char*> extensions)
{
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

	for (const char* extension : extensions) {
		bool found = false;

		for (const VkExtensionProperties& available_extension : available_extensions) {
			if (strcmp(available_extension.extensionName, extension) == 0) {
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

VkSurfaceFormatKHR Vulkan::Renderer::ChooseSwapChainImageFormat(const SwapChainSupportDetails& swap_chain_support_details)
{
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
