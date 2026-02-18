#pragma once

#include "BrowserTab2.h"

#include <memory>
#include <vector>

class TabManager2
{
public:
	int AddTab();

	int AcceptTab(std::shared_ptr<BrowserTab2> tab);
	std::shared_ptr<BrowserTab2> DetachTab(int id);

	void RemoveTab(int id);
	void CloseAll();

	void SetActiveTab(int id);

	BrowserTab2* GetTab(int id);

	int GetActiveTabId() const;
	BrowserTab2* GetActiveTab();

	const std::vector<std::shared_ptr<BrowserTab2>>& Tabs() const;
	bool HasAnyTab() const;

	static int GenerateTabId();

private:
	void PickNextActiveTab();

	int m_activeTabId = -1;
	std::vector<std::shared_ptr<BrowserTab2>> m_tabs;

	static int s_nextGlobalId;
};