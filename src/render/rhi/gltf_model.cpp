#include "gltf_model.h"

#include <sstream>

#include <utils/build_macro.h>
#include <utils/debug_macro.h>

#pragma region CTors/Dtors

GLTFModel::GLTFModel(const std::string& model_name) {

	const std::string& model_path = std::string(MODELS_DIR) + model_name;

	tg3_parse_options_init(&m_options);
	tg3_error_stack_init(&m_errors);
	tg3_error_code err = tg3_parse_file(&_model, &m_errors, model_path.data(), model_path.length(), &m_options);
	if (err != TG3_OK) {

		std::stringstream msg;

		for (uint32_t i = 0; i < m_errors.count; i++) {
			msg << (int)m_errors.entries[i].severity << " " << (m_errors.entries[i].message ? m_errors.entries[i].message : "(null)") << "\n";
		}

		PRINT_RUNTIME_ERROR(msg.str());
	}

	_meshes.resize(_model.meshes_count);
	for (uint32_t i = 0; i < _model.meshes_count; ++i) {
		_meshes[i] = ParseMesh(_model.meshes[i]);
	}

}

GLTFModel::~GLTFModel() {
	tg3_model_free(&_model);
	tg3_error_stack_free(&m_errors);
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

	return out;
}
