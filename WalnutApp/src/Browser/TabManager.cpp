#include "TabManager.h"
#include "Tabs/BrowserTab.h"
#include "Tabs/SavedBrowserTab.h"

#include <algorithm>

int TabManager::s_nextGlobalId = InvalidTabId;

int TabManager::GenerateTabId()
{
	return ++s_nextGlobalId;
}

int TabManager::InsertTab(std::shared_ptr<Tab> tab)
{
	int id = tab->GetId();

	if (m_tabs.empty())
	{
		SetActiveTab(id);
	}

	m_tabs.push_back(std::move(tab));

	return id;
}

int TabManager::AddTab()
{
	return InsertTab(std::make_shared<BrowserTab>(GenerateTabId()));
}

int TabManager::AddSavedTab(int dbId, const std::string& baseUrl)
{
	return InsertTab(std::make_shared<SavedBrowserTab>(GenerateTabId(), dbId, baseUrl));
}

int TabManager::AcceptTab(std::shared_ptr<Tab> tab)
{
	if (!tab)
	{
		return InvalidTabId;
	}

	int id = tab->GetId();
	m_tabs.push_back(std::move(tab));
	SetActiveTab(id);

	return id;
}

std::shared_ptr<Tab> TabManager::DetachTab(int id)
{
	auto it = FindIterator(id);
	if (it == m_tabs.end())
	{
		return nullptr;
	}

	auto tab = std::move(*it);
	m_tabs.erase(it);

	if (m_activeTabId == id)
	{
		PickNextActiveTab();
	}

	return tab;
}

void TabManager::RemoveTab(int id)
{
	auto it = FindIterator(id);
	if (it == m_tabs.end())
	{
		return;
	}

	(*it)->Close();
	m_tabs.erase(it);

	if (m_activeTabId == id)
	{
		PickNextActiveTab();
	}
}

void TabManager::MoveTabBefore(int draggedId, int targetId)
{
	auto draggedIt = FindIterator(draggedId);
	auto targetIt = FindIterator(targetId);

	if (draggedIt == m_tabs.end() || targetIt == m_tabs.end() || draggedIt == targetIt)
	{
		return;
	}

	auto tab = std::move(*draggedIt);
	m_tabs.erase(draggedIt);

	targetIt = FindIterator(targetId);

	if (targetIt == m_tabs.end())
	{
		m_tabs.push_back(std::move(tab));
	}
	else
	{
		m_tabs.insert(targetIt, std::move(tab));
	}
}

void TabManager::CloseAll()
{
	for (auto& tab : m_tabs)
	{
		tab->Close();
	}

	m_tabs.clear();
	SetActiveTab(InvalidTabId);
}

void TabManager::SetActiveTab(int id)
{
	m_activeTabId = id;
}

Tab* TabManager::GetTab(int id)
{
	auto it = FindIterator(id);
	if (it == m_tabs.end())
	{
		return nullptr;
	}

	return it->get();
}

const Tab* TabManager::GetTab(int id) const
{
	auto it = FindIterator(id);
	if (it == m_tabs.end())
	{
		return nullptr;
	}

	return it->get();
}

int TabManager::GetActiveTabId() const
{
	return m_activeTabId;
}

Tab* TabManager::GetActiveTab()
{
	return GetTab(m_activeTabId);
}

const Tab* TabManager::GetActiveTab() const
{
	return GetTab(m_activeTabId);
}

const std::vector<std::shared_ptr<Tab>>& TabManager::Tabs() const
{
	return m_tabs;
}

bool TabManager::HasAnyTab() const
{
	return !m_tabs.empty();
}

TabManager::TabIterator TabManager::FindIterator(int id)
{
	return std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });
}

TabManager::ConstTabIterator TabManager::FindIterator(int id) const
{
	return std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });
}

void TabManager::PickNextActiveTab()
{
	if (!HasAnyTab())
	{
		SetActiveTab(InvalidTabId);

		return;
	}

	Tab* validTab = GetTab(m_activeTabId);

	if (validTab == nullptr)
	{
		SetActiveTab(m_tabs.front()->GetId());

		return;
	}

	SetActiveTab(InvalidTabId);
}
