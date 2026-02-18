#pragma once

#include "../Tabs/TabManager2.h"
#include "AddressBar.h"

#include "imgui.h"

#include <functional>
#include <string>

class BrowserViewport
{
public:
	explicit BrowserViewport(const std::string& uniqueId = "main");

	void Render(TabManager2& tabManager);
	void UpdateBrowserImage(BrowserTab2& tab);

private:
	void RenderTabBar(TabManager2& tabManager);
	void RenderTabContent(BrowserTab2& tab);
	void RenderBrowserContent(BrowserTab2& tab);
	void ForwardInputToBrowser(BrowserTab2& tab, ImVec2 imagePos);

	std::string m_uniqueId;
	AddressBar m_addressBar;
};
