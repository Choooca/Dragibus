#include "gltf_model.h"

#include <sstream>
#include <unordered_map>

#include <stb_image.h>

#include <utils/build_macro.h>
#include <utils/debug_macro.h>

#include <render/primitives.h>

Render::Model Render::GLTF::ParseGLTFModel(const std::string& model_name)
{
	const std::string& model_path = std::string(MODELS_DIR) + model_name;

	tg3_parse_options options;
	tg3_error_stack errors{};
	tg3_model model{};

	struct TG3Raii {
		tg3_model& model;
		tg3_error_stack& errors;
		~TG3Raii() {
			tg3_model_free(&model);
			tg3_error_stack_free(&errors);
		}
	};
	TG3Raii tg3_raii{ model, errors };

	tg3_parse_options_init(&options);
	tg3_error_stack_init(&errors);
	tg3_error_code err = tg3_parse_file(&model, &errors, model_path.data(), model_path.length(), &options);
	if (err != TG3_OK) {

		std::stringstream msg;

		for (uint32_t i = 0; i < errors.count; i++) {
			msg << (int)errors.entries[i].severity << " " << (errors.entries[i].message ? errors.entries[i].message : "(null)") << "\n";
		}

		THROW_RUNTIME_ERROR("{}", msg.str());
	}

	const std::filesystem::path base_dir = std::filesystem::path(model_path).parent_path();

	Render::Model ret = {};

	ret.meshes.resize(model.meshes_count);
	for (uint32_t i = 0; i < model.meshes_count; ++i) {
		ret.meshes[i] = ParseMesh(model.meshes[i], model);
	}

	ret.materials.resize(model.materials_count);
	for (uint32_t i = 0; i < model.materials_count; ++i) {
		ret.materials[i] = ParseMaterial(model.materials[i]);
	}

	ret.textures.resize(model.textures_count);
	for (uint32_t i = 0; i < model.textures_count; ++i) {
		ret.textures[i] = ParseTexture(model.textures[i]);
	}

	ret.samplers.resize(model.samplers_count);
	for (uint32_t i = 0; i < model.samplers_count; ++i) {
		ret.samplers[i] = ParseSampler(model.samplers[i]);
	}

	ret.images.resize(model.images_count);
	for (uint32_t i = 0; i < model.images_count; ++i) {
		ret.images[i] = ParseImage(model.images[i], model, base_dir);
	}

	return ret;
}


Render::Mesh Render::GLTF::ParseMesh(const tg3_mesh& mesh, const tg3_model& model)
{
	Mesh out{};

	// Name
	out.name.assign(mesh.name.data, mesh.name.len);

	//Primitives
	out.primitives.resize(mesh.primitives_count);
	for (uint32_t i = 0; i < mesh.primitives_count; ++i) {
		out.primitives[i] = ParsePrimitive(mesh.primitives[i], model);
	}

	return out;
}

