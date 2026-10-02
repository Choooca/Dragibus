#pragma once

#include <string>
#include <vector>

#include <tiny_gltf_v3.h>

struct Primitive;
struct Mesh;
struct Material;
struct Texture;
struct Sampler;
struct Image;

class GLTFModel {
public:
	GLTFModel(const std::string &model_name);
	~GLTFModel();

	std::vector<Mesh> &&GetMeshes(); 

private :

	tg3_parse_options _options;
	tg3_error_stack _errors;
	tg3_model _model;

	std::vector<Mesh> _meshes;
	Mesh ParseMesh(const tg3_mesh &mesh);

	std::vector<Material> _materials;
	Material ParseMaterial(const tg3_material& material);

	std::vector<Texture> _textures;
	Texture ParseTexture(const tg3_texture& texture);

	std::vector<Sampler> _samplers;
	Sampler ParseSampler(const tg3_sampler& sampler);

	std::vector<Image> _images;
	Image ParseImage(const tg3_image& image);

	Primitive ParsePrimitive(const tg3_primitive &primitive);
	
	template<typename T>
	T ParseByte(const uint8_t *src) {
		T dst;
		memcpy(&dst, src, sizeof(T));
		return static_cast<T>(dst);
	}

};