#include "UrlSuggestions.h"

#include <IconsFontAwesome6.h>
#include <algorithm>

#include "imgui_internal.h"

void UrlSuggestions::Update(const std::string& query, HistoryManager* history)
{
	if (m_lastQuery == query)
	{
		return;
	}

	m_lastQuery = query;
	m_selectedSuggestion = -1;

	if (query.empty() || !history)
	{
		m_suggestions.clear();
		return;
	}

	m_suggestions = history->Query(query);
}

void UrlSuggestions::HandleKeyboard()
{
	if (m_suggestions.empty())
	{
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		Clear();
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
	{
		SelectPreviousSuggestion();
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
	{
		SelectNextSuggestion();
	}
}

void UrlSuggestions::RenderDropdown(Tab* tab, ImVec2 inputMin, ImVec2 inputMax)
{
	if (m_suggestions.empty())
	{
		return;
	}

	const float width = inputMax.x - inputMin.x;
	const float maxHeight = 8.0f * ImGui::GetFrameHeightWithSpacing();

	ImGui::SetNextWindowPos(ImVec2(inputMin.x, inputMax.y));
	ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, maxHeight));
	ImGui::SetNextWindowBgAlpha(1.0f);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::Begin("##url_suggestions", nullptr, flags);
	ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
	m_dropdownHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	for (int i = 0; i < static_cast<int>(m_suggestions.size()); i++)
	{
		const HistoryEntry& entry = m_suggestions[i];
		bool selected = (i == m_selectedSuggestion);

		std::string label = ICON_FA_CLOCK_ROTATE_LEFT "  ";
		label += entry.title.empty() ? entry.url : entry.title + "  \xe2\x80\x94  " + entry.url;

		ImGui::PushID(i);
		if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_None))
		{
			tab->Open(entry.url);
			Clear();
		}
		ImGui::PopID();
	}

	ImGui::End();
}

void UrlSuggestions::Clear()
{
	m_suggestions.clear();
	m_lastQuery.clear();
	m_selectedSuggestion = -1;
	m_dropdownHovered = false;
}

std::string UrlSuggestions::GetSelectedUrl() const
{
	if (m_selectedSuggestion >= 0 && m_selectedSuggestion < static_cast<int>(m_suggestions.size()))
	{
		return m_suggestions[m_selectedSuggestion].url;
	}

	return {};
}

void UrlSuggestions::SelectNextSuggestion()
{
	m_selectedSuggestion = std::min(m_selectedSuggestion + 1, static_cast<int>(m_suggestions.size()) - 1);
}

void UrlSuggestions::SelectPreviousSuggestion()
{
	m_selectedSuggestion = std::max(m_selectedSuggestion - 1, -1);
}
