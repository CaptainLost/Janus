#pragma once

#include "../HistoryManager.h"
#include "../Tabs/TabManager2.h"

#include "imgui.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

class HistoryWindow
{
public:
	void Open();
	bool IsOpen() const { return m_open; }
	void Render(TabManager2& tabManager, HistoryManager& history, ImGuiID dockspaceId);

private:
	enum class PendingDelete { None, Selected, All };

	void RenderConfirmModal(HistoryManager& history);

	bool m_open = false;
	char m_searchBuffer[256] = {};
	std::string m_lastSearch;
	uint64_t m_lastSeenVersion = UINT64_MAX;
	std::vector<HistoryEntry> m_entries;
	std::unordered_set<std::string> m_selectedUrls;
	PendingDelete m_pendingDelete = PendingDelete::None;
};
