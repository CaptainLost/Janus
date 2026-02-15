#pragma once

#include "BrowserTab.h"
#include "TabManager.h"

#include <string>

/// Handles rendering and interaction with the browser viewport.
class BrowserViewport
{
public:
	BrowserViewport() = default;

	/// Renders the main browser viewport window with tab bar.
	/// Returns true if the viewport was rendered successfully.
	void Render(TabManager& tabManager, const std::string& newTabURL);

	/// Updates the browser image for a given tab.
	void UpdateBrowserImage(BrowserTab& tab);

	/// Forwards input events (mouse, keyboard) to the browser.
	void ForwardInputToBrowser(BrowserTab& tab);

private:
	/// Renders the tab bar at the top of the viewport.
	void RenderTabBar(TabManager& tabManager, const std::string& newTabURL);

	/// Renders the actual browser content for the active tab.
	void RenderBrowserContent(BrowserTab* activeTab);
};
