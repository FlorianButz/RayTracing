#include "Renderer.h"
#include "Walnut/Random.h"

#include "Object.h"

#include <execution>
#include <cmath>

# define M_PI           3.14159265358979323846

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
		float toReturn = (float)seed / (float)std::numeric_limits<uint32_t>::max();
		return toReturn;
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(RandomFloat(seed) * 2.0f - 1.0f,
										RandomFloat(seed) * 2.0f - 1.0f,
										RandomFloat(seed) * 2.0f - 1.0f));
	}

	static glm::vec3 Refract(glm::vec3 rayDirection, glm::vec3 normal, float ior)
	{
		ior = 2.0f - ior;
		float cosi = glm::dot(normal, rayDirection);
		glm::vec3 o = (rayDirection * ior - normal * (-cosi + ior * cosi));
		return o;
	}

	static float Lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	static glm::vec3 Lerp3(const glm::vec3& a, const glm::vec3& b, float t) {
		t = glm::clamp(t, 0.0f, 1.0f); // Ensure t is clamped between 0 and 1
		return a + t * (b - a);
	}

	static glm::vec2 RandomPointInCircle(uint32_t seed)
	{
		float angle = RandomFloat(seed) * 2 * M_PI;
		glm::vec2 pointOnCircle = glm::vec2(cos(angle), sin(angle));
		return pointOnCircle * sqrt(RandomFloat(seed));
	}
}

int Renderer::GetFrameIndex() 
{
	return m_FrameIndex;
}

uint32_t Renderer::GetPixelAt(int x, int y)
{
	return m_ImageData[x + y * m_FinalImage->GetWidth()];
}

bool Renderer::DoesImageExist()
{
	return m_HasImageData;
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

	m_HasImageData = false;
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

	m_HasImageData = false;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_HasImageData = true;

	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	const glm::vec3& rayOrigin = camera.GetPosition();

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

	std::for_each(std::execution::par, m_ImageVerticalIterator.begin(), m_ImageVerticalIterator.end(),
		[this](uint32_t y)
		{
			for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
			{
				glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				for (int pixelRay = 0; pixelRay < m_Settings.RaysPerPixel; pixelRay++)
				{
					float imgWidth = m_FinalImage->GetWidth();
					float imgHeight = m_FinalImage->GetHeight();

					uint32_t seed = x + y * m_FinalImage->GetWidth();
					seed *= m_FrameIndex * (pixelRay * pixelRay + 293123);

					Ray ray;
					ray.Origin = m_ActiveCamera->GetPosition();
					ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()] + (Utils::InUnitSphere(seed) * m_Settings.AntiAliasingAmount);

					//color += PerPixel(ray, seed, x, y);
					color += PerPixel(ray, seed, x, y);
				}
				color /= m_Settings.RaysPerPixel;
				color.a = 1.0f;

				m_AccumulationData[x + y * m_FinalImage->GetWidth()] = m_AccumulationData[x + y * m_FinalImage->GetWidth()] + color;

				glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
				accumulatedColor = accumulatedColor / (float)m_FrameIndex;

				accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
				m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
			}

		});


	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(Ray ray, uint32_t seed, uint32_t x, uint32_t y)
{
	glm::vec3 incomingLight = glm::vec3(0.0f);
	glm::vec3 rayColor = glm::vec3(1.0f);
	
	for (int i = 0; i <= m_Settings.LightBounces; i++)
	{
		Renderer::HitInfo hitInfo = TraceRay(ray);

		if (hitInfo.HitDistance > 0.0f)
		{
			if (m_Settings.DisplayNormals)
				return glm::vec4(hitInfo.HitNormal, 1.0f);

			const Material& material = m_ActiveScene->Materials[m_ActiveScene->SceneObjects[hitInfo.ObjectIndex]->GetMaterialIndex()];

			glm::vec3 materialColor = material.Color;

			ray.Origin = hitInfo.HitPosition;

			glm::vec3 difuseDir = glm::normalize(hitInfo.HitNormal + Utils::InUnitSphere(seed));
			glm::vec3 specularDir = reflect(ray.Direction, hitInfo.HitNormal);
			
			bool isRefractiveBounce = material.Transmission >= Utils::RandomFloat(seed);
			bool isSpecularBounce = material.Metallness >= Utils::RandomFloat(seed);

			ray.Direction = Utils::Lerp3(glm::normalize(Utils::Lerp3(difuseDir, specularDir, material.Smoothness * isSpecularBounce)),
										glm::normalize(Utils::Lerp3(
											Utils::Refract(ray.Direction + Utils::InUnitSphere(seed), hitInfo.HitNormal, material.IOR),
											Utils::Refract(ray.Direction, hitInfo.HitNormal, material.IOR),
											material.Smoothness)), isRefractiveBounce);
			
			glm::vec3 emittedLight = material.EmissionColor * material.EmissionPower;
			incomingLight += emittedLight * rayColor;
			//rayColor *= Utils::Lerp3(material.Color, material.SpecularColor, isSpecularBounce);
			rayColor *= materialColor;

			// Early exit if ray is too weak
			float p = glm::max(rayColor.r, glm::max(rayColor.g, rayColor.b));
			if (Utils::RandomFloat(seed) >= p) {
				break;
			}
			rayColor *= 1.0f / p;
		}
		else 
		{
			incomingLight += m_ActiveScene->SkyColor * rayColor;
			break;
		}
	}

	return glm::vec4(incomingLight, 1.0f); 
}

Renderer::HitInfo Renderer::Miss(const Ray& ray)
{
	Renderer::HitInfo payload;
	payload.HitDistance = -1.0f;
	return payload;
}

Renderer::HitInfo Renderer::TraceRay(const Ray& ray)
{
	Renderer::HitInfo payload;

	int closestObj = -1;

	float hitDistance = std::numeric_limits<float>::max();
	float exitDistance = std::numeric_limits<float>::max();
	

	for (size_t i = 0; i < m_ActiveScene->SceneObjects.size(); i++)
	{
		if (m_ActiveScene->SceneObjects[i] == nullptr) continue;

		glm::vec2 data = m_ActiveScene->SceneObjects[i]->Intersection(ray);

		float closestT = data.x;
		float t0 = data.y;

		if (closestT == INT16_MIN)
			continue;

		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			exitDistance = t0;
			closestObj = (int)i;
		}
	}

	if (closestObj < 0) return Miss(ray);

	return ClosestHit(ray, hitDistance, exitDistance, closestObj);
}

Renderer::HitInfo Renderer::ClosestHit(const Ray& ray, float hitDistance, float exitDistane, int objectIndex)
{
	Renderer::HitInfo payload;
	payload.HitDistance = hitDistance;
	payload.ExitDistance = exitDistane;
	payload.ObjectIndex = objectIndex;

	RTObject* closestObj = m_ActiveScene->SceneObjects[objectIndex];

	glm::vec3 origin = ray.Origin - m_ActiveScene->SceneObjects[objectIndex]->GetPosition();
	payload.HitPosition = origin + ray.Direction * hitDistance;
	payload.HitNormal = m_ActiveScene->SceneObjects[objectIndex]->Normal(payload.HitPosition);

	payload.HitPosition += m_ActiveScene->SceneObjects[objectIndex]->GetPosition();

	return payload;
}