#include "SavedDatabase.h"
#include "../Utils/Paths.h"
#include "../Utils/Sqlite.h"

SavedDatabase::SavedDatabase()
{
	OpenOrCreate(Paths::SavedTabsDb().string());
}

SavedDatabase::~SavedDatabase()
{
	sqlite3_close(m_db);
}

sqlite3* SavedDatabase::Handle() const
{
	return m_db;
}

void SavedDatabase::OpenOrCreate(const std::string& path)
{
	if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK)
	{
		return;
	}

	constexpr const char* sql =
		"CREATE TABLE IF NOT EXISTS saved_tabs ("
		"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  base_url   TEXT NOT NULL,"
		"  sort_order INTEGER NOT NULL DEFAULT 0,"
		"  folder_id  INTEGER DEFAULT NULL"
		");"
		"CREATE TABLE IF NOT EXISTS folders ("
		"  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  name       TEXT NOT NULL,"
		"  sort_order INTEGER NOT NULL DEFAULT 0,"
		"  color      INTEGER NOT NULL DEFAULT -1"
		");";

	sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);
}
