#pragma once

#include "imgui.h"

#include <cstdint>
#include <string>
#include <vector>

struct SavedFolderRecord
{
	int dbId = -1;
	std::string name;
	int sortOrder = 0;
	int parentId = -1;
	ImColor color = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
};

struct SavedTabRecord
{
	int dbId = -1;
	std::string baseUrl;
	int sortOrder = 0;
	int folderId = -1;
};

class SavedTabsManager
{
public:
	SavedTabsManager();
	~SavedTabsManager();

	SavedTabsManager(const SavedTabsManager&) = delete;
	SavedTabsManager& operator=(const SavedTabsManager&) = delete;

	int AddEntry(const std::string& baseUrl);
	void RemoveEntry(int dbId);

	std::vector<SavedTabRecord> GetAll() const;

	int CreateFolder(const std::string& name, int parentId = -1);
	void DeleteFolder(int folderId);
	void RenameFolder(int folderId, const std::string& name);
	void SetFolderColor(int folderId, ImColor color);
	void MoveFolder(int folderId, int newParentId);
	void MoveTabToFolder(int tabDbId, int folderId);
	void SwapTabSortOrders(int tabDbId1, int tabDbId2);
	std::vector<SavedFolderRecord> GetAllFolders() const;

private:
	void OpenOrCreate(const std::string& path);

	struct sqlite3* m_db = nullptr;
	int m_nextSortOrder = 0;
	int m_nextFolderSortOrder = 0;
};
