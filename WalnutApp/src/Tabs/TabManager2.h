#pragma once

#include "BrowserTab2.h"

#include <memory>
#include <vector>

class TabManager2
{
public:
	int AddTab();
	int AddTab(const std::string& url);

	/// Insert an existing tab without closing it. Returns its id.
	int AcceptTab(std::shared_ptr<BrowserTab2> tab);

	/// Remove a tab without closing its WebView. Returns ownership.
	std::shared_ptr<BrowserTab2> DetachTab(int id);

	void RemoveTab(int id);
	void CloseAll();

	void         SetActiveTab(int id);
	int          GetActiveTabId() const;
	BrowserTab2* GetActiveTab();

	BrowserTab2*                                     FindTab(int id);
	const std::vector<std::shared_ptr<BrowserTab2>>& Tabs() const;
	bool                                             HasAnyTab() const;

	static int GenerateTabId();

private:
	void PickNextActiveTab();

	int m_activeTabId = -1;
	std::vector<std::shared_ptr<BrowserTab2>> m_tabs;

	static int s_nextGlobalId;
};