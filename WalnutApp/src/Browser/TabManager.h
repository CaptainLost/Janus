#pragma once

#include "Tabs/Tab.h"

#include <memory>
#include <vector>

class TabManager
{
public:
	static constexpr int InvalidTabId = -1;

	int AddTab();
	int AddSavedTab(int dbId, const std::string& baseUrl);

	int AcceptTab(std::shared_ptr<Tab> tab);
	std::shared_ptr<Tab> DetachTab(int id);

	void RemoveTab(int id);
	void MoveTabBefore(int draggedId, int targetId);
	void CloseAll();

	void SetActiveTab(int id);

	Tab* GetTab(int id);
	const Tab* GetTab(int id) const;

	int GetActiveTabId() const;
	Tab* GetActiveTab();
	const Tab* GetActiveTab() const;

	const std::vector<std::shared_ptr<Tab>>& Tabs() const;
	bool HasAnyTab() const;

	static int GenerateTabId();

private:
	using TabIterator = std::vector<std::shared_ptr<Tab>>::iterator;
	using ConstTabIterator = std::vector<std::shared_ptr<Tab>>::const_iterator;

	int InsertTab(std::shared_ptr<Tab> tab);
	TabIterator FindIterator(int id);
	ConstTabIterator FindIterator(int id) const;
	void PickNextActiveTab();

	int m_activeTabId = InvalidTabId;
	std::vector<std::shared_ptr<Tab>> m_tabs;

	static int s_nextGlobalId;
};
