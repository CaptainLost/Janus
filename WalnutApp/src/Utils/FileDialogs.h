#pragma once

#pragma comment(lib, "comdlg32.lib")
#include <windows.h>
#include <commdlg.h>

#include <string>

namespace FileDialogs
{
	inline std::string ShowSaveImageDialog(const std::string& sourceUrl)
	{
		std::string suggestedName = "image";
		size_t lastSlash = sourceUrl.find_last_of('/');
		if (lastSlash != std::string::npos && lastSlash + 1 < sourceUrl.size())
		{
			suggestedName = sourceUrl.substr(lastSlash + 1);
			size_t queryPos = suggestedName.find('?');
			if (queryPos != std::string::npos)
			{
				suggestedName = suggestedName.substr(0, queryPos);
			}
		}

		wchar_t filePath[MAX_PATH] = {};
		MultiByteToWideChar(CP_UTF8, 0, suggestedName.c_str(), -1, filePath, MAX_PATH);

		OPENFILENAMEW ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner   = nullptr;
		ofn.lpstrFilter = L"Image Files\0*.png;*.jpg;*.jpeg;*.gif;*.webp;*.svg;*.bmp\0All Files\0*.*\0\0";
		ofn.lpstrFile   = filePath;
		ofn.nMaxFile    = MAX_PATH;
		ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
		ofn.lpstrTitle  = L"Save Image As";

		if (!GetSaveFileNameW(&ofn))
		{
			return {};
		}

		int size = WideCharToMultiByte(CP_UTF8, 0, filePath, -1, nullptr, 0, nullptr, nullptr);
		std::string result(size - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, filePath, -1, result.data(), size, nullptr, nullptr);
		return result;
	}
}
