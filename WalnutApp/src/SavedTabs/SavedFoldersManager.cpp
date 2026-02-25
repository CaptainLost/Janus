#include "SavedFoldersManager.h"
#include "SavedDatabase.h"
#include "../Utils/Sqlite.h"
#include "../Utils/ColorUtils.h"

SavedFoldersManager::SavedFoldersManager(SavedDatabase& database)
	: m_database(database)
{
	auto stmt = Sqlite::Prepare(m_database.Handle(), "SELECT MAX(sort_order) FROM folders;");
	if (stmt && sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		m_nextSortOrder = sqlite3_column_int(stmt.get(), 0) + 1;
	}
}

int SavedFoldersManager::CreateFolder(const std::string& name, int parentId)
{
	sqlite3* db = m_database.Handle();
	if (!db)
	{
		return -1;
	}

	auto stmt = Sqlite::Prepare(db,
		"INSERT INTO folders(name, sort_order, color, parent_id) VALUES(?1, ?2, ?3, ?4);");
	if (!stmt)
	{
		return -1;
	}

	constexpr int whiteColor = static_cast<int>(0xFFFFFFFF);
	sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, m_nextSortOrder++);
	sqlite3_bind_int(stmt.get(), 3, whiteColor);

	if (parentId < 0)
	{
		sqlite3_bind_null(stmt.get(), 4);
	}
	else
	{
		sqlite3_bind_int(stmt.get(), 4, parentId);
	}

	sqlite3_step(stmt.get());

	return static_cast<int>(sqlite3_last_insert_rowid(db));
}

void SavedFoldersManager::DeleteFolder(int folderId)
{
	sqlite3* db = m_database.Handle();
	if (!db || folderId < 0)
	{
		return;
	}

	int grandparentId = -1;
	{
		auto stmt = Sqlite::Prepare(db, "SELECT parent_id FROM folders WHERE id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, folderId);
			if (sqlite3_step(stmt.get()) == SQLITE_ROW)
			{
				grandparentId = (sqlite3_column_type(stmt.get(), 0) == SQLITE_NULL)
					? -1 : sqlite3_column_int(stmt.get(), 0);
			}
		}
	}

	if (grandparentId < 0)
	{
		auto stmt = Sqlite::Prepare(db,
			"UPDATE folders SET parent_id = NULL WHERE parent_id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, folderId);
			sqlite3_step(stmt.get());
		}
	}
	else
	{
		auto stmt = Sqlite::Prepare(db,
			"UPDATE folders SET parent_id = ?1 WHERE parent_id = ?2;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, grandparentId);
			sqlite3_bind_int(stmt.get(), 2, folderId);
			sqlite3_step(stmt.get());
		}
	}

	auto unassignStmt = Sqlite::Prepare(db,
		"UPDATE saved_tabs SET folder_id = NULL WHERE folder_id = ?1;");
	if (unassignStmt)
	{
		sqlite3_bind_int(unassignStmt.get(), 1, folderId);
		sqlite3_step(unassignStmt.get());
	}

	auto deleteStmt = Sqlite::Prepare(db, "DELETE FROM folders WHERE id = ?1;");
	if (deleteStmt)
	{
		sqlite3_bind_int(deleteStmt.get(), 1, folderId);
		sqlite3_step(deleteStmt.get());
	}
}

void SavedFoldersManager::MoveFolder(int folderId, int newParentId)
{
	sqlite3* db = m_database.Handle();
	if (!db || folderId < 0)
	{
		return;
	}

	if (newParentId < 0)
	{
		auto stmt = Sqlite::Prepare(db,
			"UPDATE folders SET parent_id = NULL WHERE id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, folderId);
			sqlite3_step(stmt.get());
		}
	}
	else
	{
		auto stmt = Sqlite::Prepare(db,
			"UPDATE folders SET parent_id = ?1 WHERE id = ?2;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, newParentId);
			sqlite3_bind_int(stmt.get(), 2, folderId);
			sqlite3_step(stmt.get());
		}
	}
}

void SavedFoldersManager::RenameFolder(int folderId, const std::string& name)
{
	sqlite3* db = m_database.Handle();
	if (!db || folderId < 0)
	{
		return;
	}

	auto stmt = Sqlite::Prepare(db, "UPDATE folders SET name = ?1 WHERE id = ?2;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, folderId);
	sqlite3_step(stmt.get());
}

void SavedFoldersManager::SetFolderColor(int folderId, ImColor color)
{
	sqlite3* db = m_database.Handle();
	if (!db || folderId < 0)
	{
		return;
	}

	int packed = ColorUtils::PackColor(color);

	auto stmt = Sqlite::Prepare(db, "UPDATE folders SET color = ?1 WHERE id = ?2;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_int(stmt.get(), 1, packed);
	sqlite3_bind_int(stmt.get(), 2, folderId);
	sqlite3_step(stmt.get());
}

std::vector<SavedFolderRecord> SavedFoldersManager::GetAll() const
{
	sqlite3* db = m_database.Handle();
	if (!db)
	{
		return {};
	}

	auto stmt = Sqlite::Prepare(db,
		"SELECT id, name, sort_order, color, parent_id FROM folders ORDER BY sort_order ASC;");
	if (!stmt)
	{
		return {};
	}

	std::vector<SavedFolderRecord> results;
	while (sqlite3_step(stmt.get()) == SQLITE_ROW)
	{
		SavedFolderRecord record;
		record.dbId = sqlite3_column_int(stmt.get(), 0);
		record.name = Sqlite::ColumnText(stmt.get(), 1);
		record.sortOrder = sqlite3_column_int(stmt.get(), 2);
		record.parentId = (sqlite3_column_type(stmt.get(), 4) == SQLITE_NULL)
			? -1 : sqlite3_column_int(stmt.get(), 4);
		record.color = ColorUtils::UnpackColor(sqlite3_column_int(stmt.get(), 3));

		results.push_back(std::move(record));
	}

	return results;
}
