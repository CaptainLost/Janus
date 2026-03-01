#include "AdressBarPanel.h"

#include <imgui.h>
#include <IconsFontAwesome6.h>
#include "../Utils/UrlUtils.h"

static int UrlInputResizeCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		std::string* str = (std::string*)data->UserData;
		str->resize(data->BufSize - 1);
		data->Buf = str->data();
	}

	return 0;
}

AdressBarPanel::AdressBarPanel()
{
}

void AdressBarPanel::Render(const std::shared_ptr<Tab>& tab)
{
	RenderNavigationButtons(tab);

	ImGui::SameLine();

	RenderUrlInput(tab);
}

void AdressBarPanel::RenderNavigationButtons(const std::shared_ptr<Tab>& tab)
{
	Walnut::WebViewState state = tab->GetWebViewState();

	if (!state.CanGoBack)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button(ICON_FA_ARROW_LEFT "##nav_back"))
	{
		tab->GoBack();
	}

	if (!state.CanGoBack)
	{
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	if (!state.CanGoForward)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button(ICON_FA_ARROW_RIGHT "##nav_forward"))
	{
		tab->GoForward();
	}

	if (!state.CanGoForward)
	{
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	const char* reloadButtonLabel = state.IsLoading
		? ICON_FA_XMARK "##nav_action"
		: ICON_FA_ROTATE_RIGHT "##nav_action";

	if (ImGui::Button(reloadButtonLabel))
	{
		if (state.IsLoading)
		{
			tab->StopLoading();
		}
		else
		{
			tab->ReloadPage();
		}
	}
}

void AdressBarPanel::RenderUrlInput(const std::shared_ptr<Tab>& tab)
{
	Walnut::WebViewState state = tab->GetWebViewState();

	RenderSecurityIcon(tab, state);

	ImGui::SameLine();

	float goButtonWidth = ImGui::CalcTextSize(ICON_FA_ARROW_RIGHT).x + ImGui::GetStyle().FramePadding.x * 2;
	float inputWidth = ImGui::GetContentRegionAvail().x - goButtonWidth - ImGui::GetStyle().ItemSpacing.x;

	ImGui::SetNextItemWidth(inputWidth);

	std::string& urlInput = tab->GetUrlInput();

	bool enterPressed = ImGui::InputTextWithHint(
		"##nav_url",
		"Enter web address...",
		urlInput.data(),
		urlInput.capacity() + 1,
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize,
		UrlInputResizeCallback,
		&urlInput);

	ImGui::SameLine();

	if (ImGui::Button(ICON_FA_ARROW_RIGHT "##nav_go") || enterPressed)
	{
		tab->Open(urlInput);
	}
}

void AdressBarPanel::RenderSecurityIcon(const std::shared_ptr<Tab>& tab, const Walnut::WebViewState& state)
{
	if (UrlUtils::IsHttps(state.URL))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.85f, 0.4f, 1.0f));
		ImGui::TextUnformatted(ICON_FA_LOCK);
		ImGui::PopStyleColor();
	}
	else if (UrlUtils::IsHttp(state.URL))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.55f, 0.2f, 1.0f));
		ImGui::TextUnformatted(ICON_FA_LOCK_OPEN);
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::TextUnformatted(ICON_FA_LOCK_OPEN);
	}
}