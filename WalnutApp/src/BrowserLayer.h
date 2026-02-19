#pragma once

#include "Walnut/Layer.h"

#include "Tabs/TabManager2.h"
#include "UI/BrowserViewport.h"
#include "HistoryManager.h"

#include <string>
#include <unordered_map>
#include <vector>

class BrowserLayer : public Walnut::Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float ts) override;
	void OnUIRender() override;

	void OpenHistoryWindow() { m_viewport.OpenHistoryWindow(); }

private:
	void BuildDockLayout();
	void RenderSidebar();
	void RenderMainViewport();

	TabManager2 m_tabManager;
	BrowserViewport m_viewport{"main"};
	HistoryManager m_historyManager;

	std::unordered_map<int, std::string> m_lastRecordedUrls;

	bool m_layoutBuilt = false;
};
