#pragma once

#include "Walnut/Layer.h"
#include "Browser/TabManager.h"
#include "UI/SidebarPanel.h"

#include <memory>
#include <string>
#include <unordered_map>

class BrowserViewport;
class HistoryManager;
class SavedDatabase;
class SavedTabsManager;
class SavedFoldersManager;

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
	void RenderMainViewport();

	std::shared_ptr<TabManager> m_tabManager = std::make_shared<TabManager>();

	std::unique_ptr<BrowserViewport> m_viewport;
	std::unique_ptr<HistoryManager> m_historyManager;
	std::unique_ptr<SavedDatabase> m_savedDatabase;
	std::unique_ptr<SavedTabsManager> m_savedTabsManager;
	std::unique_ptr<SavedFoldersManager> m_savedFoldersManager;
	SidebarPanel m_sidebarPanel;

	std::unordered_map<int, std::string> m_lastRecordedUrls;
	bool m_layoutBuilt = false;
};
