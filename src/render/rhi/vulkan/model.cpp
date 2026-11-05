#include "model.h"
#include <render/rhi/vulkan/renderer.h>
#include <utils/build_macro.h>

Vulkan::Model::Model(const Renderer* renderer, const Render::Model& model)
{
	uint32_t material_count = model.materials.size() + 1;

	std::array<vk::DescriptorPoolSize, 2> pool_sizes{
		{
			{ .type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = material_count * Material::MATERIAL_TEXTURE_COUNT },
			{ .type = vk::DescriptorType::eUniformBuffer, .descriptorCount = material_count }
		}
	};

	vk::DescriptorPoolCreateInfo descriptor_pool_info = {
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = material_count,
		.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data()
	};

	_descriptor_pool = vk::raii::DescriptorPool(renderer->GetDevice(), descriptor_pool_info);

	std::vector<vk::DescriptorSetLayout> layouts(material_count, renderer->GetMatDescriptorSetLayout());
	vk::DescriptorSetAllocateInfo alloc_info{
		.descriptorPool = _descriptor_pool,
		.descriptorSetCount = static_cast<uint32_t>(material_count),
		.pSetLayouts = layouts.data()
	};

	_descriptor_sets = renderer->GetDevice().allocateDescriptorSets(alloc_info);

	{
		auto [image_default_white, default_white_memory] = CreateTexture(renderer, "default_white.png", vk::Format::eR8G8B8A8Srgb);
		auto default_white_view = CreateImageView(renderer, image_default_white, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

		_default_white = {
			"Default White",
			std::move(default_white_memory),
			std::move(image_default_white),
			std::move(default_white_view)
		};

		auto [gpu_image_normal, default_normal_memory] = CreateTexture(renderer, "default_normal.png", vk::Format::eR8G8B8A8Unorm);
		auto default_normal_view = CreateImageView(renderer, gpu_image_normal, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

		_default_normal = {
			"Default Normal",
			std::move(default_normal_memory),
			std::move(gpu_image_normal),
			std::move(default_normal_view)
		};
	}

	_default_sampler = CreateSampler(renderer);

	_meshes.reserve(model.meshes.size());

	for (const auto& mesh : model.meshes) {

		Mesh gpu_mesh = {};
		gpu_mesh.primitives.reserve(mesh.primitives.size());
		gpu_mesh.name = mesh.name;

		for (const auto& primitive : mesh.primitives) {

			gpu_mesh.primitives.push_back({
					CreatePrimitiveBuffer(renderer, primitive.vertices, vk::BufferUsageFlagBits::eVertexBuffer),
					CreatePrimitiveBuffer(renderer, primitive.indices, vk::BufferUsageFlagBits::eIndexBuffer),
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

		auto [gpu_image, memory] = CreateTexture(renderer, image.pixels, image.width, image.height, image.channels, format);
		auto view = CreateImageView(renderer, gpu_image, format, vk::ImageAspectFlagBits::eColor);

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
		_samplers.emplace_back(CreateSampler(renderer));
	}

	_materials.reserve(material_count);

	for (int i = 0; i < model.materials.size(); ++i) {

		const Render::Material &material = model.materials[i];

		_materials.emplace_back(
			CreateMaterial(
				renderer, model, material.name, i,
				material.base_color_texture_index, material.emmisive_texture_index, material.normal_texture_index, material.occlusion_texture_index,
				material.metallic_roughness_texture_index, material.base_color, material.emissive, material.metallic_factor, material.roughness_factor)
		);
	}

	_materials.emplace_back(
		CreateMaterial(renderer, model, "Default Material",
			model.materials.size(), -1, -1, -1, -1, -1, glm::vec4(1), glm::vec3(0), 0, 1)
	);
}

const std::vector<Vulkan::Mesh>& Vulkan::Model::GetMeshes() const { return _meshes; }

const std::vector<Vulkan::Material>& Vulkan::Model::GetMaterials() const { return _materials; }

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
	const Renderer* renderer, const Render::Model& model, std::string material_name, int material_index,
	int base_color_tex_idx, int emmisive_tex_idx, int normal_tex_idx, int occlusion_tex_idx, int metallic_roughness_tex_idx,
	glm::vec4 base_color, glm::vec3 emissive, float metallic_factor, float roughness_factor)
{
	std::array<vk::DescriptorImageInfo, Material::MATERIAL_TEXTURE_COUNT> descriptor_image_infos = {
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

	PrimitiveBuffer uniform_buffer = CreatePrimitiveBuffer<Material::Uniform>(renderer, uniform_data, vk::BufferUsageFlagBits::eUniformBuffer);

	vk::DescriptorBufferInfo descriptor_buffer_info = {
		.buffer = uniform_buffer.GetBuffer(),
		.offset = 0,
		.range = sizeof(Material::Uniform)
	};

	vk::DescriptorSet descriptor_set = _descriptor_sets[material_index];
	std::array<vk::WriteDescriptorSet, 2> descriptor_write{
		{
			{
				.dstSet = descriptor_set,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = descriptor_image_infos.size(),
				.descriptorType = vk::DescriptorType::eCombinedImageSampler,
				.pImageInfo = descriptor_image_infos.data()
			},
			{
				.dstSet = descriptor_set,
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &descriptor_buffer_info
			}
		}
	};

	renderer->GetDevice().updateDescriptorSets(descriptor_write, {});

	return {
		material_name,
		descriptor_set,
		std::move(uniform_buffer)
	};
}

const Vulkan::PrimitiveBuffer& Vulkan::Primitive::GetVertexBuffer() const { return vertex_buffer; }
const Vulkan::PrimitiveBuffer& Vulkan::Primitive::GetIndexBuffer() const { return index_buffer; }
uint32_t Vulkan::Primitive::GetIndexCount() const { return index_count; }
int Vulkan::Primitive::GetMaterialIndex() const { return material_index; }
