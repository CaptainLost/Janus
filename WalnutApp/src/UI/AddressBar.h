#pragma once

#include "../Tabs/TabManager2.h"
#include "../Tabs/BrowserTab2.h"
#include "../HistoryManager.h"
#include "UrlSuggestions.h"

#include "imgui.h"

#include <string>

class AddressBar
{
public:
	AddressBar() = default;

	void Render(TabManager2& tabManager);
	void RenderForTab(BrowserTab2* tab);

	void SetHistoryManager(HistoryManager* hm) { m_history = hm; }

	bool IsUrlBarFocused() const { return m_urlBarFocused; }

	static int StringResizeCallback(ImGuiInputTextCallbackData* data);

private:
	void RenderNavigationButtons(BrowserTab2* tab);
	void RenderUrlInput(BrowserTab2* tab, bool& urlBarFocused);

	HistoryManager* m_history = nullptr;
	UrlSuggestions m_suggestions;
	bool m_urlBarFocused = false;
};
