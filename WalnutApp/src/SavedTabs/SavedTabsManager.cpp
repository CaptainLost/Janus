#include "SavedTabsManager.h"
#include "../Utils/Paths.h"
#include "../Utils/Sqlite.h"

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

	auto tabStmt = Sqlite::Prepare(m_db, "SELECT MAX(sort_order) FROM saved_tabs;");
	if (tabStmt && sqlite3_step(tabStmt.get()) == SQLITE_ROW)
	{
		m_nextSortOrder = sqlite3_column_int(tabStmt.get(), 0) + 1;
	}

	auto folderStmt = Sqlite::Prepare(m_db, "SELECT MAX(sort_order) FROM folders;");
	if (folderStmt && sqlite3_step(folderStmt.get()) == SQLITE_ROW)
	{
		m_nextFolderSortOrder = sqlite3_column_int(folderStmt.get(), 0) + 1;
	}
}

int SavedTabsManager::AddEntry(const std::string& baseUrl)
{
	if (!m_db || baseUrl.empty())
	{
		return -1;
	}

	auto stmt = Sqlite::Prepare(m_db,
		"INSERT INTO saved_tabs(base_url, sort_order) VALUES(?1, ?2);");
	if (!stmt)
	{
		return -1;
	}

	sqlite3_bind_text(stmt.get(), 1, baseUrl.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, m_nextSortOrder++);
	sqlite3_step(stmt.get());

	return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

void SavedTabsManager::RemoveEntry(int dbId)
{
	if (!m_db || dbId < 0)
	{
		return;
	}

	auto stmt = Sqlite::Prepare(m_db, "DELETE FROM saved_tabs WHERE id = ?1;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_int(stmt.get(), 1, dbId);
	sqlite3_step(stmt.get());
}

std::vector<SavedTabRecord> SavedTabsManager::GetAll() const
{
	if (!m_db)
	{
		return {};
	}

	auto stmt = Sqlite::Prepare(m_db,
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

int SavedTabsManager::CreateFolder(const std::string& name, int parentId)
{
	if (!m_db)
	{
		return -1;
	}

	auto stmt = Sqlite::Prepare(m_db,
		"INSERT INTO folders(name, sort_order, color, parent_id) VALUES(?1, ?2, ?3, ?4);");
	if (!stmt)
	{
		return -1;
	}

	constexpr int whiteColor = static_cast<int>(0xFFFFFFFF);
	sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, m_nextFolderSortOrder++);
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

	return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

void SavedTabsManager::DeleteFolder(int folderId)
{
	if (!m_db || folderId < 0)
	{
		return;
	}

	// Get the parent of the folder being deleted so children can be re-parented
	int grandparentId = -1;
	{
		auto stmt = Sqlite::Prepare(m_db, "SELECT parent_id FROM folders WHERE id = ?1;");
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

	// Move child folders up to grandparent
	if (grandparentId < 0)
	{
		auto stmt = Sqlite::Prepare(m_db,
			"UPDATE folders SET parent_id = NULL WHERE parent_id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, folderId);
			sqlite3_step(stmt.get());
		}
	}
	else
	{
		auto stmt = Sqlite::Prepare(m_db,
			"UPDATE folders SET parent_id = ?1 WHERE parent_id = ?2;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, grandparentId);
			sqlite3_bind_int(stmt.get(), 2, folderId);
			sqlite3_step(stmt.get());
		}
	}

	auto unassignStmt = Sqlite::Prepare(m_db,
		"UPDATE saved_tabs SET folder_id = NULL WHERE folder_id = ?1;");
	if (unassignStmt)
	{
		sqlite3_bind_int(unassignStmt.get(), 1, folderId);
		sqlite3_step(unassignStmt.get());
	}

	auto deleteStmt = Sqlite::Prepare(m_db, "DELETE FROM folders WHERE id = ?1;");
	if (deleteStmt)
	{
		sqlite3_bind_int(deleteStmt.get(), 1, folderId);
		sqlite3_step(deleteStmt.get());
	}
}

void SavedTabsManager::MoveFolder(int folderId, int newParentId)
{
	if (!m_db || folderId < 0)
	{
		return;
	}

	if (newParentId < 0)
	{
		auto stmt = Sqlite::Prepare(m_db,
			"UPDATE folders SET parent_id = NULL WHERE id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, folderId);
			sqlite3_step(stmt.get());
		}
	}
	else
	{
		auto stmt = Sqlite::Prepare(m_db,
			"UPDATE folders SET parent_id = ?1 WHERE id = ?2;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, newParentId);
			sqlite3_bind_int(stmt.get(), 2, folderId);
			sqlite3_step(stmt.get());
		}
	}
}

void SavedTabsManager::RenameFolder(int folderId, const std::string& name)
{
	if (!m_db || folderId < 0)
	{
		return;
	}

	auto stmt = Sqlite::Prepare(m_db, "UPDATE folders SET name = ?1 WHERE id = ?2;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt.get(), 2, folderId);
	sqlite3_step(stmt.get());
}

void SavedTabsManager::SetFolderColor(int folderId, ImColor color)
{
	if (!m_db || folderId < 0)
	{
		return;
	}

	auto r = static_cast<uint8_t>(color.Value.x * 255.0f);
	auto g = static_cast<uint8_t>(color.Value.y * 255.0f);
	auto b = static_cast<uint8_t>(color.Value.z * 255.0f);
	auto a = static_cast<uint8_t>(color.Value.w * 255.0f);
	int packed = (r << 24) | (g << 16) | (b << 8) | a;

	auto stmt = Sqlite::Prepare(m_db, "UPDATE folders SET color = ?1 WHERE id = ?2;");
	if (!stmt)
	{
		return;
	}

	sqlite3_bind_int(stmt.get(), 1, packed);
	sqlite3_bind_int(stmt.get(), 2, folderId);
	sqlite3_step(stmt.get());
}

void SavedTabsManager::MoveTabToFolder(int tabDbId, int folderId)
{
	if (!m_db || tabDbId < 0)
	{
		return;
	}

	if (folderId < 0)
	{
		auto stmt = Sqlite::Prepare(m_db,
			"UPDATE saved_tabs SET folder_id = NULL WHERE id = ?1;");
		if (stmt)
		{
			sqlite3_bind_int(stmt.get(), 1, tabDbId);
			sqlite3_step(stmt.get());
		}
	}
	else
	{
		auto stmt = Sqlite::Prepare(m_db,
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
	if (!m_db || tabDbId1 < 0 || tabDbId2 < 0 || tabDbId1 == tabDbId2)
	{
		return;
	}

	auto fetchOrder = [&](int dbId) -> int
	{
		auto stmt = Sqlite::Prepare(m_db, "SELECT sort_order FROM saved_tabs WHERE id = ?1;");
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

	auto updateStmt = Sqlite::Prepare(m_db,
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

std::vector<SavedFolderRecord> SavedTabsManager::GetAllFolders() const
{
	if (!m_db)
	{
		return {};
	}

	auto stmt = Sqlite::Prepare(m_db,
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

		int packed = sqlite3_column_int(stmt.get(), 3);
		float r = ((packed >> 24) & 0xFF) / 255.0f;
		float g = ((packed >> 16) & 0xFF) / 255.0f;
		float b = ((packed >> 8)  & 0xFF) / 255.0f;
		float a = (packed & 0xFF) / 255.0f;
		record.color = ImColor(r, g, b, a);

		results.push_back(std::move(record));
	}

	return results;
}
