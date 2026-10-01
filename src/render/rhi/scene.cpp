#include "scene.h"

#include <render/primitives.h>

Scene::Scene() = default;
Scene::~Scene() = default;

const std::vector<Mesh>& Scene::GetMeshes() const {
	return _meshes;
}

void Scene::SetMeshes(std::vector<Mesh>&& meshes) {
	_meshes = std::move(meshes);
}