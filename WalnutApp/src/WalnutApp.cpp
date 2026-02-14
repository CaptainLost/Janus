#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "BrowserLayer.h"

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name   = "Browser";
	spec.Width  = 1400;
	spec.Height = 900;

	Application* app = new Walnut::Application(spec);
	app->PushLayer<BrowserLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Close"))
				app->Close();
			ImGui::EndMenu();
		}
	});
	return app;
}