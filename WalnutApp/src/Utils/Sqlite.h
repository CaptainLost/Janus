#pragma once

#include "sqlite3.h"

#include <memory>
#include <string>

namespace Sqlite
{
	struct StmtDeleter
	{
		void operator()(sqlite3_stmt* stmt) const { sqlite3_finalize(stmt); }
	};

	using StmtPtr = std::unique_ptr<sqlite3_stmt, StmtDeleter>;

	inline StmtPtr Prepare(sqlite3* db, const char* sql)
	{
		sqlite3_stmt* raw = nullptr;

		if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK)
		{
			return {};
		}

		return StmtPtr{raw};
	}

	inline std::string ColumnText(sqlite3_stmt* stmt, int col)
	{
		const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
		return text ? text : "";
	}
}
