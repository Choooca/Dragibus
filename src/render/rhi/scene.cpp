#include "scene.h"

#include <render/primitives.h>

Scene::Scene() = default;
Scene::~Scene() = default;

void Scene::AddModel(Render::Model&& model) { _models.push_back(std::move(model)); }

const std::vector<Render::Model>& Scene::GetModels() const { return _models; }