Render::Material Render::GLTF::ParseMaterial(const tg3_material& material)
{
	Material out = {};

	out.name.assign(material.name.data, material.name.len);
	out.double_sided = material.double_sided;
	out.emissive = glm::vec3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);

	const std::unordered_map<std::string, Render::Material::AlphaMode> alpha_string_to_enum = {
		{ "OPAQUE", Render::Material::AlphaMode::OPAQUE},
		{ "BLEND", Render::Material::AlphaMode::BLEND},
		{ "MASK", Render::Material::AlphaMode::MASK}
	};

	std::string alpha_mode_key = std::string(material.alpha_mode.data, material.alpha_mode.len);

	auto itr = alpha_string_to_enum.find(alpha_mode_key);
	if (itr != alpha_string_to_enum.end()) {
		out.alpha_mode = itr->second;
	}

	out.alpha_cutoff = material.alpha_cutoff;

	out.emmisive_texture_index = material.emissive_texture.index;
	out.emmisive_texture_texcoord = material.emissive_texture.tex_coord;

	out.normal_texture_index = material.normal_texture.index;
	out.normal_texture_texcoord = material.normal_texture.tex_coord;

	out.occlusion_texture_index = material.occlusion_texture.index;
	out.occlusion_texture_texcoord = material.occlusion_texture.tex_coord;

	out.base_color = {
		material.pbr_metallic_roughness.base_color_factor[0],
		material.pbr_metallic_roughness.base_color_factor[1],
		material.pbr_metallic_roughness.base_color_factor[2],
		material.pbr_metallic_roughness.base_color_factor[3]
	};

	out.base_color_texture_index = material.pbr_metallic_roughness.base_color_texture.index;
	out.base_color_texture_texcoord = material.pbr_metallic_roughness.base_color_texture.tex_coord;

	out.metallic_factor = material.pbr_metallic_roughness.metallic_factor;
	out.roughness_factor = material.pbr_metallic_roughness.roughness_factor;
	out.metallic_roughness_texture_index = material.pbr_metallic_roughness.metallic_roughness_texture.index;
	out.metallic_roughness_texture_texcoord = material.pbr_metallic_roughness.metallic_roughness_texture.tex_coord;

	return out;
}

Render::Texture Render::GLTF::ParseTexture(const tg3_texture& texture)
{
	Texture out{};

	out.name.assign(texture.name.data, texture.name.len);
	out.sampler_index = texture.sampler;
	out.image_index = texture.source;

	return out;
}

Render::Sampler Render::GLTF::ParseSampler(const tg3_sampler& sampler)
{
	Sampler out{};
	out.name.assign(sampler.name.data, sampler.name.len);
	out.mag_filter = (Sampler::FilterMode)sampler.mag_filter;
	out.min_filter = (Sampler::FilterMode)sampler.min_filter;
	out.wrap_s = (Sampler::WrapMode)sampler.wrap_s;
	out.wrap_t = (Sampler::WrapMode)sampler.wrap_t;

	return out;
}

Render::Image Render::GLTF::ParseImage(const tg3_image& image, const tg3_model &model, const std::filesystem::path &base_dir)
{
	Render::Image out{};

	out.name.assign(image.name.data, image.name.len);

	stbi_uc* pixels = nullptr;
	int width, height, channel;

	if (image.buffer_view != -1) {

		const tg3_buffer_view& view = model.buffer_views[image.buffer_view];
		const tg3_buffer& buffer = model.buffers[view.buffer];
		pixels = stbi_load_from_memory(buffer.data.data + view.byte_offset, static_cast<int>(view.byte_length), &width, &height, &channel, STBI_rgb_alpha);
	}
	else {
		const std::filesystem::path path = base_dir / std::string(image.uri.data, image.uri.len);
		pixels = stbi_load(path.string().c_str(), &width, &height, &channel, STBI_rgb_alpha);
	}

	if (!pixels)
		THROW_RUNTIME_ERROR("Failed to load image {} : {}", out.name, stbi_failure_reason());

	out.pixels.assign(pixels, pixels + static_cast<size_t>(width) * height * 4);
	out.channels = 4;
	out.width = width;
	out.height = height;
	stbi_image_free(pixels);

	return out;
}

