#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Object.h"

struct Material
{
	glm::vec3 Color{ 0.75f };
	glm::vec3 SpecularColor{ 1.0f };

	float Smoothness = 0.15f;
	float Metallness = 0.1f;

	glm::vec3 EmissionColor{ 0.0f };
	float EmissionPower = 0.0f;

	glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; }

	float Transmission = 0.0f;
	float IOR = 1.0f;

	bool Checker = false;
	float CheckerScale = 1.0f;
};

struct Scene
{
	std::vector<RTObject*> SceneObjects = std::vector<RTObject*>();
	std::vector<Material> Materials = std::vector<Material>();

	glm::vec3 SkyColor = glm::vec3(0.0f);
};