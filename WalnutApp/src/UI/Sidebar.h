#pragma once

#include "../Browser/TabManager.h"
#include "../SavedTabs/SavedTabsManager.h"

#include <unordered_map>
#include <vector>

class SavedBrowserTab;

class Sidebar
{
public:
	void Load(SavedTabsManager& savedTabsManager);
	void Render(TabManager& tabManager, SavedTabsManager& savedTabsManager);

private:
	void RenderSavedSection(TabManager& tabManager, SavedTabsManager& savedTabsManager);
	void RenderAddSavedPopup(SavedTabsManager& savedTabsManager);
	void RenderTemporarySection(TabManager& tabManager);

	SavedBrowserTab* FindSavedTab(int dbId, TabManager& tabManager);

	std::vector<SavedTabRecord> m_savedEntries;
	std::unordered_map<int, std::string> m_savedLastUrls;
	char m_savedUrlBuffer[512] = {};
};
