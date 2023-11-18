#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

#include <iostream>

class RTObject
{
public:
	virtual glm::vec2 Intersection(const Ray& ray) {
		return glm::vec2(0.0f);
	}
	virtual glm::vec3 Normal(glm::vec3 position){
		return glm::vec3(0.0f);
	}

	glm::vec3 Position;
	int MaterialIndex;

	virtual ~RTObject() { }
};

class Sphere : public RTObject
{
public:
	glm::vec2 Intersection(const Ray& ray) override
	{
		glm::vec3 origin = ray.Origin - Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - Radius * Radius;

		float discriminant = b * b - 4.0f * a * c;
		
		if (discriminant < 0.0f) {
			return glm::vec2(INT16_MIN);
		}

		float closestT = (-b - glm::sqrt(discriminant)) / (2.0 * a);
		float t0 = (-b + glm::sqrt(discriminant)) / (2.0 * a);

		return glm::vec2(closestT, t0);

	};

	glm::vec3 Normal(glm::vec3 position) override
	{
		return glm::normalize(position);
	}

	glm::vec3 Position = glm::vec3(0.0f);
	int MaterialIndex = 0;

	float Radius = 1.0f;
};

class Cube : public RTObject
{
public:
	glm::vec2 Intersection(const Ray& ray) override
	{
		glm::vec3 origin = ray.Origin - Position;

		float tx1, tx2, ty1, ty2, tz1, tz2, closestT, t0;
		tx1 = (-Dimensions.x - origin.x) / ray.Direction.x;
		tx2 = ( Dimensions.x - origin.x) / ray.Direction.x;
		ty1 = (-Dimensions.y - origin.y) / ray.Direction.y;
		ty2 = ( Dimensions.y - origin.y) / ray.Direction.y;
		tz1 = (-Dimensions.z - origin.z) / ray.Direction.z;
		tz2 = ( Dimensions.z - origin.z) / ray.Direction.z;

		closestT = std::max(std::min(tx1, tx2), std::max(std::min(ty1, ty2), std::min(tz1, tz2)));
		t0 = std::min(std::max(tx1, tx2), std::min(std::max(ty1, ty2), std::max(tz1, tz2)));

		if (closestT > t0 || t0 < 0) {
			return glm::vec2(-1.0f);
		}
		return glm::vec2(closestT, t0);
	};

	glm::vec3 Normal(glm::vec3 position) override
	{
		glm::vec3 vMax = glm::vec3(Dimensions.x, Dimensions.y, Dimensions.z);
		glm::vec3 vMin = glm::vec3(-Dimensions.x, -Dimensions.y, -Dimensions.z);
		glm::vec3 center = (vMax + vMin) * 0.5f;
		glm::vec3 n = glm::vec3(position.x - center.x, position.y - center.y, position.z - center.z);
		return glm::normalize(n);
	}

	glm::vec3 Position = glm::vec3(0.0f);
	int MaterialIndex = 0;

	glm::vec3 Dimensions = glm::vec3(1.0f);
};