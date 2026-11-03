#include "scene.h"

#include <render/primitives.h>

Scene::Scene() = default;
Scene::~Scene() = default;

const std::vector<Render::Mesh>& Scene::GetMeshes() const {
	return _meshes;
}

void Scene::SetMeshes(std::vector<Render::Mesh>&& meshes) {
	_meshes = std::move(meshes);
}