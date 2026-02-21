#include "HistoryManager.h"
#include "../Utils/Paths.h"
#include "../Utils/Sqlite.h"

namespace
{
	HistoryEntry ReadEntry(sqlite3_stmt* stmt)
	{
		return {
			.url = Sqlite::ColumnText(stmt, 0),
			.title = Sqlite::ColumnText(stmt, 1),
			.visitCount = sqlite3_column_int(stmt, 2),
			.lastVisit = sqlite3_column_int64(stmt, 3),
		};
	}
}

HistoryManager::HistoryManager()
{
	OpenOrCreate(Paths::HistoryDb().string());
}

HistoryManager::~HistoryManager()
{
	sqlite3_close(m_db);
}

void HistoryManager::OpenOrCreate(const std::string& path)
{
	if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK)
	{
		return;
	}

	constexpr const char* sql =
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
	{
		return;
	}

	constexpr const char* sql =
		"INSERT INTO visits(url, title, visit_count, last_visit) VALUES(?1, ?2, 1, strftime('%s','now'))"
		" ON CONFLICT(url) DO UPDATE SET"
		"  title       = excluded.title,"
		"  visit_count = visit_count + 1,"
		"  last_visit  = excluded.last_visit;";

	auto stmt = Sqlite::Prepare(m_db, sql);
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_text(stmt.get(), 1, url.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt.get(), 2, title.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt.get());
	m_version++;
}

void HistoryManager::DeleteEntry(const std::string& url)
{
	if (!m_db || url.empty())
	{
		return;
	}

	auto stmt = Sqlite::Prepare(m_db, "DELETE FROM visits WHERE url = ?1;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_text(stmt.get(), 1, url.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt.get());
	m_version++;
}

void HistoryManager::DeleteAll()
{
	if (!m_db)
	{
		return;
	}

	sqlite3_exec(m_db, "DELETE FROM visits;", nullptr, nullptr, nullptr);
	m_version++;
}

std::vector<HistoryEntry> HistoryManager::GetAll(int limit) const
{
	if (!m_db)
	{
		return {};
	}

	constexpr const char* sql =
		"SELECT url, title, visit_count, last_visit FROM visits"
		" ORDER BY last_visit DESC"
		" LIMIT ?1;";

	auto stmt = Sqlite::Prepare(m_db, sql);
	if (!stmt)
	{
		return {};
	}

	sqlite3_bind_int(stmt.get(), 1, limit);

	std::vector<HistoryEntry> results;
	while (sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		results.push_back(ReadEntry(stmt.get()));
	}

	return results;
}

std::vector<HistoryEntry> HistoryManager::Query(const std::string& query, int limit) const
{
	if (!m_db || query.empty())
	{
		return {};
	}

	constexpr const char* sql =
		"SELECT url, title, visit_count, last_visit FROM visits"
		" WHERE url LIKE ?1 OR title LIKE ?1"
		" ORDER BY visit_count DESC, last_visit DESC"
		" LIMIT ?2;";

	auto stmt = Sqlite::Prepare(m_db, sql);
	if (!stmt)
	{
		return {};
	}

	std::string pattern = "%" + query + "%";
	sqlite3_bind_text(stmt.get(), 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, limit);

	std::vector<HistoryEntry> results;
	while (sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		results.push_back(ReadEntry(stmt.get()));
	}

	return results;
}
