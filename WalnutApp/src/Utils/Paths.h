#pragma once

#include <ShlObj.h>
#include <filesystem>
#include <string_view>

namespace Paths
{
	inline constexpr std::string_view kAppName = "Janus";
	inline constexpr std::string_view kHistoryDbName = "history.db";
	inline constexpr std::string_view kSavedTabsDbName = "saved_tabs.db";

	inline const std::filesystem::path& AppDataDir()
	{
		static const std::filesystem::path dir = []() -> std::filesystem::path {
			PWSTR raw = nullptr;

			if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw)))
			{
				return std::filesystem::current_path();
			}

			std::filesystem::path result = std::filesystem::path(raw) / kAppName;
			CoTaskMemFree(raw);
			std::filesystem::create_directories(result);

			return result;
		}();

		return dir;
	}

	inline std::filesystem::path HistoryDb()
	{
		return AppDataDir() / kHistoryDbName;
	}

	inline std::filesystem::path SavedTabsDb()
	{
		return AppDataDir() / kSavedTabsDbName;
	}
}
