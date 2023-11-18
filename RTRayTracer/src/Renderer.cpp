#include "Renderer.h"
#include "Walnut/Random.h"

#include <execution>

namespace Utils {
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
	
	static uint32_t PCG_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)std::numeric_limits<uint32_t>::max();
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(RandomFloat(seed) * 2.0f - 1.0f,
										RandomFloat(seed) * 2.0f - 1.0f,
										RandomFloat(seed) * 2.0f - 1.0f));
	}

	static glm::vec3 refract(glm::vec3 rayDirection, glm::vec3 normal, float ior)
	{
		ior = 2.0f - ior;
		float cosi = glm::dot(normal, rayDirection);
		glm::vec3 o = (rayDirection * ior - normal * (-cosi + ior * cosi));
		return o;
	}
}

int Renderer::GetFrameIndex() 
{
	return m_FrameIndex;
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizonntalIterator.resize(width);
	m_ImageVerticalIterator.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizonntalIterator[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIterator[i] = i;
}

void Renderer::ResetImage(uint32_t width, uint32_t height)
{
	m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizonntalIterator.resize(width);
	m_ImageVerticalIterator.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizonntalIterator[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIterator[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	const glm::vec3& rayOrigin = camera.GetPosition();

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

#define MultiThread 1
#if MultiThread

	std::for_each(std::execution::par, m_ImageVerticalIterator.begin(), m_ImageVerticalIterator.end(),
		[this](uint32_t y)
		{
		/*
			std::for_each(std::execution::par, m_ImageHorizonntalIterator.begin(), m_ImageHorizonntalIterator.end(),
			[this, y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] = m_AccumulationData[x + y * m_FinalImage->GetWidth()] + color;

					glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColor = accumulatedColor / (float)m_FrameIndex;

					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
				});
		*/
			for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
			{
				glm::vec4 color = PerPixel(x, y);
				m_AccumulationData[x + y * m_FinalImage->GetWidth()] = m_AccumulationData[x + y * m_FinalImage->GetWidth()] + color;

				glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
				accumulatedColor = accumulatedColor / (float)m_FrameIndex;

				accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
				m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
			}

		});

#else

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] = m_AccumulationData[x + y * m_FinalImage->GetWidth()] + color;

			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor = accumulatedColor / (float)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
		}
	}
	

#endif

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];
	ray.Direction = ray.Direction + (Utils::InUnitSphere(seed) * m_Settings.AntiAliasingAmount);

	glm::vec3 light(0.0f);
	glm::vec3 contribution(1.0f);

	int bounces = m_Settings.LightBounces;
	for (int i = 0; i < bounces; i++) {

		seed += i;
		
		Renderer::HitPayload payload = TraceRay(ray);	
		if (payload.HitDistance < 0.0f)
		{
			light += m_ActiveScene->skyColor * contribution;
			break;
		}

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		contribution *= material.Albedo;
		light += material.GetEmission();

		if (!material.isGlass)
		{
			ray.Origin = payload.WorldPosition + (payload.WorldNormal * glm::vec3(0.0001f));
			if (m_Settings.SlowRandom)
				ray.Direction = glm::normalize(payload.WorldNormal + (Walnut::Random::InUnitSphere() * material.Roughness));
			else
				ray.Direction = glm::normalize(glm::reflect(ray.Direction, payload.WorldNormal) + (Utils::InUnitSphere(seed) * material.Roughness));
		}
		else
		{
			ray.Direction = Utils::refract(ray.Direction, payload.WorldNormal, material.IOR) + (Utils::InUnitSphere(seed) * material.Roughness);
			ray.Origin = payload.WorldPosition + ray.Direction * 1.0001f;
		}

	}

	return glm::vec4(light, 1.0f);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex, float exitDistance)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ExitDistance = exitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{

	int closestSphere = -1  ;
	float hitDistance = std::numeric_limits<float>::max();
	float exitDistance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			continue;

		float closestT = (-b - glm::sqrt(discriminant)) / (2.0 * a);
		float t0 = (-b + glm::sqrt(discriminant)) / (2.0 * a);
		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			exitDistance = t0;
			closestSphere = (int)i;
		}
	}

	if (closestSphere < 0) return Miss(ray);
		
	return ClosestHit(ray, hitDistance, closestSphere, exitDistance);
}
