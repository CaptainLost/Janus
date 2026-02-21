#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"
#include "BrowserLayer.h"

static void TitlebarButton(const char* label)
{
	const float size = ImGui::GetWindowHeight();
	ImGui::SetCursorPosY(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
	ImGui::Button(label, ImVec2(size, size));
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
}

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Janus";
	spec.Width = 1400;
	spec.Height = 900;
	spec.CustomTitlebar = true;

	Application* app = new Walnut::Application(spec);

	auto browserLayer = std::make_shared<BrowserLayer>();
	app->PushLayer(browserLayer);

	app->SetTitlebarLeftCallback([]()
	{
		TitlebarButton(ICON_FA_BARS "##sidebar");
		ImGui::SameLine(0.0f, 0.0f);
		TitlebarButton(ICON_FA_DOWNLOAD "##downloads");
	});

	app->SetTitlebarRightCallback([browserLayer]()
	{
		TitlebarButton(ICON_FA_GEAR "##settings");

		ImVec2 buttonMin = ImGui::GetItemRectMin();
		ImVec2 buttonMax = ImGui::GetItemRectMax();

		if (ImGui::IsItemClicked())
		{
			ImGui::OpenPopup("##settings_popup");
		}

		ImGui::SetNextWindowPos(ImVec2(buttonMin.x, buttonMax.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		if (ImGui::BeginPopup("##settings_popup"))
		{
			if (ImGui::MenuItem(ICON_FA_CLOCK_ROTATE_LEFT "  History"))
			{
				browserLayer->OpenHistoryWindow();
			}

			ImGui::EndPopup();
		}
	});

	return app;
}
