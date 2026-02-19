#include "HistoryManager.h"

#include "sqlite3.h"

#include <ShlObj.h>
#include <filesystem>

static std::string GetHistoryDbPath()
{
	PWSTR appDataPath = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath)))
	{
		std::filesystem::path dir = std::filesystem::path(appDataPath) / "Janus";
		CoTaskMemFree(appDataPath);
		std::filesystem::create_directories(dir);
		return (dir / "history.db").string();
	}
	return "history.db";
}

HistoryManager::HistoryManager()
{
	OpenOrCreate(GetHistoryDbPath());
}

HistoryManager::~HistoryManager()
{
	if (m_db)
		sqlite3_close(m_db);
}

void HistoryManager::OpenOrCreate(const std::string& path)
{
	if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK)
		return;

	const char* sql =
		"CREATE TABLE IF NOT EXISTS visits ("
		"  url         TEXT PRIMARY KEY,"
		"  title       TEXT,"
		"  visit_count INTEGER NOT NULL DEFAULT 1,"
		"  last_visit  INTEGER NOT NULL"
		");";

	sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);
}

void HistoryManager::AddVisit(const std::string& url, const std::string& title)
{
	if (!m_db || url.empty())
		return;

	const char* sql =
		"INSERT INTO visits(url, title, visit_count, last_visit) VALUES(?1, ?2, 1, strftime('%s','now'))"
		"ON CONFLICT(url) DO UPDATE SET"
		"  title       = excluded.title,"
		"  visit_count = visit_count + 1,"
		"  last_visit  = excluded.last_visit;";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return;

	sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

std::vector<HistoryEntry> HistoryManager::Query(const std::string& prefix, int limit) const
{
	std::vector<HistoryEntry> results;
	if (!m_db || prefix.empty())
		return results;

	const char* sql =
		"SELECT url, title, visit_count FROM visits"
		" WHERE url LIKE ?1 OR title LIKE ?1"
		" ORDER BY visit_count DESC, last_visit DESC"
		" LIMIT ?2;";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return results;

	std::string pattern = "%" + prefix + "%";
	sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, limit);

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		HistoryEntry e;
		auto col = [&](int i) -> std::string {
			const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
			return s ? s : "";
		};
		e.url        = col(0);
		e.title      = col(1);
		e.visitCount = sqlite3_column_int(stmt, 2);
		results.push_back(std::move(e));
	}

	sqlite3_finalize(stmt);
	return results;
}
