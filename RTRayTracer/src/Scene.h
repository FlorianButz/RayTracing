#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Object.h"

struct Material
{
	glm::vec3 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;

	glm::vec3 EmissionColor{ 0.0f };
	float EmissionPower = 0.0f;

	glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; }

	bool isGlass;
	float IOR = 1.0f;
};

struct Scene
{
	std::vector<RTObject*> SceneObjects;
	std::vector<Material> Materials;

	glm::vec3 skyColor = glm::vec3(0.0f);
};