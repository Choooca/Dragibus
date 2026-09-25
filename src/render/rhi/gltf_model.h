#pragma once

#include <string>
#include <vector>

#include <tiny_gltf_v3.h>

#include <render/primitives.h>

class GLTFModel {
public:
	GLTFModel(const std::string &model_name);
	~GLTFModel();

private :

	tg3_parse_options m_options;
	tg3_error_stack m_errors;
	tg3_model _model;

	std::vector<class Mesh> _meshes;
	Mesh ParseMesh(const tg3_mesh &mesh);

	Primitive ParsePrimitive(const tg3_primitive &primitive);
	
	template<typename T>
	T ParseByte(const uint8_t *src) {
		T dst;
		memcpy(&dst, src, sizeof(T));
		return static_cast<T>(dst);
	}

};