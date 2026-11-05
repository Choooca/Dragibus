#pragma once
#include <vector>
#include <memory>
#include <string>
#include <map>

#include <vulkan/vulkan_raii.hpp>
#include <render/rhi/vulkan/resource_utils.h>

namespace Render {
	struct Model;
}

namespace Vulkan {

	class PrimitiveBuffer;
	class Renderer;

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
		PrimitiveBuffer uniform_buffer;
	};

	struct Primitive {

		const PrimitiveBuffer& GetVertexBuffer() const;
		const PrimitiveBuffer& GetIndexBuffer() const;
		uint32_t GetIndexCount() const;
		int GetMaterialIndex() const;

		PrimitiveBuffer vertex_buffer;
		PrimitiveBuffer index_buffer;
		uint32_t index_count = 0;
		int material_index = -1;
	};

	struct Mesh {
		std::string name;
		std::vector<Primitive> primitives;
	};

	class Model {

	public:

		Model(const Renderer* renderer, const Render::Model& model);

		const std::vector<Mesh>& GetMeshes() const;
		const std::vector<Material>& GetMaterials() const;

	private:

		std::vector<Mesh> _meshes;
		std::vector<Material> _materials;
		std::vector<Image> _images;
		std::vector<vk::raii::Sampler> _samplers;

		vk::DescriptorImageInfo CreateImageInfo(const Render::Model& model, int texture_index, vk::ImageView fallback) const;

		Vulkan::Material CreateMaterial(
			const Renderer* renderer, const Render::Model& model, std::string material_name, int material_index,
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