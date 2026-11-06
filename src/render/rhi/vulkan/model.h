#pragma once
#include <vector>
#include <memory>
#include <string>
#include <map>

#include <vulkan/vulkan_raii.hpp>
#include <render/rhi/vulkan/resource_utils.h>
#include <render/primitives.h>

namespace Render {
	struct Model;
}

namespace Vulkan {

	class DeviceLocalBuffer;
	class Renderer;
	class Context;

	struct Vertex : Render::Vertex {

		static vk::VertexInputBindingDescription GetBindingDescription() {
			vk::VertexInputBindingDescription out{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = vk::VertexInputRate::eVertex
			};

			return out;
		}

		static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescription() {
			std::array<vk::VertexInputAttributeDescription, 4> out{};

			out[0].binding = 0;
			out[0].location = 0;
			out[0].format = vk::Format::eR32G32B32Sfloat;
			out[0].offset = offsetof(Vertex, position);

			out[1].binding = 0;
			out[1].location = 1;
			out[1].format = vk::Format::eR32G32B32Sfloat;
			out[1].offset = offsetof(Vertex, normal);

			out[2].binding = 0;
			out[2].location = 2;
			out[2].format = vk::Format::eR32G32B32A32Sfloat;
			out[2].offset = offsetof(Vertex, tangent);

			out[3].binding = 0;
			out[3].location = 3;
			out[3].format = vk::Format::eR32G32Sfloat;;
			out[3].offset = offsetof(Vertex, tex_coord0);

			return out;
		}
	};

	struct Image {
		std::string name;

		vk::raii::DeviceMemory memory = nullptr;
		vk::raii::Image image = nullptr;
		vk::raii::ImageView view = nullptr;
	};

	struct Material {

		struct Uniform {
			alignas(16) glm::vec4 base_color;
			alignas(16) glm::vec3 emissive;
			alignas(4) float metallic_factor;
			alignas(4) float roughness_factor;
		};

		static constexpr uint8_t MATERIAL_TEXTURE_COUNT = 5;

		std::string name = "";
		vk::DescriptorSet descriptor_sets = nullptr;
		DeviceLocalBuffer uniform_buffer;
	};

	struct Primitive {

		const DeviceLocalBuffer& GetVertexBuffer() const;
		const DeviceLocalBuffer& GetIndexBuffer() const;
		uint32_t GetIndexCount() const;
		int GetMaterialIndex() const;

		DeviceLocalBuffer vertex_buffer;
		DeviceLocalBuffer index_buffer;
		uint32_t index_count = 0;
		int material_index = -1;
	};

	struct Mesh {
		std::string name;
		std::vector<Primitive> primitives;
	};

	class Model {

	public:

		Model(const Vulkan::Context &context, const Vulkan::Renderer &renderer, const Render::Model& model);

		const std::vector<Mesh>& GetMeshes() const;
		const std::vector<Material>& GetMaterials() const;

	private:

		std::vector<Mesh> _meshes;
		std::vector<Material> _materials;
		std::vector<Image> _images;
		std::vector<vk::raii::Sampler> _samplers;

		vk::DescriptorImageInfo CreateImageInfo(const Render::Model& model, int texture_index, vk::ImageView fallback) const;

		Vulkan::Material CreateMaterial(
			const Context &context, const Render::Model& model, std::string material_name, int material_index,
			int base_color_tex_idx, int emmisive_tex_idx, int normal_tex_idx,
			int occlusion_tex_idx, int metallic_roughness_tex_idx,
			glm::vec4 base_color, glm::vec3 emissive, float metallic_factor, float roughness_factor);

		vk::raii::DescriptorPool _descriptor_pool = nullptr;
		std::vector<vk::raii::DescriptorSet> _descriptor_sets;

		//TODO move default 
		Image _default_white;
		Image _default_normal;
		vk::raii::Sampler _default_sampler = nullptr;
	};
}