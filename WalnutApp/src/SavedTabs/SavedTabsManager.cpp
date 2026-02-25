#include "SavedTabsManager.h"
#include "SavedDatabase.h"
#include "../Utils/Sqlite.h"

SavedTabsManager::SavedTabsManager(SavedDatabase& database)
	: m_database(database)
{
	auto stmt = Sqlite::Prepare(m_database.Handle(), "SELECT MAX(sort_order) FROM saved_tabs;");
	if (stmt && sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		m_nextSortOrder = sqlite3_column_int(stmt.get(), 0) + 1;
	}
}

int SavedTabsManager::AddEntry(const std::string& baseUrl)
{
	sqlite3* db = m_database.Handle();
	if (!db || baseUrl.empty())
	{
		return -1;
	}

	auto stmt = Sqlite::Prepare(db,
		"INSERT INTO saved_tabs(base_url, sort_order) VALUES(?1, ?2);");
	if (!stmt)
	{
		return -1;
	}

	sqlite3_bind_text(stmt.get(), 1, baseUrl.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, m_nextSortOrder++);
	sqlite3_step(stmt.get());

	return static_cast<int>(sqlite3_last_insert_rowid(db));
}

void SavedTabsManager::RemoveEntry(int dbId)
{
	sqlite3* db = m_database.Handle();
	if (!db || dbId < 0)
	{
		return;
	}

	auto stmt = Sqlite::Prepare(db, "DELETE FROM saved_tabs WHERE id = ?1;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_int(stmt.get(), 1, dbId);
	sqlite3_step(stmt.get());
}

std::vector<SavedTabRecord> SavedTabsManager::GetAll() const
{
	sqlite3* db = m_database.Handle();
	if (!db)
	{
		return {};
	}

	auto stmt = Sqlite::Prepare(db,
		"SELECT id, base_url, sort_order, folder_id FROM saved_tabs ORDER BY sort_order ASC;");
	if (!stmt)
	{
		return {};
	}

	std::vector<SavedTabRecord> results;
	while (sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		SavedTabRecord record;
		record.dbId = sqlite3_column_int(stmt.get(), 0);
		record.baseUrl = Sqlite::ColumnText(stmt.get(), 1);
		record.sortOrder = sqlite3_column_int(stmt.get(), 2);
		record.folderId = (sqlite3_column_type(stmt.get(), 3) == SQLITE_NULL)
			? -1
			: sqlite3_column_int(stmt.get(), 3);
		results.push_back(std::move(record));
	}

	return results;
}

void SavedTabsManager::MoveTabToFolder(int tabDbId, int folderId)
{
	sqlite3* db = m_database.Handle();
	if (!db || tabDbId < 0)
	{
		return;
	}

	if (folderId < 0)
	{
		auto stmt = Sqlite::Prepare(db,
			"UPDATE saved_tabs SET folder_id = NULL WHERE id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, tabDbId);
			sqlite3_step(stmt.get());
		}
	}
	else
	{
		auto stmt = Sqlite::Prepare(db,
			"UPDATE saved_tabs SET folder_id = ?1 WHERE id = ?2;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, folderId);
			sqlite3_bind_int(stmt.get(), 2, tabDbId);
			sqlite3_step(stmt.get());
		}
	}
}

void SavedTabsManager::SwapTabSortOrders(int tabDbId1, int tabDbId2)
{
	sqlite3* db = m_database.Handle();
	if (!db || tabDbId1 < 0 || tabDbId2 < 0 || tabDbId1 == tabDbId2)
	{
		return;
	}

	auto fetchOrder = [&](int dbId) -> int
	{
		auto stmt = Sqlite::Prepare(db, "SELECT sort_order FROM saved_tabs WHERE id = ?1;");
		if (!stmt)
		{
			return -1;
		}

		sqlite3_bind_int(stmt.get(), 1, dbId);
		if (sqlite3_step(stmt.get()) != SQLITE_ROW)
		{
			return -1;
		}

		return sqlite3_column_int(stmt.get(), 0);
	};

	int order1 = fetchOrder(tabDbId1);
	int order2 = fetchOrder(tabDbId2);
	if (order1 < 0 || order2 < 0)
	{
		return;
	}

	auto updateStmt = Sqlite::Prepare(db,
		"UPDATE saved_tabs SET sort_order = ?1 WHERE id = ?2;");
	if (!updateStmt)
	{
		return;
	}

	sqlite3_bind_int(updateStmt.get(), 1, order2);
	sqlite3_bind_int(updateStmt.get(), 2, tabDbId1);
	sqlite3_step(updateStmt.get());

	sqlite3_reset(updateStmt.get());

	sqlite3_bind_int(updateStmt.get(), 1, order1);
	sqlite3_bind_int(updateStmt.get(), 2, tabDbId2);
	sqlite3_step(updateStmt.get());
}
