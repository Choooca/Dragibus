#include "model.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <render/rhi/vulkan/context.h>
#include <render/rhi/vulkan/renderer.h>
#include <utils/build_macro.h>
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/descriptor_layouts.h>

Vulkan::Model::Model(const Vulkan::Context& context, const Vulkan::Renderer& renderer, const Render::Model& model)
{
	uint32_t material_count = model.materials.size() + 1;

	{
		auto [image_default_white, default_white_memory] = CreateTexture(context, "default_white.png", vk::Format::eR8G8B8A8Srgb);
		auto default_white_view = CreateImageView(context, image_default_white, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

		_default_white = {
			"Default White",
			std::move(default_white_memory),
			std::move(image_default_white),
			std::move(default_white_view)
		};

		auto [gpu_image_normal, default_normal_memory] = CreateTexture(context, "default_normal.png", vk::Format::eR8G8B8A8Unorm);
		auto default_normal_view = CreateImageView(context, gpu_image_normal, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

		_default_normal = {
			"Default Normal",
			std::move(default_normal_memory),
			std::move(gpu_image_normal),
			std::move(default_normal_view)
		};
	}

	_default_sampler = CreateSampler(context);

	_default_scene = model.default_scene;

	_scenes.reserve(model.scenes.size());

	for (const auto& scene : model.scenes) {
		Scene gpu_scene = {};

		gpu_scene.name = scene.name;
		gpu_scene.nodes.assign(scene.nodes.data(), scene.nodes.data() + scene.nodes.size());

		_scenes.push_back(gpu_scene);
	}

	_nodes.reserve(model.nodes.size());

	for (const auto& node : model.nodes) {
		Node gpu_node = {};

		gpu_node.name = node.name;
		gpu_node.mesh = node.mesh;
		gpu_node.children.assign(node.children.data(), node.children.data() + node.children.size());

		if (node.has_matrix) {
			gpu_node.model_matrix = glm::make_mat4(node.matrix);
		}
		else {
			glm::mat4 model_matrix(1.0f);

			glm::vec3 translation = glm::make_vec3(node.translation);
			glm::quat rotation = glm::make_quat(node.rotation);
			glm::vec3 scale = glm::make_vec3(node.scale);
			
			model_matrix = glm::translate(model_matrix, translation);
			model_matrix *= glm::mat4_cast(rotation);
			model_matrix = glm::scale(model_matrix, scale);

			gpu_node.model_matrix = model_matrix;
		}

		_nodes.emplace_back(std::move(gpu_node));
	}

	_meshes.reserve(model.meshes.size());

	for (const auto& mesh : model.meshes) {

		Mesh gpu_mesh = {};
		gpu_mesh.primitives.reserve(mesh.primitives.size());
		gpu_mesh.name = mesh.name;

		for (const auto& primitive : mesh.primitives) {

			gpu_mesh.primitives.push_back({
					CreateDeviceLocalBuffer(context, primitive.vertices, vk::BufferUsageFlagBits::eVertexBuffer),
					CreateDeviceLocalBuffer(context, primitive.indices, vk::BufferUsageFlagBits::eIndexBuffer),
					static_cast<uint32_t>(primitive.indices.size()),
					primitive.material_index
				});
		}

		_meshes.emplace_back(std::move(gpu_mesh));
	}

	std::vector<bool> is_srgb(model.images.size());

	for (int i = 0; i < model.materials.size(); ++i) {

		const Render::Material& material = model.materials[i];

		if (material.base_color_texture_index != -1 && model.textures[material.base_color_texture_index].image_index != -1)
			is_srgb[model.textures[material.base_color_texture_index].image_index] = true;
		if (material.emmisive_texture_index != -1 && model.textures[material.emmisive_texture_index].image_index != -1)
			is_srgb[model.textures[material.emmisive_texture_index].image_index] = true;
	}

	_images.reserve(model.images.size());

	for (int i = 0; i < model.images.size(); ++i) {

		const Render::Image& image = model.images[i];

		vk::Format format = is_srgb[i] ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

		auto [gpu_image, memory] = CreateTexture(context, image.pixels, image.width, image.height, image.channels, format);
		auto view = CreateImageView(context, gpu_image, format, vk::ImageAspectFlagBits::eColor);

		_images.emplace_back(
			image.name,
			std::move(memory),
			std::move(gpu_image),
			std::move(view)
		);
	}

	_samplers.reserve(model.samplers.size());

	for (const auto& sampler : model.samplers) {

		//TODO create good sampler
		_samplers.emplace_back(CreateSampler(context));
	}

	_materials.reserve(material_count);

	for (int i = 0; i < model.materials.size(); ++i) {

		const Render::Material &material = model.materials[i];

		_materials.emplace_back(
			CreateMaterial(
				context, model, material.name, i,
				material.base_color_texture_index, material.emmisive_texture_index, material.normal_texture_index, material.occlusion_texture_index,
				material.metallic_roughness_texture_index, material.base_color, material.emissive, material.metallic_factor, material.roughness_factor)
		);
	}


	_materials.emplace_back(
		CreateMaterial(context, model, "Default Material",
			model.materials.size(), -1, -1, -1, -1, -1, glm::vec4(1), glm::vec3(0), 0, 1)
	);

	DescriptorSetGroupDesc mat_group_desc;
	mat_group_desc.descriptor_set_layout = renderer.GetMatDescriptorSetLayout();
	mat_group_desc.bindings = MAT_BINDINGS;
	mat_group_desc.resources.resize(material_count);

	for (uint32_t i = 0; i < material_count; ++i) {
		mat_group_desc.resources[i] = {
			_materials[i].image_infos,
			std::vector{ _materials[i].uniform_buffer_info }
		};
	}

	_mat_descriptor_sets = CreateDescriptorSetGroup(context, mat_group_desc);
}

const Vulkan::Scene& Vulkan::Model::GetDefaultScene() const { 
	
	if (_default_scene == -1) {
		if(_scenes.size() <= 0)
			return empty_scene;
		return _scenes[0];
	}

	if (static_cast<size_t>(_default_scene) >= _scenes.size())
		THROW_RUNTIME_ERROR("Default scene index is out of range");

	return _scenes[_default_scene]; 
}

const std::vector<Vulkan::Mesh>& Vulkan::Model::GetMeshes() const { return _meshes; }

const Vulkan::Mesh& Vulkan::Model::GetMesh(int index) const { 
	if (index < 0 || static_cast<size_t>(index) >= _meshes.size())
		THROW_RUNTIME_ERROR("Get Mesh index out of range");
	return _meshes[index]; 
}

vk::DescriptorSet Vulkan::Model::GetMaterialDescriptor(int index) const { 
	
	if (index < 0 || index >= _mat_descriptor_sets.sets.size() - 1)
		THROW_RUNTIME_ERROR("Index out of range");

	return _mat_descriptor_sets.sets[index]; 
}

vk::DescriptorSet Vulkan::Model::GetDefaultMaterialDescriptor() const { return _mat_descriptor_sets.sets.back(); }

const Vulkan::Node& Vulkan::Model::GetNode(int index) const { 
	if (index < 0 || static_cast<size_t>(index) >= _nodes.size())
		THROW_RUNTIME_ERROR("Get Node index out of range");
	return _nodes[index]; 
}

vk::DescriptorImageInfo Vulkan::Model::CreateImageInfo(const Render::Model& model, int texture_index, vk::ImageView view_fallback) const
{
	vk::DescriptorImageInfo out = { .sampler = _default_sampler, .imageView = view_fallback, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

	if (texture_index == -1)
		return out;

	const Render::Texture& texture = model.textures[texture_index];
	if (texture.sampler_index != -1)
		out.sampler = _samplers[texture.sampler_index];
	if (texture.image_index != -1)
		out.imageView = _images[texture.image_index].view;

	return out;
}

Vulkan::Material Vulkan::Model::CreateMaterial(
	const Context& context, const Render::Model& model, std::string material_name, int material_index,
	int base_color_tex_idx, int emmisive_tex_idx, int normal_tex_idx, int occlusion_tex_idx, int metallic_roughness_tex_idx,
	glm::vec4 base_color, glm::vec3 emissive, float metallic_factor, float roughness_factor)
{
	std::vector<vk::DescriptorImageInfo> image_infos = {
		{
			{ CreateImageInfo(model, base_color_tex_idx, _default_white.view) },
			{ CreateImageInfo(model, emmisive_tex_idx, _default_white.view) },
			{ CreateImageInfo(model, normal_tex_idx, _default_normal.view) },
			{ CreateImageInfo(model, occlusion_tex_idx, _default_white.view) },
			{ CreateImageInfo(model, metallic_roughness_tex_idx, _default_white.view) },
		}
	};

	Material::Uniform uniform_data =
	{
		.base_color = base_color,
		.emissive = emissive,
		.metallic_factor = metallic_factor,
		.roughness_factor = roughness_factor
	};

	DeviceLocalBuffer uniform_buffer = CreateDeviceLocalBuffer<Material::Uniform>(context, uniform_data, vk::BufferUsageFlagBits::eUniformBuffer);

	vk::DescriptorBufferInfo descriptor_buffer_info = {
		.buffer = uniform_buffer.GetBuffer(),
		.offset = 0,
		.range = sizeof(Material::Uniform)
	};

	return {
		material_name,
		std::move(uniform_buffer),
		descriptor_buffer_info,
		std::move(image_infos)
	};
}

const Vulkan::DeviceLocalBuffer& Vulkan::Primitive::GetVertexBuffer() const { return vertex_buffer; }
const Vulkan::DeviceLocalBuffer& Vulkan::Primitive::GetIndexBuffer() const { return index_buffer; }
uint32_t Vulkan::Primitive::GetIndexCount() const { return index_count; }
int Vulkan::Primitive::GetMaterialIndex() const { return material_index; }
