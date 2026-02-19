#include "HistoryWindow.h"

#include <IconsFontAwesome6.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <iterator>

constexpr const char* kDays[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

constexpr const char* kMonths[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

static std::string GetDayLabel(int64_t timestamp)
{
	time_t entryTimestamp = static_cast<time_t>(timestamp);
	tm entryTime = {};
	localtime_s(&entryTime, &entryTimestamp);

	time_t now = time(nullptr);
	tm nowTime = {};
	localtime_s(&nowTime, &now);

	if (entryTime.tm_year == nowTime.tm_year && entryTime.tm_yday == nowTime.tm_yday)
		return "Today";

	tm yesterdayTime = nowTime;
	yesterdayTime.tm_mday--;
	mktime(&yesterdayTime);

	if (entryTime.tm_year == yesterdayTime.tm_year && entryTime.tm_yday == yesterdayTime.tm_yday)
		return "Yesterday";

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%s, %s %d, %d",
		kDays[entryTime.tm_wday],
		kMonths[entryTime.tm_mon],
		entryTime.tm_mday,
		entryTime.tm_year + 1900);
	return buffer;
}

void HistoryWindow::Open()
{
	if (m_open)
		return;

	m_open = true;
	memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
	m_lastSearch = "";
	m_lastSeenVersion = UINT64_MAX;
	m_entries.clear();
	m_selectedUrls.clear();
}

void HistoryWindow::Render(TabManager2& tabManager, HistoryManager& history, ImGuiID dockspaceId)
{
	if (!m_open)
		return;

	ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_Once);

	if (!ImGui::Begin(ICON_FA_CLOCK_ROTATE_LEFT " History###HistoryWindow", &m_open, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	// Top bar
	const int selectedCount = static_cast<int>(m_selectedUrls.size());

	char deleteLabel[64];
	if (selectedCount > 0)
		snprintf(deleteLabel, sizeof(deleteLabel), ICON_FA_TRASH " Delete (%d)", selectedCount);
	else
		snprintf(deleteLabel, sizeof(deleteLabel), ICON_FA_TRASH " Delete");

	float refreshButtonWidth = ImGui::CalcTextSize(ICON_FA_ROTATE_RIGHT).x + ImGui::GetStyle().FramePadding.x * 2;
	float deleteButtonWidth  = ImGui::CalcTextSize(deleteLabel).x + ImGui::GetStyle().FramePadding.x * 2;
	float clearButtonWidth   = ImGui::CalcTextSize(ICON_FA_TRASH_CAN " Clear All").x + ImGui::GetStyle().FramePadding.x * 2;
	float rightButtonsWidth  = deleteButtonWidth + clearButtonWidth + ImGui::GetStyle().ItemSpacing.x;

	float searchWidth = ImGui::GetContentRegionAvail().x - refreshButtonWidth - rightButtonsWidth - ImGui::GetStyle().ItemSpacing.x * 2;

	if (ImGui::Button(ICON_FA_ROTATE_RIGHT "##history_refresh"))
		m_lastSeenVersion = UINT64_MAX;

	ImGui::SameLine();
	ImGui::SetNextItemWidth(searchWidth);
	ImGui::InputTextWithHint("##history_search", ICON_FA_MAGNIFYING_GLASS " Search...", m_searchBuffer, sizeof(m_searchBuffer));

	ImGui::SameLine();
	ImGui::BeginDisabled(selectedCount == 0);
	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.1f,  0.1f,  1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.85f, 0.1f,  0.1f,  1.0f));
	if (ImGui::Button(deleteLabel))
	{
		m_pendingDelete = PendingDelete::Selected;
		ImGui::OpenPopup("##confirm_delete");
	}
	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
	if (ImGui::Button(ICON_FA_TRASH_CAN " Clear All"))
	{
		m_pendingDelete = PendingDelete::All;
		ImGui::OpenPopup("##confirm_delete");
	}
	ImGui::PopStyleColor(3);

	RenderConfirmModal(history);

	// Reload when search query changes or history was modified
	std::string searchQuery = m_searchBuffer;
	uint64_t currentVersion = history.GetVersion();
	if (searchQuery != m_lastSearch || currentVersion != m_lastSeenVersion)
	{
		m_lastSearch = searchQuery;
		m_lastSeenVersion = currentVersion;
		m_selectedUrls.clear();
		m_entries = searchQuery.empty() ? history.GetAll() : history.Query(searchQuery, 200);
	}

	ImGui::Separator();

	ImGui::BeginChild("##history_list");

	BrowserTab2* activeTab = tabManager.GetActiveTab();

	const float checkboxWidth = ImGui::GetFrameHeight();

	struct DayGroup
	{
		std::string label;
		std::vector<size_t> indices;
	};

	std::vector<DayGroup> dayGroups;
	for (size_t i = 0; i < m_entries.size(); i++)
	{
		std::string dayLabel = m_entries[i].lastVisit > 0 ? GetDayLabel(m_entries[i].lastVisit) : "Unknown date";
		if (dayGroups.empty() || dayGroups.back().label != dayLabel)
			dayGroups.push_back({.label = std::move(dayLabel)});
		dayGroups.back().indices.push_back(i);
	}

	for (const auto& group : dayGroups)
	{
		auto groupSelectedCount = std::ranges::count_if(group.indices,
			[&](size_t idx) { return m_selectedUrls.contains(m_entries[idx].url); });

		bool allSelected = groupSelectedCount == std::ssize(group.indices);
		bool someSelected = groupSelectedCount > 0;

		ImGui::PushID(group.label.c_str());

		bool groupChecked = allSelected;
		if (someSelected && !allSelected)
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
		if (ImGui::Checkbox("##group_check", &groupChecked))
		{
			if (groupChecked)
			{
				for (size_t idx : group.indices)
					m_selectedUrls.insert(m_entries[idx].url);
			}
			else
			{
				for (size_t idx : group.indices)
					m_selectedUrls.erase(m_entries[idx].url);
			}
		}
		if (someSelected && !allSelected)
			ImGui::PopItemFlag();

		ImGui::SameLine();
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		bool groupOpen = ImGui::TreeNodeEx(group.label.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);

		if (groupOpen)
		{
			for (size_t idx : group.indices)
			{
				const auto& entry = m_entries[idx];
				ImGui::PushID(entry.url.c_str());

				bool checked = m_selectedUrls.contains(entry.url);
				if (ImGui::Checkbox("##check", &checked))
				{
					if (checked)
						m_selectedUrls.insert(entry.url);
					else
						m_selectedUrls.erase(entry.url);
				}

				ImGui::SameLine();

				char timeBuffer[8] = "—";
				if (entry.lastVisit > 0)
				{
					time_t timestamp = static_cast<time_t>(entry.lastVisit);
					tm localTime = {};
					localtime_s(&localTime, &timestamp);
					strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &localTime);
				}

				float availableWidth = ImGui::GetContentRegionAvail().x;
				float selectableWidth = availableWidth - checkboxWidth - ImGui::GetStyle().ItemSpacing.x;

				std::string label = entry.title.empty() ? entry.url : entry.title;

				float selectableStartX = ImGui::GetCursorPosX();
				if (ImGui::Selectable("##entry", false, ImGuiSelectableFlags_None, ImVec2(selectableWidth, 0.0f)))
				{
					if (activeTab)
						activeTab->Open(entry.url);
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", entry.url.c_str());

				ImGui::SameLine(selectableStartX);
				ImGui::TextDisabled("%s", timeBuffer);
				ImGui::SameLine();
				ImGui::Text("%s", label.c_str());

				ImGui::PopID();
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::End();
}

void HistoryWindow::RenderConfirmModal(HistoryManager& history)
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("##confirm_delete", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	if (m_pendingDelete == PendingDelete::Selected)
	{
		int count = static_cast<int>(m_selectedUrls.size());
		ImGui::Text("Delete %d %s?", count, count == 1 ? "entry" : "entries");
	}
	else
	{
		ImGui::Text("Delete all history?");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
	{
		m_pendingDelete = PendingDelete::None;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,  0.1f,  0.1f,  1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.8f,  0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.95f, 0.1f,  0.1f,  1.0f));
	if (ImGui::Button("Delete", ImVec2(100.0f, 0.0f)))
	{
		if (m_pendingDelete == PendingDelete::Selected)
		{
			for (const auto& url : m_selectedUrls)
				history.DeleteEntry(url);
			m_selectedUrls.clear();
		}
		else
		{
			history.DeleteAll();
			m_selectedUrls.clear();
		}

		m_pendingDelete = PendingDelete::None;
		ImGui::CloseCurrentPopup();
	}
	ImGui::PopStyleColor(3);

	ImGui::EndPopup();
}
