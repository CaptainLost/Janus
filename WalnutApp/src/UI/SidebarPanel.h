#pragma once

#include "../Browser/TabManager.h"

class SidebarPanel
{
public:
	SidebarPanel(const std::shared_ptr<TabManager>& tabManager);

	void Render();

private:
	using tabCallbackFn = const std::function<void(const std::shared_ptr<Tab>&)>;

	void RenderTabSeparator(const std::string& label, tabCallbackFn& onTabDropCallback);

	void RenderPersistentTabSection();
	void RenderTemporaryTabSection();

	void RenderTabComplete(const std::shared_ptr<Tab>& tab, tabCallbackFn& onTabClickedCallback, tabCallbackFn& onCloseCallback, tabCallbackFn& onDuplicateCallback);
	void RenderTab(const std::shared_ptr<Tab>& tab, bool isActive, tabCallbackFn& onTabClickedCallback);
	void RenderTabCloseButton(const std::shared_ptr<Tab>& tab, bool isHovered, bool isClicked, tabCallbackFn& onCloseCallback);
	void RenderTabContextPopup(const std::shared_ptr<Tab>& tab, tabCallbackFn& onCloseCallback, tabCallbackFn& onDuplicateCallback);

	void OnTabDuplicate(const std::shared_ptr<Tab>& tab);

	void OnTemporaryTabClicked(const std::shared_ptr<Tab>& tab);
	void OnTemporaryTabClose(const std::shared_ptr<Tab>& tab);
	void OnTemporaryTabMoveToSection(const std::shared_ptr<Tab>& tab);

	void OnSavedTabClicked(const std::shared_ptr<Tab>& tab);
	void OnSavedTabClose(const std::shared_ptr<Tab>& tab);
	void OnSavedTabMoveToSection(const std::shared_ptr<Tab>& tab);

private:
	std::shared_ptr<TabManager> m_tabManager;
};
