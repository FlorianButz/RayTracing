#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "Walnut/Input/Input.h"

#include "Renderer.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>

#include "lodepng.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

using namespace Walnut;

struct Color {
	uint8_t r, g, b, a;
};

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() :
		m_Camera(45.0f, 0.1f, 100.0f)
	{
			Material& defaultMaterial = m_Scene.Materials.emplace_back();
			defaultMaterial.Color = { 0.5f, 0.5f, 0.5f };
			defaultMaterial.Smoothness = 0.05f;

			Material& defaultMaterial2 = m_Scene.Materials.emplace_back();
			defaultMaterial2.Color = { 0.7f, 0.7f, 0.7f };
			defaultMaterial2.Smoothness = 0.95f;

			Material& defaultMaterial3 = m_Scene.Materials.emplace_back();
			defaultMaterial3.Color = { 0.7f, 0.7f, 0.7f };
			defaultMaterial3.Smoothness = 0.0f;
			defaultMaterial3.EmissionColor = glm::vec3(1.0f);
			defaultMaterial3.EmissionPower = 2.0f;

			Sphere* sphere = new Sphere();
			sphere->Position = { -1.5f, 0.0f, 0.0f };
			sphere->Radius = 1.0f;
			sphere->MaterialIndex = 1;
			m_Scene.SceneObjects.push_back(sphere);

			Sphere* sphere2 = new Sphere();
			sphere2->Position = { 1.5f, 0.0f, 0.0f };
			sphere2->Radius = 1.0f;
			sphere2->MaterialIndex = 2;
			m_Scene.SceneObjects.push_back(sphere2);

			Cube* floor = new Cube();
			floor->Position = { 0.0f, -1.0f, 0.0f };
			floor->Dimensions = glm::vec3(1000.0f, 0.01f, 1000.0f);
			floor->MaterialIndex = 0;
			m_Scene.SceneObjects.push_back(floor);

			m_Scene.SkyColor = glm::vec3(0.175f, 0.175f, 0.175f);
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_Camera.OnUpdate(ts))
		{
			m_Renderer.ResetFrameIndex();
		}
	}

	Color ConvertFromRGBA(uint32_t colorData)
	{
		Color color;

		color.r = colorData & 0xFF;           // Extract red component
		color.g = (colorData >> 8) & 0xFF;   // Extract green component
		color.b = (colorData >> 16) & 0xFF;  // Extract blue component
		color.a = (colorData >> 24) & 0xFF;  // Extract alpha component

		return color;
	}

	void SaveImageFile() {
		int imageSize = m_Renderer.GetFinalImage()->GetWidth() * m_Renderer.GetFinalImage()->GetHeight() * 4;

		std::vector<std::uint8_t> PngBuffer(imageSize);

		for (std::int32_t I = 0; I < m_Renderer.GetFinalImage()->GetHeight(); ++I) {
			for (std::int32_t J = 0; J < m_Renderer.GetFinalImage()->GetWidth(); ++J) {
				std::size_t NewPos = (I * m_Renderer.GetFinalImage()->GetWidth() + J) * 4;
				Color pixelColor = ConvertFromRGBA(m_Renderer.GetPixelAt(J, I));
				PngBuffer[NewPos + 0] = pixelColor.r; // B is offset 0
				PngBuffer[NewPos + 1] = pixelColor.g; // G is offset 1
				PngBuffer[NewPos + 2] = pixelColor.b; // R is offset 2
				PngBuffer[NewPos + 3] = pixelColor.a; // A is offset 3
			}
		}

		std::vector<uint8_t> FlippedBuffer(PngBuffer.size());
		int width = m_Renderer.GetFinalImage()->GetWidth();
		int height = m_Renderer.GetFinalImage()->GetHeight();

		for (int y = 0; y < height; ++y) {
			int flippedY = height - y - 1; // Calculate the flipped row position
			std::memcpy(&FlippedBuffer[flippedY * width * 4], &PngBuffer[y * width * 4], width * 4);
		}

		std::vector<std::uint8_t> ImageBuffer;
		lodepng::encode(ImageBuffer, FlippedBuffer, m_Renderer.GetFinalImage()->GetWidth(), m_Renderer.GetFinalImage()->GetHeight());

		std::string fileName = std::string(m_ImageFileName) + ".png";
		lodepng::save_file(ImageBuffer, fileName);
	}


	virtual void OnUIRender() override
	{
		ImGui::Begin("File");
		{
			ImGui::Text("Export Render: ");
			ImGui::Spacing();

			ImGui::InputText("Image File Name", m_ImageFileName, IM_ARRAYSIZE(m_ImageFileName));

			if (m_Renderer.DoesImageExist()) {
				if (ImGui::Button("Save Render"))
				{
					SaveImageFile();
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Export Scene: ");
			ImGui::Spacing();

			ImGui::InputText("Scene File Name", m_SceneFileName, IM_ARRAYSIZE(m_SceneFileName));

			if (ImGui::Button("Save Scene"))
			{
				throw "Not Implemented";
			}
			if (ImGui::Button("Load Scene"))
			{
				throw "Not Implemented";
			}
		}
		ImGui::End();

		ImGui::Begin("Settings");
		{
			ImGui::Text("Rendertime: %.3fms Samples Rendered: %d", m_LastRenderTime, m_Renderer.GetFrameIndex()-1);

			ImGui::Separator();

			ImGui::Text("Render Settings");

			ImGui::Spacing();

			ImGui::Checkbox("Realtime", &m_IsRealTime);
			ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
			ImGui::Checkbox("Slow Random", &m_Renderer.GetSettings().SlowRandom);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::SliderFloat("Resolution Scale", &m_ResolutionScale, 0.25f, 2.0f);
			ImGui::SliderInt("Light Bounces", &m_Renderer.GetSettings().LightBounces, 0, 250);
			ImGui::SliderInt("Rays Per Pixel", &m_Renderer.GetSettings().RaysPerPixel, 0, 25);
			ImGui::DragFloat("Anti Alias Radius", &m_Renderer.GetSettings().AntiAliasingAmount, 0.01f, 0, 50);

			if(!m_IsRealTime)
				ImGui::DragInt("Samples", &m_Samples);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Debug Settings");
			ImGui::Checkbox("Display Surface Normals", &m_Renderer.GetSettings().DisplayNormals);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

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
			ImGui::ColorEdit3("Sky Color", glm::value_ptr(m_Scene.SkyColor));
			
			//ImGui::SliderFloat("Defocus Strength", &m_Renderer.GetSettings().DefocusStrength, 0.0f, 500.0f);
			//ImGui::SliderFloat("Defocus Distance", &m_Renderer.GetSettings().DefocusDist, 0.0f, 150.0f);
			//ImGui::SliderFloat3("Defocus Distance", glm::value_ptr(m_Renderer.GetSettings().DefocusDist), 0.0f, 150.0f);

			ImGui::Separator();

			// Material list

			for (size_t i = 0; i < m_Scene.Materials.size(); i++)
			{
				ImGui::PushID(i);

				std::string headerName = "[Material " + std::to_string(i) + "] Settings: ";
				const char* headerNameChar = headerName.c_str();

				if (ImGui::CollapsingHeader(headerNameChar))
				{
					Material& material = m_Scene.Materials[i];
					ImGui::ColorEdit3("Color", glm::value_ptr(material.Color));
					ImGui::SliderFloat("Smoothness", &material.Smoothness, 0.0f, 1.0f);
					ImGui::SliderFloat("Metallic", &material.Metallness, 0.05f, 1.0f);

					ImGui::Separator();

					ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
					ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.01f, 0.0f, FLT_MAX);
					
					ImGui::Separator();

					ImGui::SliderFloat("Transmission Amount", &material.Transmission, 0.0f, 1.0f);
					ImGui::SliderFloat("Index Of Refraction", &material.IOR, 0.0f, 10.0f);

					ImGui::Separator();
					
					ImGui::Checkbox("Checker Texture", &material.Checker);
					ImGui::DragFloat("Checker Scale", &material.CheckerScale, 0.01f, 0.0f, FLT_MAX);

					ImGui::Separator();

					if (ImGui::Button("||"))
					{
						Material& defaultMaterial = material;
						m_Scene.Materials.emplace_back(defaultMaterial);
					}
				}

				ImGui::PopID();
			}

			// Add new material

			if (ImGui::Button("Add new Material"))
			{
				Material& newMaterial = m_Scene.Materials.emplace_back();
				newMaterial.Color = { 1.0f, 1.0f, 1.0f };
				newMaterial.Smoothness = 1.0f;
			}
		}
		ImGui::End();

		ImGui::Begin("Objects");
		{
			// Object list

			for (size_t i = 0; i < m_Scene.SceneObjects.size(); i++)
			{
				ImGui::PushID(i);

				RTObject* rtobject = m_Scene.SceneObjects[i];
				
				if (Cube* cube = dynamic_cast<Cube*>(rtobject))
				{
					ImGui::Text("[Cube %d] General Settings: ", i);
					ImGui::DragFloat3("Position", glm::value_ptr(cube->Position), 0.01f);
					ImGui::DragInt("Material Index", &cube->MaterialIndex, 1.0f, 0.0, (int)m_Scene.Materials.size() - 1);

					ImGui::Text("[Cube %d] Object specific Settings: ", i);
					ImGui::DragFloat3("Dimensions", glm::value_ptr(cube->Dimensions), 0.01f);

					if (ImGui::Button("X")) m_Scene.SceneObjects.erase(m_Scene.SceneObjects.begin() + i);
				}
				else if (Sphere* sphere = dynamic_cast<Sphere*>(rtobject))
				{
					ImGui::Text("[Sphere %d] General Settings: ", i);
					ImGui::DragFloat3("Position", glm::value_ptr(sphere->Position), 0.01f);
					ImGui::DragInt("Material Index", &sphere->MaterialIndex, 1.0f, 0.0, (int)m_Scene.Materials.size() - 1);

					ImGui::Text("[Sphere %d] Object specific Settings: ", i);
					ImGui::DragFloat("Radius", &sphere->Radius, 0.01f);

					if (ImGui::Button("X")) m_Scene.SceneObjects.erase(m_Scene.SceneObjects.begin() + i);
				}

				ImGui::Separator();

				ImGui::PopID();
			}

			// Add new object

			if (ImGui::Button("Add new Sphere"))
			{
				Sphere* sphere = new Sphere();
				sphere->Position = { 0.0f, 0.0f, 0.0f };
				sphere->Radius = 1.0f;
				sphere->MaterialIndex = 0;
				m_Scene.SceneObjects.push_back(sphere);
			}

			if (ImGui::Button("Add new Cube"))
			{
				Cube* cube = new Cube();
				cube->Position = { 0.0f, 0.0f, 0.0f };
				cube->Dimensions = glm::vec3(1.0f);
				cube->MaterialIndex = 0;
				m_Scene.SceneObjects.push_back(cube);
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

		/*if (m_IsRealTime)
		{
			if (ImGui::IsAnyMouseDown())
				m_Renderer.ResetFrameIndex();
		}*/

		if (Input::IsKeyDown(KeyCode::LeftShift) && m_IsRealTime)
			m_Renderer.ResetFrameIndex();

		if (m_IsRealTime) {
			Render(); // UNCOMMENT FOR RT-RAYTRACING
		}
	}

	void Render() 
	{
		Timer time;

		m_Renderer.OnResize(m_ViewportWidth * m_ResolutionScale, m_ViewportHeight * m_ResolutionScale);
		m_Camera.OnResize(m_ViewportWidth * m_ResolutionScale, m_ViewportHeight * m_ResolutionScale);
		m_Renderer.Render(m_Scene, m_Camera);

		if(m_IsRealTime)
			m_LastRenderTime = time.ElapsedMillis();
	}

public:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	char m_ImageFileName[256] = "Render";
	char m_SceneFileName[256] = "Render";

	float m_ResolutionScale = 1.0f;

	float m_LastRenderTime = 0.0f;
	int m_Samples = 10;
	bool m_IsRealTime = true;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("App"))
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