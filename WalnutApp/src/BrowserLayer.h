#pragma once

#include "Walnut/Layer.h"

#include <memory>
#include <string>
#include <unordered_map>

class TabManager;
class BrowserViewport;
class HistoryManager;
class SavedTabsManager;
class Sidebar;

class BrowserLayer : public Walnut::Layer
{
public:
	BrowserLayer();
	~BrowserLayer() override;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float ts) override;
	void OnUIRender() override;

	void OpenHistoryWindow();

private:
	void BuildDockLayout();
	void RenderSidebar();
	void RenderMainViewport();

	std::unique_ptr<TabManager> m_tabManager;
	std::unique_ptr<BrowserViewport> m_viewport;
	std::unique_ptr<HistoryManager> m_historyManager;
	std::unique_ptr<SavedTabsManager> m_savedTabsManager;
	std::unique_ptr<Sidebar> m_sidebar;

	std::unordered_map<int, std::string> m_lastRecordedUrls;
	bool m_layoutBuilt = false;
};
