#ifndef RENDERER_H

#define RENDERER_H

#include "Walnut/Image.h"
#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include <memory>
#include <glm/glm.hpp>

class Renderer
{

public:
	struct Settings 
	{
		bool DisplayNormals = false;
		
		bool Accumulate = true;
		bool SlowRandom = false;

		int LightBounces = 5;
		int RaysPerPixel = 1;
		float AntiAliasingAmount = 0.001f;
	};
public:
	Renderer() = default;

	void ResetImage(uint32_t width, uint32_t height);
	void OnResize(uint32_t width, uint32_t height);
	void Render(const Scene& scene, const Camera& camera);
	int GetFrameIndex();

	uint32_t GetPixelAt(int x, int y);
	bool DoesImageExist();
	
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_Settings; }
private:
	struct HitInfo
	{
		float HitDistance;
		float ExitDistance;
		glm::vec3 HitPosition;
		glm::vec3 HitNormal;

		int ObjectIndex;
	};

	glm::vec4 PerPixel(Ray ray, uint32_t seed, uint32_t x, uint32_t y); // RayGen
	
	HitInfo TraceRay(const Ray& ray);
	HitInfo Miss(const Ray& ray);
	Renderer::HitInfo ClosestHit(const Ray& ray, float hitDistance, float exitDistance, int objectIndex);

private:
	std::shared_ptr<Walnut::Image> m_FinalImage;

	Settings m_Settings;
	
	std::vector<uint32_t> m_ImageHorizonntalIterator, m_ImageVerticalIterator;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	bool m_HasImageData = false;

	uint32_t* m_ImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;

	uint32_t m_FrameIndex = 1;
};

#endif // !RENDERER_H