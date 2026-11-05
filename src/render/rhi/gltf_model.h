#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include <tiny_gltf_v3.h>

namespace Render {
	struct Primitive;
	struct Mesh;
	struct Material;
	struct Texture;
	struct Sampler;
	struct Image;
	struct Model;
}

namespace Render::GLTF {
	
	Render::Model ParseGLTFModel(const std::string& model_name);

	Mesh ParseMesh(const tg3_mesh& mesh, const tg3_model &model);
	Material ParseMaterial(const tg3_material& material);
	Texture ParseTexture(const tg3_texture& texture);
	Sampler ParseSampler(const tg3_sampler& sampler);
	Image ParseImage(const tg3_image& image, const tg3_model& model, const std::filesystem::path& base_dir);
	Primitive ParsePrimitive(const tg3_primitive& primitive, const tg3_model& model);

	template<typename T>
	T ParseByte(const uint8_t* src) {
		T dst;
		memcpy(&dst, src, sizeof(T));
		return static_cast<T>(dst);
	}
};