#pragma once

#include "../Tabs/TabManager2.h"
#include "../Tabs/BrowserTab2.h"

#include "imgui.h"

#include <string>

class AddressBar
{
public:
	AddressBar() = default;

	void Render(TabManager2& tabManager);

	bool IsUrlBarFocused() const { return m_urlBarFocused; }

	static int StringResizeCallback(ImGuiInputTextCallbackData* data);

private:
	void RenderNavigationButtons(BrowserTab2* tab);
	void RenderUrlInput(BrowserTab2* tab);
	void HandleUrlSubmission(BrowserTab2* tab, const std::string& url);

	bool m_urlBarFocused = false;
};
