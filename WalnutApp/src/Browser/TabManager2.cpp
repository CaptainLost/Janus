#include "TabManager2.h"

#include <algorithm>

int TabManager2::s_nextGlobalId = -1;

int TabManager2::GenerateTabId()
{
	return ++s_nextGlobalId;
}

int TabManager2::AddTab()
{
	int id = GenerateTabId();

	if (m_tabs.size() == 0)
	{
		SetActiveTab(id);
	}

	m_tabs.emplace_back(std::make_shared<BrowserTab2>(id));

	return id;
}

int TabManager2::AcceptTab(std::shared_ptr<BrowserTab2> tab)
{
	if (!tab)
		return -1;

	int id = tab->GetId();
	m_tabs.push_back(std::move(tab));
	SetActiveTab(id);
	return id;
}

std::shared_ptr<BrowserTab2> TabManager2::DetachTab(int id)
{
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });

	if (it == m_tabs.end())
		return nullptr;

	auto tab = std::move(*it);
	m_tabs.erase(it);

	if (m_activeTabId == id)
		PickNextActiveTab();

	return tab;
}

void TabManager2::RemoveTab(int id)
{
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });

	if (it == m_tabs.end())
		return;

	(*it)->Close();
	m_tabs.erase(it);

	if (m_activeTabId == id)
		PickNextActiveTab();
}

void TabManager2::CloseAll()
{
	for (auto& tab : m_tabs)
		tab->Close();
	m_tabs.clear();
	SetActiveTab(-1);
}

void TabManager2::SetActiveTab(int id)
{
	m_activeTabId = id;
}

BrowserTab2* TabManager2::GetTab(int id)
{
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });

	if (it == m_tabs.end())
		return nullptr;

	return it->get();
}

int TabManager2::GetActiveTabId() const
{
	return m_activeTabId;
}

BrowserTab2* TabManager2::GetActiveTab()
{
	return GetTab(m_activeTabId);
}

const std::vector<std::shared_ptr<BrowserTab2>>& TabManager2::Tabs() const
{
	return m_tabs;
}

bool TabManager2::HasAnyTab() const
{
	return !m_tabs.empty();
}

void TabManager2::PickNextActiveTab()
{
	if (!HasAnyTab())
	{
		SetActiveTab(-1);

		return;
	}

	BrowserTab2* validIndex = GetTab(m_activeTabId);

	if (validIndex == nullptr)
	{
		SetActiveTab(m_tabs.front()->GetId());

		return;
	}

	SetActiveTab(-1);
}