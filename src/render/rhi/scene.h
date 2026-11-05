#pragma once
#include <vector>

namespace Render {
	struct Model;
}

class Scene
{
public:

	Scene();
	~Scene();

	void AddModel(Render::Model&& model);
	const std::vector<Render::Model>& GetModels() const;

private:

	std::vector<Render::Model> _models;
};