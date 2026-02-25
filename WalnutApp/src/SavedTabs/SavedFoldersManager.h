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

class SavedDatabase;

class SavedFoldersManager
{
public:
	explicit SavedFoldersManager(SavedDatabase& database);

	int CreateFolder(const std::string& name, int parentId = -1);
	void DeleteFolder(int folderId);
	void RenameFolder(int folderId, const std::string& name);
	void SetFolderColor(int folderId, ImColor color);
	void MoveFolder(int folderId, int newParentId);

	std::vector<SavedFolderRecord> GetAll() const;

private:
	SavedDatabase& m_database;
	int m_nextSortOrder = 0;
};
