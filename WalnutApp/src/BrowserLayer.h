#pragma once

#include "Walnut/Layer.h"
#include "Walnut/Image.h"
#include "Walnut/WebView.h"

#include "TabManager.h"

#include "include/cef_base.h"

#include <memory>
#include <string>

class BrowserLayer : public Walnut::Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float ts) override;
	void OnUIRender() override;

private:
	// Per-tab helpers
	void UpdateBrowserImage(BrowserTab& tab);
	void SyncURLFromBrowser(BrowserTab& tab);
	void ForwardInputToBrowser(BrowserTab& tab);

	// Layout
	void BuildDockLayout();

	// UI sections
	void RenderSidebar();
	void RenderAddressBar();
	void RenderBrowserViewport();

	// Dialogs
	void RenderNewTabPopup();

	TabManager m_TabManager;

	// Dock layout
	bool m_LayoutBuilt = false;

	// Sidebar visibility
	bool m_SidebarOpen = true;

	// "New Tab" dialog state
	bool   m_ShowNewTabPopup      = false;
	int    m_NewTabKind           = 0;  // 0=Dynamic, 1=Permanent, 2=Temporary
	char   m_NewTabURL[2048]      = "https://www.google.com";
	float  m_NewTabLifetimeHours  = 24.0f;
};
