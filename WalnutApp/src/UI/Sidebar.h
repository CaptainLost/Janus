#pragma once

#include "../Browser/TabManager.h"
#include "../SavedTabs/SavedTabsManager.h"

#include "imgui.h"

#include <vector>

class SavedBrowserTab;

class Sidebar
{
public:
	void Load(SavedTabsManager& savedTabsManager);
	void Render(TabManager& tabManager, SavedTabsManager& savedTabsManager);

private:
	void RenderSavedSection(TabManager& tabManager, SavedTabsManager& savedTabsManager);
	void RenderFolderNode(const SavedFolderRecord& folder,
		TabManager& tabManager, SavedTabsManager& savedTabsManager, int& outRemoveTabDbId);
	void RenderSavedTabItem(const SavedTabRecord& entry,
		TabManager& tabManager, SavedTabsManager& savedTabsManager, int& outRemoveTabDbId);
	void RenderAddSavedPopup(SavedTabsManager& savedTabsManager);
	void RenderNewFolderPopup(SavedTabsManager& savedTabsManager);
	void RenderEditFolderPopup(SavedTabsManager& savedTabsManager);
	void RenderTemporarySection(TabManager& tabManager);

	SavedBrowserTab* FindSavedTab(int dbId, TabManager& tabManager);
	bool IsFolderDescendant(int ancestorId, int nodeId) const;

	std::vector<SavedTabRecord> m_savedEntries;
	std::vector<SavedFolderRecord> m_folders;

	char m_savedUrlBuffer[512] = {};

	char m_newFolderNameBuffer[256] = {};
	ImColor m_newFolderColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
	int m_newFolderParentId = -1;

	int m_editingFolderId = -1;
	char m_editFolderNameBuffer[256] = {};
	ImColor m_editFolderColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);

	bool m_pendingOpenEditFolder = false;
	bool m_pendingOpenNewFolder = false;
};
