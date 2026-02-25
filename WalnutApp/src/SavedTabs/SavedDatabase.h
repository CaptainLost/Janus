#pragma once

#include <string>

struct sqlite3;

class SavedDatabase
{
public:
	SavedDatabase();
	~SavedDatabase();

	SavedDatabase(const SavedDatabase&) = delete;
	SavedDatabase& operator=(const SavedDatabase&) = delete;

	sqlite3* Handle() const;

private:
	void OpenOrCreate(const std::string& path);

	sqlite3* m_db = nullptr;
};
