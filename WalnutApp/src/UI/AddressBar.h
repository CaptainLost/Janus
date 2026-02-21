#pragma once

#include "../Browser/TabManager.h"
#include "../Browser/Tabs/Tab.h"
#include "../History/HistoryManager.h"
#include "UrlSuggestions.h"

#include "imgui.h"

#include <string>

class AddressBar
{
public:
	AddressBar() = default;

	void Render(TabManager& tabManager);
	void RenderForTab(Tab* tab);

	void SetHistoryManager(HistoryManager* hm) { m_history = hm; }

	bool IsUrlBarFocused() const { return m_urlBarFocused; }

	static int StringResizeCallback(ImGuiInputTextCallbackData* data);

private:
	void RenderNavigationButtons(Tab* tab);
	void RenderUrlInput(Tab* tab, bool& urlBarFocused);

	HistoryManager* m_history = nullptr;
	UrlSuggestions m_suggestions;
	bool m_urlBarFocused = false;
};
