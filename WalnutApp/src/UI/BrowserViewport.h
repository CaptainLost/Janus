#pragma once

#include "../Browser/TabManager.h"
#include "../History/HistoryManager.h"
#include "AddressBar.h"
#include "../History/HistoryWindow.h"

#include "imgui.h"

#include <functional>
#include <string>

class BrowserViewport
{
public:
	explicit BrowserViewport(const std::string& uniqueId = "main");

	void Render(TabManager& tabManager);
	void UpdateBrowserImage(Tab& tab);
	void UpdateFaviconImage(Tab& tab);

	void SetHistoryManager(HistoryManager* hm)
	{
		m_addressBar.SetHistoryManager(hm);
		m_historyManager = hm;
	}

	void OpenHistoryWindow() { m_historyWindow.Open(); }

private:
	void RenderTabBar(TabManager& tabManager);
	void DrawFaviconInTab(Tab& tab);
	void RenderTabContent(Tab& tab);
	void RenderBrowserContent(Tab& tab);
	void ForwardInputToBrowser(Tab& tab, ImVec2 imagePos);

	std::string m_uniqueId;
	AddressBar m_addressBar;
	HistoryWindow m_historyWindow;
	HistoryManager* m_historyManager = nullptr;
};
