#pragma once

#include "../Browser/TabManager2.h"
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

	void Render(TabManager2& tabManager);
	void UpdateBrowserImage(BrowserTab2& tab);

	void SetHistoryManager(HistoryManager* hm)
	{
		m_addressBar.SetHistoryManager(hm);
		m_historyManager = hm;
	}

	void OpenHistoryWindow() { m_historyWindow.Open(); }

private:
	void RenderTabBar(TabManager2& tabManager);
	void RenderTabContent(BrowserTab2& tab);
	void RenderBrowserContent(BrowserTab2& tab);
	void ForwardInputToBrowser(BrowserTab2& tab, ImVec2 imagePos);

	std::string m_uniqueId;
	AddressBar m_addressBar;
	HistoryWindow m_historyWindow;
	HistoryManager* m_historyManager = nullptr;
};
