#pragma once

#include "../Browser/TabManager.h"
#include "AdressBarPanel.h"

#include <memory>

class ViewportPanel
{
public:
	ViewportPanel(const std::shared_ptr<TabManager>& tabManager);

	void Render();

private:
	void RenderTabComplete(const std::shared_ptr<Tab>& tab);
	void RenderTabContent(const std::shared_ptr<Tab>& tab);

private:
	AdressBarPanel m_adressBarPanel;

	std::shared_ptr<TabManager> m_tabManager;
	int m_lastFocusedTabId = TabManager::InvalidTabId;
};