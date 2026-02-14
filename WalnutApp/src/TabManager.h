#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "BrowserTab.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

/// Manages three collections of tabs: dynamic, permanent, and temporary.
class TabManager
{
public:
	TabManager() = default;

	// ----- Tab creation -----------------------------------------------------

	/// Create a new dynamic tab (top bar) and make it active.
	int AddDynamicTab(const std::string& url = "https://www.google.com",
	                  int viewW = 1280, int viewH = 720)
	{
		BrowserTab tab;
		tab.ID        = m_NextID++;
		tab.Kind      = TabKind::Dynamic;
		tab.StartURL  = url;
		tab.ViewWidth = viewW;
		tab.ViewHeight= viewH;
		tab.Create();
		m_DynamicTabs.push_back(std::move(tab));
		m_ActiveTabID = m_DynamicTabs.back().ID;
		return m_ActiveTabID;
	}

	/// Create a new permanent tab (left sidebar, always persists).
	int AddPermanentTab(const std::string& url = "https://www.google.com",
	                    int viewW = 1280, int viewH = 720)
	{
		BrowserTab tab;
		tab.ID        = m_NextID++;
		tab.Kind      = TabKind::Permanent;
		tab.StartURL  = url;
		tab.ViewWidth = viewW;
		tab.ViewHeight= viewH;
		// Permanent tabs are listed but not necessarily opened immediately.
		m_PermanentTabs.push_back(std::move(tab));
		return m_PermanentTabs.back().ID;
	}

	/// Create a new temporary tab (left sidebar, auto-expires).
	/// `lifetimeSeconds` – how many seconds from *now* until it disappears.
	int AddTemporaryTab(const std::string& url = "https://www.google.com",
	                    double lifetimeSeconds = 86400.0,
	                    int viewW = 1280, int viewH = 720)
	{
		BrowserTab tab;
		tab.ID        = m_NextID++;
		tab.Kind      = TabKind::Temporary;
		tab.StartURL  = url;
		tab.ViewWidth = viewW;
		tab.ViewHeight= viewH;
		tab.Deadline  = std::chrono::steady_clock::now()
		              + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
		                    std::chrono::duration<double>(lifetimeSeconds));
		m_TemporaryTabs.push_back(std::move(tab));
		return m_TemporaryTabs.back().ID;
	}

	// ----- Tab access -------------------------------------------------------

	BrowserTab* GetActiveTab()
	{
		return FindTab(m_ActiveTabID);
	}

	void SetActiveTab(int id)
	{
		m_ActiveTabID = id;
	}

	int GetActiveTabID() const { return m_ActiveTabID; }

	BrowserTab* FindTab(int id)
	{
		for (auto& t : m_DynamicTabs)   if (t.ID == id) return &t;
		for (auto& t : m_PermanentTabs) if (t.ID == id) return &t;
		for (auto& t : m_TemporaryTabs) if (t.ID == id) return &t;
		return nullptr;
	}

	std::vector<BrowserTab>& DynamicTabs()   { return m_DynamicTabs; }
	std::vector<BrowserTab>& PermanentTabs() { return m_PermanentTabs; }
	std::vector<BrowserTab>& TemporaryTabs() { return m_TemporaryTabs; }

	// ----- Opening / Closing ------------------------------------------------

	/// Open a sidebar tab (creates WebView if needed) and make it active.
	void OpenTab(int id)
	{
		if (auto* tab = FindTab(id))
		{
			if (!tab->IsOpen)
				tab->Create();
			m_ActiveTabID = id;
		}
	}

	/// Close a tab's WebView but keep it in the list (for permanent / temporary).
	void CloseTabView(int id)
	{
		if (auto* tab = FindTab(id))
		{
			tab->Close();
			// If this was the active tab, pick another.
			if (m_ActiveTabID == id)
				PickNextActiveTab();
		}
	}

	/// Remove a dynamic tab entirely.
	void RemoveDynamicTab(int id)
	{
		auto it = std::find_if(m_DynamicTabs.begin(), m_DynamicTabs.end(),
		                       [id](auto& t){ return t.ID == id; });
		if (it != m_DynamicTabs.end())
		{
			it->Close();
			m_DynamicTabs.erase(it);
		}
		if (m_ActiveTabID == id)
			PickNextActiveTab();
	}

	/// Remove a permanent tab entirely.
	void RemovePermanentTab(int id)
	{
		auto it = std::find_if(m_PermanentTabs.begin(), m_PermanentTabs.end(),
		                       [id](auto& t){ return t.ID == id; });
		if (it != m_PermanentTabs.end())
		{
			it->Close();
			m_PermanentTabs.erase(it);
		}
		if (m_ActiveTabID == id)
			PickNextActiveTab();
	}

	/// Remove a temporary tab entirely.
	void RemoveTemporaryTab(int id)
	{
		auto it = std::find_if(m_TemporaryTabs.begin(), m_TemporaryTabs.end(),
		                       [id](auto& t){ return t.ID == id; });
		if (it != m_TemporaryTabs.end())
		{
			it->Close();
			m_TemporaryTabs.erase(it);
		}
		if (m_ActiveTabID == id)
			PickNextActiveTab();
	}

	// ----- Housekeeping -----------------------------------------------------

	/// Call once per frame. Removes expired temporary tabs.
	void PurgeExpiredTabs()
	{
		auto now = std::chrono::steady_clock::now();
		for (auto it = m_TemporaryTabs.begin(); it != m_TemporaryTabs.end(); )
		{
			if (it->Deadline <= now)
			{
				if (m_ActiveTabID == it->ID)
					m_ActiveTabID = -1;
				it->Close();
				it = m_TemporaryTabs.erase(it);
			}
			else
			{
				++it;
			}
		}
		if (m_ActiveTabID == -1)
			PickNextActiveTab();
	}

	/// Close all tabs (call on detach).
	void CloseAll()
	{
		for (auto& t : m_DynamicTabs)   t.Close();
		for (auto& t : m_PermanentTabs) t.Close();
		for (auto& t : m_TemporaryTabs) t.Close();
		m_DynamicTabs.clear();
		m_PermanentTabs.clear();
		m_TemporaryTabs.clear();
		m_ActiveTabID = -1;
	}

	bool HasAnyTab() const
	{
		return !m_DynamicTabs.empty() || !m_PermanentTabs.empty() || !m_TemporaryTabs.empty();
	}

private:
	void PickNextActiveTab()
	{
		// Prefer open tabs.
		for (auto& t : m_DynamicTabs)   if (t.IsOpen) { m_ActiveTabID = t.ID; return; }
		for (auto& t : m_PermanentTabs) if (t.IsOpen) { m_ActiveTabID = t.ID; return; }
		for (auto& t : m_TemporaryTabs) if (t.IsOpen) { m_ActiveTabID = t.ID; return; }
		m_ActiveTabID = -1;
	}

	std::vector<BrowserTab> m_DynamicTabs;
	std::vector<BrowserTab> m_PermanentTabs;
	std::vector<BrowserTab> m_TemporaryTabs;

	int m_NextID      = 1;
	int m_ActiveTabID = -1;
};
