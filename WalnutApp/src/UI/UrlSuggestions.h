#pragma once

#include "../History/HistoryManager.h"
#include "../Browser/Tab.h"

#include "imgui.h"

#include <string>
#include <vector>

class UrlSuggestions
{
public:
	void Update(const std::string& query, HistoryManager* history);
	void HandleKeyboard();
	void RenderDropdown(Tab* tab, ImVec2 inputMin, ImVec2 inputMax);
	void Clear();

	bool HasSuggestions() const { return !m_suggestions.empty(); }
	bool IsDropdownHovered() const { return m_dropdownHovered; }
	std::string GetSelectedUrl() const;

	void SelectNextSuggestion();
	void SelectPreviousSuggestion();

private:
	std::vector<HistoryEntry> m_suggestions;
	std::string m_lastQuery;
	int m_selectedSuggestion = -1;
	bool m_dropdownHovered = false;
};
