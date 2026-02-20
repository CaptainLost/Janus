#pragma once

#include "../Browser/TabManager2.h"
#include "../SavedTabs/SavedTabsManager.h"

#include <unordered_map>
#include <vector>

class Sidebar
{
public:
	void Load(SavedTabsManager& savedTabsManager);
	void Render(TabManager2& tabManager, SavedTabsManager& savedTabsManager);

private:
	void RenderSavedSection(TabManager2& tabManager, SavedTabsManager& savedTabsManager);
	void RenderAddSavedPopup(SavedTabsManager& savedTabsManager);
	void RenderTemporarySection(TabManager2& tabManager);

	BrowserTab2* ResolveTab(int dbId, TabManager2& tabManager);

	std::vector<SavedTabRecord> m_savedEntries;
	std::unordered_map<int, int> m_savedDbIdToTabId;
	std::unordered_map<int, std::string> m_savedLastUrls;
	char m_savedUrlBuffer[512] = {};
};
