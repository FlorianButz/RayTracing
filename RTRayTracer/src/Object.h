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

	virtual glm::vec3& GetPosition()
	{
		return Position;
	}

	virtual int& GetMaterialIndex()
	{
		return MaterialIndex;
	}

	glm::vec3 Position = glm::vec3(0.0f);
	int MaterialIndex = 0;
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

	glm::vec3& GetPosition() override
	{
		return Position;
	}

	int& GetMaterialIndex() override
	{
		return MaterialIndex;
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
		glm::vec3 invScale = 1.0f / Dimensions;

		position *= invScale; // Apply inverse scaling to the position

		glm::vec3 absPos = glm::abs(position);
		float maxComponent = glm::max(glm::max(absPos.x, absPos.y), absPos.z);

		glm::vec3 boxNormal;

		if (maxComponent == absPos.x) {
			boxNormal = glm::vec3(position.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
		}
		else if (maxComponent == absPos.y) {
			boxNormal = glm::vec3(0.0f, position.y < 0.0f ? -1.0f : 1.0f, 0.0f);
		}
		else {
			boxNormal = glm::vec3(0.0f, 0.0f, position.z < 0.0f ? -1.0f : 1.0f);
		}

		return glm::normalize(boxNormal);
	}

	glm::vec3& GetPosition() override
	{
		return Position;
	}

	int& GetMaterialIndex() override
	{
		return MaterialIndex;
	}

	glm::vec3 Position = glm::vec3(0.0f);
	int MaterialIndex = 0;
	glm::vec3 Dimensions = glm::vec3(1.0f);
};