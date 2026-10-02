#include "gltf_model.h"

#include <sstream>
#include <unordered_map>

#include <utils/build_macro.h>
#include <utils/debug_macro.h>

#include <render/primitives.h>

#pragma region CTors/Dtors

GLTFModel::GLTFModel(const std::string& model_name) {

	const std::string& model_path = std::string(MODELS_DIR) + model_name;

	tg3_parse_options_init(&_options);
	tg3_error_stack_init(&_errors);
	tg3_error_code err = tg3_parse_file(&_model, &_errors, model_path.data(), model_path.length(), &_options);
	if (err != TG3_OK) {

		std::stringstream msg;

		for (uint32_t i = 0; i < _errors.count; i++) {
			msg << (int)_errors.entries[i].severity << " " << (_errors.entries[i].message ? _errors.entries[i].message : "(null)") << "\n";
		}

		PRINT_RUNTIME_ERROR(msg.str());
	}

	_meshes.resize(_model.meshes_count);
	for (uint32_t i = 0; i < _model.meshes_count; ++i) {
		_meshes[i] = ParseMesh(_model.meshes[i]);
	}

	_materials.resize(_model.materials_count);
	for (uint32_t i = 0; i <  _model.materials_count; ++i) {
		_materials[i] = ParseMaterial(_model.materials[i]);
	}

	_textures.resize(_model.textures_count);
	for (uint32_t i = 0; i < _model.textures_count; ++i) {
		_textures[i] = ParseTexture(_model.textures[i]);
	}

	_samplers.resize(_model.samplers_count);
	for (uint32_t i = 0; i < _model.samplers_count; ++i) {
		_samplers[i] = ParseSampler(_model.samplers[i]);
	}

	_images.resize(_model.images_count);
	for (uint32_t i = 0; i < _model.images_count; ++i) {
		_images[i] = ParseImage(_model.images[i]);
	}

}

GLTFModel::~GLTFModel() {
	tg3_model_free(&_model);
	tg3_error_stack_free(&_errors);
}

std::vector<Mesh> &&GLTFModel::GetMeshes()
{
	return std::move(_meshes);
}

#pragma endregion

Mesh GLTFModel::ParseMesh(const tg3_mesh &mesh)
{
	Mesh out{};

	// Name
	out.name.assign(mesh.name.data, mesh.name.len);

	//Primitives
	out.primitives.resize(mesh.primitives_count);
	for (uint32_t i = 0; i < mesh.primitives_count; ++i) {
		out.primitives[i] = ParsePrimitive(mesh.primitives[i]);
	}

	return out;
}

Material GLTFModel::ParseMaterial(const tg3_material& material)
{
	Material out = {};

	out.name.assign(material.name.data, material.name.len);
	out.double_sided = material.double_sided;
	out.emissive = glm::vec3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);

	const std::unordered_map<std::string, Material::AlphaMode> alpha_string_to_enum = {
		{ "OPAQUE", Material::AlphaMode::OPAQUE},
		{ "BLEND", Material::AlphaMode::BLEND},
		{ "MASK", Material::AlphaMode::MASK}
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

Texture GLTFModel::ParseTexture(const tg3_texture& texture)
{
	Texture out{};

	out.name.assign(texture.name.data, texture.name.len);
	out.sampler_index = texture.sampler;
	out.image_index = texture.source;

	return out;
}

Sampler GLTFModel::ParseSampler(const tg3_sampler& sampler)
{
	Sampler out{};
	out.name.assign(sampler.name.data, sampler.name.len);
	out.mag_filter = (Sampler::FilterMode)sampler.mag_filter;
	out.min_filter = (Sampler::FilterMode)sampler.min_filter;
	out.wrap_s = (Sampler::WrapMode)sampler.wrap_s;
	out.wrap_t = (Sampler::WrapMode)sampler.wrap_t;

	return out;
}

Image GLTFModel::ParseImage(const tg3_image& image)
{
	Image out{};
	out.name.assign(image.name.data, image.name.len);
	out.channels = image.component;
	out.width = image.width;
	out.height = image.height;
	out.pixels = std::vector<uint8_t>(image.image.data, image.image.data + image.image.count);

	return out;
}

Primitive GLTFModel::ParsePrimitive(const tg3_primitive& primitive)
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
		tg3_accessor indices_accessor = _model.accessors[primitive.indices];

		if (indices_accessor.buffer_view != -1) {
			tg3_buffer_view indices_buffer_view = _model.buffer_views[indices_accessor.buffer_view];
			tg3_buffer indices_buffer = _model.buffers[indices_buffer_view.buffer];
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
			vertice_count = _model.accessors[primitive.attributes[i].value].count;
		}
 	}
	out.vertices.resize(vertice_count);

	for (uint32_t i = 0; i < primitive.attributes_count; ++i) {
		std::string key;
		key.assign((primitive.attributes + i)->key.data, (primitive.attributes + i)->key.len);
		tg3_accessor accessor = _model.accessors[(primitive.attributes + i)->value];

		if (accessor.buffer_view != -1) {
			
			tg3_buffer_view buffer_view = _model.buffer_views[accessor.buffer_view];
			tg3_buffer buffer = _model.buffers[buffer_view.buffer];
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
						THROW_INVALID_ARGUMENT("Invalid component type for texcoord0 : " + std::to_string(accessor.component_type))
					}
				}
			}
		}
	}

	//Material
	out.material_index = primitive.material;

	return out;
}
