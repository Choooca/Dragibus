#pragma once
#include <vector>

class Scene
{
public:

	Scene();
	~Scene();

	const std::vector<struct Mesh>& GetMeshes() const;
	void SetMeshes(std::vector<struct Mesh>&& meshes);

private:

	std::vector<struct Mesh> _meshes;
};