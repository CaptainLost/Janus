#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "imgui.h"
#include "BrowserLayer.h"

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name           = "Janus";
	spec.Width          = 1400;
	spec.Height         = 900;
	spec.CustomTitlebar = true;

	Application* app = new Walnut::Application(spec);
	app->PushLayer<BrowserLayer>();
	//app->SetMenubarCallback([app]()
	//{
	//	if (ImGui::BeginMenu("File"))
	//	{
	//		if (ImGui::MenuItem("Close"))
	//			app->Close();
	//		ImGui::EndMenu();
	//	}

	//	ImGui::SameLine();

	//	if (ImGui::BeginMenu("View"))
	//	{
	//		ImGui::MenuItem("Placeholder 1");
	//		ImGui::MenuItem("Placeholder 2");
	//		ImGui::EndMenu();
	//	}
	//});

	return app;
}