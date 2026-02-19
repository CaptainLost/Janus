#pragma once

#include "../History/HistoryManager.h"
#include "../Browser/BrowserTab2.h"

#include "imgui.h"

#include <string>
#include <vector>

class UrlSuggestions
{
public:
	void Update(const std::string& query, HistoryManager* history);
	void HandleKeyboard();
	void RenderDropdown(BrowserTab2* tab, ImVec2 inputMin, ImVec2 inputMax);
	void Clear();

	bool HasSuggestions() const { return !m_suggestions.empty(); }
	std::string GetSelectedUrl() const;

private:
	std::vector<HistoryEntry> m_suggestions;
	std::string m_lastQuery;
	int m_selectedSuggestion = -1;
};
