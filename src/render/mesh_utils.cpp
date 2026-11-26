#include "mesh_utils.h"

#include <meshoptimizer.h>

#include <render/primitives.h>


std::vector<Render::Vertex> Render::MeshUtils::Unindex(const Render::Primitive& primitive)
{
    if (primitive.indices.empty())
        return primitive.vertices;

    std::vector<Render::Vertex> out;
    out.reserve(primitive.indices.size());
    for (uint32_t index : primitive.indices) {
        out.push_back(primitive.vertices[index]);
    }

    return out;
}

void Render::MeshUtils::Reindex(Primitive& primitive, const std::vector<Render::Vertex> &corners)
{
	size_t corner_count = corners.size();
	std::vector<uint32_t> remap(corner_count);
	size_t unique = meshopt_generateVertexRemap(remap.data(), nullptr, corner_count, corners.data(), corner_count, sizeof(Render::Vertex));

	primitive.indices.resize(corner_count);
	meshopt_remapIndexBuffer(primitive.indices.data(), nullptr, corner_count, remap.data());
	primitive.vertices.resize(unique);
	meshopt_remapVertexBuffer(primitive.vertices.data(), corners.data(), corner_count, sizeof(Render::Vertex), remap.data());
}

void Render::MeshUtils::GenerateFlatNormals(Render::Primitive& primitive)
{
	std::vector<Render::Vertex> corners = Unindex(primitive);

	for (size_t i = 0; i + 2 < corners.size(); i += 3) {
		glm::vec3 e1 = corners[i + 1].position - corners[i].position;
		glm::vec3 e2 = corners[i + 2].position - corners[i].position;
		glm::vec3 normal = glm::cross(e1, e2);
		float length = glm::length(normal);
		normal = length > 1e-12f ? normal / length : glm::vec3(0, 0, 1);
		corners[i].normal = corners[i + 1].normal = corners[i + 2].normal = normal;
	}

	Reindex(primitive, corners);
}
