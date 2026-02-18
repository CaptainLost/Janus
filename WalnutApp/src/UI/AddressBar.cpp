#include "AddressBar.h"

#include <algorithm>
#include <cstring>
#include <IconsFontAwesome6.h>

void AddressBar::Render(TabManager2& tabManager)
{
	BrowserTab2* tab = tabManager.GetActiveTab();
	if (!tab)
	{
		ImGui::TextDisabled("No active tab");
		return;
	}

	RenderNavigationButtons(tab);
	ImGui::SameLine();
	RenderUrlInput(tab, m_urlBarFocused);
}

void AddressBar::RenderForTab(BrowserTab2* tab)
{
	if (!tab)
		return;

	ImGui::PushID(tab->GetId());
	bool localFocused = false;
	RenderNavigationButtons(tab);
	ImGui::SameLine();
	RenderUrlInput(tab, localFocused);
	ImGui::PopID();
}

void AddressBar::RenderNavigationButtons(BrowserTab2* tab)
{
	bool isOpen = tab->IsOpen();
	Walnut::WebViewState state;
	CefRefPtr<Walnut::WebView> webView;

	if (isOpen)
	{
		state = tab->GetWebViewState();
		webView = tab->GetWebView();
	}

	ImGui::BeginDisabled(!isOpen || !state.CanGoBack);
	if (ImGui::Button(ICON_FA_ANGLE_LEFT "##navigationPrevious") && webView)
		webView->GoBack();
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(!isOpen || !state.CanGoForward);
	if (ImGui::Button(ICON_FA_ANGLE_RIGHT "##navigationNext") && webView)
		webView->GoForward();
	ImGui::EndDisabled();

	ImGui::SameLine();

	if (isOpen && state.IsLoading)
	{
		if (ImGui::Button("X") && webView)
			webView->StopLoading();
	}
	else
	{
		ImGui::BeginDisabled(!isOpen);
		if (ImGui::Button(ICON_FA_ROTATE_RIGHT "##refresh") && webView)
			webView->Reload();
		ImGui::EndDisabled();
	}
}

void AddressBar::RenderUrlInput(BrowserTab2* tab, bool& urlBarFocused)
{
	bool isOpen = tab->IsOpen();
	Walnut::WebViewState state;

	if (isOpen)
		state = tab->GetWebViewState();

	ImGui::SameLine();

	std::string& url = tab->GetUrlInput();
	if (url.capacity() < 2048)
		url.reserve(2048);

	if (isOpen && !urlBarFocused && !state.URL.empty())
		url = state.URL;

	float goWidth = ImGui::CalcTextSize(ICON_FA_PLAY).x
		+ ImGui::GetStyle().ItemSpacing.x * 2
		+ ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - goWidth);

	bool enterPressed = ImGui::InputText(
		"##nav_url",
		url.data(),
		url.capacity() + 1,
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize,
		StringResizeCallback,
		&url);

	urlBarFocused = ImGui::IsItemActive();
	url.resize(std::strlen(url.c_str()));

	ImGui::SameLine();

	bool goPressed = ImGui::Button(ICON_FA_PLAY "##goLabel");

	if ((enterPressed || goPressed) && !url.empty())
		tab->Open(url);
}

int AddressBar::StringResizeCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		auto* str = static_cast<std::string*>(data->UserData);
		str->resize(data->BufTextLen);
		data->Buf = str->data();
	}
	return 0;
}
