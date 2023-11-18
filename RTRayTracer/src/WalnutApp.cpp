#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "Renderer.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Walnut;



class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() :
		m_Camera(45.0f, 0.1f, 100.0f)
	{
			Material& defaultMaterial = m_Scene.Materials.emplace_back();
			defaultMaterial.Albedo = { 0.5f, 0.5f, 0.5f };
			defaultMaterial.Roughness = 0.5f;

			Material& defaultMaterial2 = m_Scene.Materials.emplace_back();
			defaultMaterial2.Albedo = { 0.7f, 0.7f, 0.7f };
			defaultMaterial2.Roughness = 1.0f;

			Sphere* sphere = new Sphere();
			sphere->Position = { 0.0f, 2.0f, 0.0f };
			sphere->Radius = 1.0f;
			sphere->MaterialIndex = 1;
			m_Scene.SceneObjects.push_back(sphere);

			Cube* floor = new Cube();
			floor->Position = { 0.0f, -5.0f, 0.0f };
			floor->Dimensions = glm::vec3(1000.0f, 0.01f, 1000.0f);
			floor->MaterialIndex = 0;
			m_Scene.SceneObjects.push_back(floor);

			m_Scene.skyColor = glm::vec3(0.75f, 0.075f, 0.75f);
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_Camera.OnUpdate(ts))
		{
			m_Renderer.ResetFrameIndex();
		}
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		{
			ImGui::Text("Rendertime: %.3fms Samples Rendered: %d", m_LastRenderTime, m_Renderer.GetFrameIndex()-1);

			ImGui::Text("Render Settings");

			ImGui::Checkbox("Realtime", &m_IsRealTime);

			ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
			ImGui::Checkbox("Slow Random", &m_Renderer.GetSettings().SlowRandom);

			ImGui::SliderInt("Light Bounces", &m_Renderer.GetSettings().LightBounces, 0, 250);
			ImGui::DragFloat("Anti Alias Radius", &m_Renderer.GetSettings().AntiAliasingAmount, 0.01f, 0, 50);
			
			if(!m_IsRealTime)
				ImGui::DragInt("Samples", &m_Samples);


			ImGui::Text("Debug Settings");
			ImGui::Checkbox("Only Indirect Lighting", &m_Renderer.GetSettings().OnlyIndirect);
			ImGui::Checkbox("Display Surface Normals", &m_Renderer.GetSettings().DisplayNormals);

			if (!m_IsRealTime) {
				if (ImGui::Button("Render")) {
					Timer time;

					m_Renderer.ResetFrameIndex();
					for (int samples = 0; samples < m_Samples; samples++) 
					{
						Render();
					}

					m_LastRenderTime = time.ElapsedMillis();
				}
			}

			if (ImGui::Button("Reset")) {
				m_Renderer.ResetFrameIndex();
				m_Renderer.ResetImage(m_ViewportWidth, m_ViewportHeight);
			}
		}
		ImGui::End();

		ImGui::Begin("Materials");
		{
			ImGui::Text("Scene Settings");
			ImGui::ColorEdit3("Sky Color", glm::value_ptr(m_Scene.skyColor));
			ImGui::Separator();

			// Material list

			for (size_t i = 0; i < m_Scene.Materials.size(); i++)
			{

				ImGui::PushID(i);

				Material& material = m_Scene.Materials[i];
				ImGui::Text("Material Settings: ");
				ImGui::ColorEdit3("Color", glm::value_ptr(material.Albedo));
				ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Metallic", &material.Metallic, 0.01f, 0.0f, 1.0f);

				ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
				ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.01f, 0.0f, FLT_MAX);

				ImGui::Checkbox("Is Glass", &material.isGlass);
				ImGui::SliderFloat("Index Of Refraction", &material.IOR, 0.0f, 2.5f);

				ImGui::Separator();

				ImGui::PopID();
			}

			// Add new material

			if (ImGui::Button("Add new Material"))
			{
				Material& newMaterial = m_Scene.Materials.emplace_back();
				newMaterial.Albedo = { 1.0f, 1.0f, 1.0f };
				newMaterial.Roughness = 1.0f;
			}
		}
		ImGui::End();

		ImGui::Begin("Spheres");
		{
			// Object list

			// SPHERES

			for (size_t i = 0; i < m_Scene.SceneObjects.size(); i++)
			{
				ImGui::PushID(i);

				RTObject* rtobject = m_Scene.SceneObjects[i];
				
				if (Cube* cube = dynamic_cast<Cube*>(rtobject))
				{
					ImGui::Text("[Object %d] General Settings: ", i);
					ImGui::DragFloat3("Position", glm::value_ptr(cube->Position), 0.01f);
					ImGui::DragInt("Material Index", &cube->MaterialIndex, 1.0f, 0.0, (int)m_Scene.Materials.size() - 1);

					ImGui::Text("[Object %d] Object specific Settings: ", i);
					ImGui::DragFloat3("Dimensions", glm::value_ptr(cube->Dimensions), 0.01f);
				}
				else if (Sphere* sphere = dynamic_cast<Sphere*>(rtobject))
				{
					ImGui::Text("[Object %d] General Settings: ", i);
					ImGui::DragFloat3("Position", glm::value_ptr(sphere->Position), 0.01f);
					ImGui::DragInt("Material Index", &sphere->MaterialIndex, 1.0f, 0.0, (int)m_Scene.Materials.size() - 1);

					ImGui::Text("[Object %d] Object specific Settings: ", i);
					ImGui::DragFloat("Radius", &sphere->Radius, 0.01f);
				}

				ImGui::Separator();

				ImGui::PopID();
			}

			// Add new object

			if (ImGui::Button("Add new Object"))
			{
				Sphere sphere;
				sphere.Position = { 0.0f, 0.0f, 0.0f };
				sphere.Radius = 1.0f;
				sphere.MaterialIndex = 0;
				m_Scene.SceneObjects.push_back(&sphere);
			}
		}
		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		{
			m_ViewportWidth = ImGui::GetContentRegionAvail().x;
			m_ViewportHeight = ImGui::GetContentRegionAvail().y;

			auto image = m_Renderer.GetFinalImage();
			if (image)
				ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() },
					ImVec2(0, 1), ImVec2(1, 0));
		
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if(m_IsRealTime)
		{
			if (ImGui::IsAnyMouseDown())
				m_Renderer.ResetFrameIndex();
		}

		if (m_IsRealTime) {
			Render(); // UNCOMMENT FOR RT-RAYTRACING
		}
	}

	void Render() 
	{
		Timer time;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		if(m_IsRealTime)
			m_LastRenderTime = time.ElapsedMillis();
	}

private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	float m_LastRenderTime = 0.0f;
	int m_Samples = 10;
	bool m_IsRealTime = false;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}