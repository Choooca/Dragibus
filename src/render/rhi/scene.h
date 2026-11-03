#pragma once
#include <vector>

namespace Render {
	struct Mesh;
}

class Scene
{
public:

	Scene();
	~Scene();

	const std::vector<Render::Mesh>& GetMeshes() const;
	void SetMeshes(std::vector<Render::Mesh>&& meshes);

private:

	std::vector<Render::Mesh> _meshes;
};