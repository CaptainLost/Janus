#pragma once

#include "Tabs/Tab.h"

#include <memory>
#include <vector>

class TabManager
{
public:
	int AddTab();
	int AddSavedTab(int dbId, const std::string& baseUrl);

	int AcceptTab(std::shared_ptr<Tab> tab);
	std::shared_ptr<Tab> DetachTab(int id);

	void RemoveTab(int id);
	void CloseAll();

	void SetActiveTab(int id);

	Tab* GetTab(int id);

	int GetActiveTabId() const;
	Tab* GetActiveTab();

	const std::vector<std::shared_ptr<Tab>>& Tabs() const;
	bool HasAnyTab() const;

	static int GenerateTabId();

private:
	void PickNextActiveTab();

	int m_activeTabId = -1;
	std::vector<std::shared_ptr<Tab>> m_tabs;

	static int s_nextGlobalId;
};
