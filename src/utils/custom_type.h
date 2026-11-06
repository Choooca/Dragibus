#pragma once
#include <optional>
#include <glm/glm.hpp>

struct TransformationMatrix {
	glm::vec3 model;
	glm::vec3 view;
	glm::vec3 proj;
};