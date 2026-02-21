#pragma once

#include "../Browser/TabManager.h"
#include "../History/HistoryManager.h"
#include "AddressBar.h"
#include "../History/HistoryWindow.h"

#include "Walnut/WebView.h"
#include "imgui.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

class BrowserViewport
{
public:
	explicit BrowserViewport(const std::string& uniqueId = "main");

	void Render(TabManager& tabManager);

	void SetHistoryManager(HistoryManager* hm)
	{
		m_addressBar.SetHistoryManager(hm);
		m_historyManager = hm;
	}

	void OpenHistoryWindow() { m_historyWindow.Open(); }

private:
	struct ActiveContextMenu
	{
		bool requestOpen = false;
		Walnut::ContextMenuRequest params;
		ImVec2 screenPosition;
	};

	void RenderTabBar(TabManager& tabManager);
	void DrawFaviconInTab(Tab& tab);
	void RenderTabContent(Tab& tab, TabManager& tabManager);
	void RenderBrowserContent(Tab& tab, TabManager& tabManager);
	void RenderContextMenu(Tab& tab, TabManager& tabManager);
	void ForwardInputToBrowser(Tab& tab, ImVec2 imagePos);

	std::string m_uniqueId;
	AddressBar m_addressBar;
	HistoryWindow m_historyWindow;
	HistoryManager* m_historyManager = nullptr;

	std::unordered_set<int> m_dockedTabIds;
	std::unordered_map<int, ImGuiID> m_tabWindowIds;

	ActiveContextMenu m_activeContextMenu;
};
