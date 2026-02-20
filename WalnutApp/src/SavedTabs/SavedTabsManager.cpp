#include "SavedTabsManager.h"
#include "../Core/Paths.h"
#include "../Core/Sqlite.h"

SavedTabsManager::SavedTabsManager()
{
	OpenOrCreate(Paths::SavedTabsDb().string());
}

SavedTabsManager::~SavedTabsManager()
{
	sqlite3_close(m_db);
}

void SavedTabsManager::OpenOrCreate(const std::string& path)
{
	if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK)
		return;

	constexpr const char* sql =
		"CREATE TABLE IF NOT EXISTS saved_tabs ("
		"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  base_url   TEXT NOT NULL,"
		"  sort_order INTEGER NOT NULL DEFAULT 0"
		");";

	sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);

	auto stmt = Sqlite::Prepare(m_db, "SELECT MAX(sort_order) FROM saved_tabs;");
	if (stmt && sqlite3_step(stmt.get()) == SQLITE_ROW)
		m_nextSortOrder = sqlite3_column_int(stmt.get(), 0) + 1;
}

int SavedTabsManager::AddEntry(const std::string& baseUrl)
{
	if (!m_db || baseUrl.empty())
		return -1;

	auto stmt = Sqlite::Prepare(m_db,
		"INSERT INTO saved_tabs(base_url, sort_order) VALUES(?1, ?2);");
	if (!stmt)
		return -1;

	sqlite3_bind_text(stmt.get(), 1, baseUrl.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, m_nextSortOrder++);
	sqlite3_step(stmt.get());

	return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

void SavedTabsManager::RemoveEntry(int dbId)
{
	if (!m_db || dbId < 0)
		return;

	auto stmt = Sqlite::Prepare(m_db, "DELETE FROM saved_tabs WHERE id = ?1;");
	if (!stmt)
		return;

	sqlite3_bind_int(stmt.get(), 1, dbId);
	sqlite3_step(stmt.get());
}

std::vector<SavedTabRecord> SavedTabsManager::GetAll() const
{
	if (!m_db)
		return {};

	auto stmt = Sqlite::Prepare(m_db,
		"SELECT id, base_url, sort_order FROM saved_tabs ORDER BY sort_order ASC;");
	if (!stmt)
		return {};

	std::vector<SavedTabRecord> results;
	while (sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		SavedTabRecord record;
		record.dbId = sqlite3_column_int(stmt.get(), 0);
		record.baseUrl = Sqlite::ColumnText(stmt.get(), 1);
		record.sortOrder = sqlite3_column_int(stmt.get(), 2);
		results.push_back(std::move(record));
	}

	return results;
}