Render::Primitive Render::GLTF::ParsePrimitive(const tg3_primitive& primitive, const tg3_model &model)
{
	Primitive out{};

	//Draw mode
	const DRAW_MODE modes[7] = {
		DRAW_MODE::POINTS,
		DRAW_MODE::LINES,
		DRAW_MODE::LINE_LOOP,
		DRAW_MODE::LINE_STRIP,
		DRAW_MODE::TRIANGLES,
		DRAW_MODE::TRIANGLE_STRIP,
		DRAW_MODE::TRIANGLE_FAN
	};

	out.mode = primitive.mode >= 0 && primitive.mode < 7 ? modes[primitive.mode] : DRAW_MODE::TRIANGLES;

	//Indices
	if (primitive.indices == -1) {
		out.indices = {};
	}
	else {
		tg3_accessor indices_accessor = model.accessors[primitive.indices];

		if (indices_accessor.buffer_view != -1) {
			tg3_buffer_view indices_buffer_view = model.buffer_views[indices_accessor.buffer_view];
			tg3_buffer indices_buffer = model.buffers[indices_buffer_view.buffer];
			uint64_t offset = indices_accessor.byte_offset + indices_buffer_view.byte_offset;
			uint32_t stride = tg3_accessor_byte_stride(&indices_accessor, &indices_buffer_view);

			out.indices.resize(indices_accessor.count);
			for (uint64_t i = 0; i < indices_accessor.count; ++i) {

				const uint8_t* address = (indices_buffer.data.data + offset) + stride * i;

				switch (indices_accessor.component_type) {
					case 5121:
						out.indices[i] = ParseByte<uint8_t>(address);
						break;
					case 5123:
						out.indices[i] = ParseByte<uint16_t>(address);
						break;
					case 5125:
						out.indices[i] = ParseByte<uint32_t>(address);
						break;
					default:
						THROW_INVALID_ARGUMENT("Invalid component type for indice : " + std::to_string(indices_accessor.component_type));
				}
			}
		}
	}


	//Vertices
	uint32_t vertice_count = 0;
	for (uint32_t i = 0; i < primitive.attributes_count; ++i) {
		std::string key;
		key.assign((primitive.attributes + i)->key.data, (primitive.attributes + i)->key.len);
		if (key == "POSITION") {
			vertice_count = model.accessors[primitive.attributes[i].value].count;
		}
 	}
	out.vertices.resize(vertice_count);

	for (uint32_t i = 0; i < primitive.attributes_count; ++i) {
		std::string key;
		key.assign((primitive.attributes + i)->key.data, (primitive.attributes + i)->key.len);
		tg3_accessor accessor = model.accessors[(primitive.attributes + i)->value];

		if (accessor.buffer_view != -1) {
			
			tg3_buffer_view buffer_view = model.buffer_views[accessor.buffer_view];
			tg3_buffer buffer = model.buffers[buffer_view.buffer];
			uint64_t offset = accessor.byte_offset + buffer_view.byte_offset;
			uint32_t stride = tg3_accessor_byte_stride(&accessor, &buffer_view);

			if (key == "POSITION") {
				for (uint32_t j = 0; j < accessor.count; ++j) {
					const uint8_t* address = (buffer.data.data + offset) + stride * j;

					out.vertices[j].position = ParseByte<glm::vec3>(address);
				}
			}
			else if (key == "NORMAL") {
				for (uint32_t j = 0; j < accessor.count; ++j) {
					const uint8_t* address = (buffer.data.data + offset) + stride * j;

					out.vertices[j].normal = ParseByte<glm::vec3>(address);
				}
			}
			else if (key == "TANGENT") {
				for (uint32_t j = 0; j < accessor.count; ++j) {
					const uint8_t* address = (buffer.data.data + offset) + stride * j;

					out.vertices[j].tangent = ParseByte<glm::vec4>(address);
				}
			}
			else if (key == "TEXCOORD_0") {
				for (uint32_t j = 0; j < accessor.count; ++j) {
					const uint8_t* address = (buffer.data.data + offset) + stride * j;

					switch (accessor.component_type) {
					case 5126:
						out.vertices[j].tex_coord0 = ParseByte<glm::vec2>(address);
						break;
					case 5121:
						out.vertices[j].tex_coord0 = glm::vec2(ParseByte<glm::vec<2, uint8_t ,glm::qualifier::defaultp>>(address)) / 255.0f;
						break;
					case 5123:
						out.vertices[j].tex_coord0 = glm::vec2(ParseByte<glm::vec<2, uint16_t, glm::qualifier::defaultp>>(address)) / 65535.0f;
						break;
					default:
						THROW_INVALID_ARGUMENT("Invalid component type for texcoord0 : " + std::to_string(accessor.component_type));
					}
				}
			}
		}
	}

	//Material
	out.material_index = primitive.material;

	return out;
}