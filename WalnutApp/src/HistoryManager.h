#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct HistoryEntry
{
	std::string url;
	std::string title;
	int visitCount = 0;
	int64_t lastVisit = 0;
};

class HistoryManager
{
public:
	HistoryManager();
	~HistoryManager();

	HistoryManager(const HistoryManager&) = delete;
	HistoryManager& operator=(const HistoryManager&) = delete;

	void AddVisit(const std::string& url, const std::string& title);
	void DeleteEntry(const std::string& url);
	void DeleteAll();

	[[nodiscard]] std::vector<HistoryEntry> Query(const std::string& query, int limit = 8) const;
	[[nodiscard]] std::vector<HistoryEntry> GetAll(int limit = 200) const;
	[[nodiscard]] uint64_t GetVersion() const { return m_version; }

private:
	void OpenOrCreate(const std::string& path);

	uint64_t m_version = 0;
	struct sqlite3* m_db = nullptr;
};
