#include "TabManager2.h"

#include <algorithm>

int TabManager2::s_nextGlobalId = 0;

int TabManager2::GenerateTabId()
{
	return s_nextGlobalId++;
}

int TabManager2::AddTab()
{
	int id = GenerateTabId();
	m_tabs.emplace_back(std::make_shared<BrowserTab2>(id));
	m_activeTabId = id;
	return id;
}

int TabManager2::AddTab(const std::string& url)
{
	int id = AddTab();
	if (auto* tab = FindTab(id))
		tab->Open(url);
	return id;
}

int TabManager2::AcceptTab(std::shared_ptr<BrowserTab2> tab)
{
	if (!tab)
		return -1;

	int id = tab->GetId();
	m_tabs.push_back(std::move(tab));
	m_activeTabId = id;
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
	m_activeTabId = -1;
}

void TabManager2::SetActiveTab(int id)   { m_activeTabId = id; }
int  TabManager2::GetActiveTabId() const { return m_activeTabId; }
BrowserTab2* TabManager2::GetActiveTab() { return FindTab(m_activeTabId); }

BrowserTab2* TabManager2::FindTab(int id)
{
	auto it = std::ranges::find_if(m_tabs,
		[id](const auto& t) { return t->GetId() == id; });
	return it != m_tabs.end() ? it->get() : nullptr;
}

const std::vector<std::shared_ptr<BrowserTab2>>& TabManager2::Tabs() const { return m_tabs; }
bool TabManager2::HasAnyTab() const { return !m_tabs.empty(); }

void TabManager2::PickNextActiveTab()
{
	if (m_tabs.empty())
	{
		m_activeTabId = -1;
		return;
	}

	for (const auto& tab : m_tabs)
		if (tab->GetState() != TabState::Blank)
		{
			m_activeTabId = tab->GetId();
			return;
		}

	m_activeTabId = m_tabs.front()->GetId();
}