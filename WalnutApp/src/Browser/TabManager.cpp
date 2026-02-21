#include "TabManager.h"
#include "Tabs/BrowserTab.h"
#include "Tabs/SavedBrowserTab.h"

#include <algorithm>

int TabManager::s_nextGlobalId = -1;

int TabManager::GenerateTabId()
{
	return ++s_nextGlobalId;
}

int TabManager::AddTab()
{
	int id = GenerateTabId();

	if (m_tabs.empty())
	{
		SetActiveTab(id);
	}

	m_tabs.emplace_back(std::make_shared<BrowserTab>(id));

	return id;
}

int TabManager::AddSavedTab(int dbId, const std::string& baseUrl)
{
	int id = GenerateTabId();

	if (m_tabs.empty())
	{
		SetActiveTab(id);
	}

	m_tabs.emplace_back(std::make_shared<SavedBrowserTab>(id, dbId, baseUrl));

	return id;
}

int TabManager::AcceptTab(std::shared_ptr<Tab> tab)
{
	if (!tab)
	{
		return -1;
	}

	int id = tab->GetId();
	m_tabs.push_back(std::move(tab));
	SetActiveTab(id);

	return id;
}

std::shared_ptr<Tab> TabManager::DetachTab(int id)
{
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });

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
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });

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

void TabManager::CloseAll()
{
	for (auto& tab : m_tabs)
	{
		tab->Close();
	}

	m_tabs.clear();
	SetActiveTab(-1);
}

void TabManager::SetActiveTab(int id)
{
	m_activeTabId = id;
}

Tab* TabManager::GetTab(int id)
{
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });

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

const std::vector<std::shared_ptr<Tab>>& TabManager::Tabs() const
{
	return m_tabs;
}

bool TabManager::HasAnyTab() const
{
	return !m_tabs.empty();
}

void TabManager::PickNextActiveTab()
{
	if (!HasAnyTab())
	{
		SetActiveTab(-1);

		return;
	}

	Tab* validTab = GetTab(m_activeTabId);

	if (validTab == nullptr)
	{
		SetActiveTab(m_tabs.front()->GetId());

		return;
	}

	SetActiveTab(-1);
}
