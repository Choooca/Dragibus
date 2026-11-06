#pragma once

#include <array>
#include <string>

#include <glm/glm.hpp>
#include <glfw/glfw3.h>


namespace Render {

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 perspective;
		alignas(16) glm::mat4 normal;
	};

#pragma region  Model

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 tangent;
		glm::vec2 tex_coord0;
	};

	enum DRAW_MODE {
		POINTS,
		LINES,
		LINE_LOOP,
		LINE_STRIP,
		TRIANGLES,
		TRIANGLE_STRIP,
		TRIANGLE_FAN
	};

	struct Sampler {

		enum FilterMode : int32_t
		{
			NEAREST = 9728,
			LINEAR = 9729,
			NEAREST_MIPMAP_NEAREST = 9984,
			LINEAR_MIPMAP_NEAREST = 9985,
			NEAREST_MIPMAP_LINEAR = 9986,
			LINEAR_MIPMAP_LINEAR = 9987
		};

		enum WrapMode : int32_t
		{
			REPEAT = 10497,
			CLAMP_TO_EDGE = 33071,
			MIRRORED_REPEAT = 33648
		};

		std::string name;
		FilterMode mag_filter;
		FilterMode min_filter;
		WrapMode wrap_s;
		WrapMode wrap_t;

	};


	struct Texture {
		std::string name;
		int sampler_index;
		int image_index;
	};

	struct Image {
		std::string name;
		std::vector<uint8_t> pixels;
		int32_t width, height;
		int32_t channels;
	};

	struct Material {
		enum AlphaMode {
			OPAQUE,
			MASK,
			BLEND
		};

		std::string name;

		bool double_sided = false;
		glm::vec3 emissive = glm::vec3(0);

		AlphaMode alpha_mode;
		double alpha_cutoff = 0.5f;

		int emmisive_texture_index;
		int emmisive_texture_texcoord;

		int normal_texture_index;
		int normal_texture_texcoord;

		int occlusion_texture_index;
		int occlusion_texture_texcoord;

		glm::vec<4, double> base_color = glm::vec4(1);
		int base_color_texture_index;
		int base_color_texture_texcoord;

		double metallic_factor = 1;
		double roughness_factor = 1;
		int metallic_roughness_texture_index;
		int metallic_roughness_texture_texcoord;
	};


	struct Primitive {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		int material_index;

		DRAW_MODE mode;
	};

	struct Mesh {
		std::string name;

		std::vector<Primitive> primitives;
	};

	struct Model {
		std::string name;
		
		std::vector<Mesh> meshes;
		std::vector<Material> materials;
		std::vector<Texture> textures;
		std::vector<Sampler> samplers;
		std::vector<Image> images;


	};

#pragma endregion

}