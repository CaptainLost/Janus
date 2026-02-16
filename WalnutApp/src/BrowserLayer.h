#pragma once

#include "Walnut/Layer.h"

#include "Tabs/TabManager2.h"
#include "BrowserViewport.h"

#include <memory>
#include <string>
#include <vector>

class BrowserLayer : public Walnut::Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float ts) override;
	void OnUIRender() override;

private:
	void BuildDockLayout();
	void RenderSidebar();
	void RenderMainViewport();
	void RenderDetachedWindows();
	void OnTabDetachRequested(TabManager2& srcManager, int tabId, ImVec2 mousePos);

	TabManager2     m_tabManager;
	BrowserViewport m_viewport{"main"};

	struct DetachedViewport
	{
		TabManager2                      tabManager;
		std::unique_ptr<BrowserViewport> viewport;
		bool                             open = true;
	};
	std::vector<std::unique_ptr<DetachedViewport>> m_detachedViewports;
	int m_nextDetachedId = 0;

	bool m_layoutBuilt = false;
};
