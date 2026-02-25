#pragma once

#include <string>
#include <vector>

struct SavedTabRecord
{
	int dbId = -1;
	std::string baseUrl;
	int sortOrder = 0;
	int folderId = -1;
};

class SavedDatabase;

class SavedTabsManager
{
public:
	explicit SavedTabsManager(SavedDatabase& database);

	int AddEntry(const std::string& baseUrl);
	void RemoveEntry(int dbId);
	void MoveTabToFolder(int tabDbId, int folderId);
	void SwapTabSortOrders(int tabDbId1, int tabDbId2);

	std::vector<SavedTabRecord> GetAll() const;

private:
	SavedDatabase& m_database;
	int m_nextSortOrder = 0;
};
