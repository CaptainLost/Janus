#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SavedTabRecord
{
	int dbId = -1;
	std::string baseUrl;
	int sortOrder = 0;
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

private:
	void OpenOrCreate(const std::string& path);

	struct sqlite3* m_db = nullptr;
	int m_nextSortOrder = 0;
};
