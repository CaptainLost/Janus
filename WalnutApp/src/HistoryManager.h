#pragma once

#include <string>
#include <vector>

struct HistoryEntry
{
	std::string url;
	std::string title;
	int visitCount = 0;
};

class HistoryManager
{
public:
	HistoryManager();
	~HistoryManager();

	void AddVisit(const std::string& url, const std::string& title);
	std::vector<HistoryEntry> Query(const std::string& prefix, int limit = 8) const;

private:
	void OpenOrCreate(const std::string& path);

	struct sqlite3* m_db = nullptr;
};
