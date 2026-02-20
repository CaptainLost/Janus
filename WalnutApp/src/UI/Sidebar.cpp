#include "Sidebar.h"
#include "../Utils/UrlUtils.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

#include <unordered_set>

static std::string BuildSavedEntryLabel(const std::string& baseUrl, const std::string& currentUrl)
{
	std::string host = UrlUtils::GetHost(baseUrl);
	if (host.empty())
		host = baseUrl;

	if (currentUrl.empty())
		return host;

	std::string path = UrlUtils::GetPath(currentUrl);
	if (path == "/" || path.empty())
		return host;

	return host + " " + path;
}

static std::string Truncate(const std::string& text, size_t maxLength)
{
	if (text.size() <= maxLength)
		return text;
	return text.substr(0, maxLength - 3) + "...";
}

static std::string BuildTemporaryTabLabel(const BrowserTab2& tab)
{
	if (!tab.IsOpen())
		return ICON_FA_FILE " New Tab";

	Walnut::WebViewState state = tab.GetWebViewState();
	const char* icon = state.IsLoading ? ICON_FA_SPINNER : ICON_FA_GLOBE;
	std::string title = state.Title.empty() ? "Loading..." : state.Title;
	return Truncate(std::string(icon) + " " + title, 30);
}

// --- Sidebar ---

void Sidebar::Load(SavedTabsManager& savedTabsManager)
{
	m_savedEntries = savedTabsManager.GetAll();
}

BrowserTab2* Sidebar::ResolveTab(int dbId, TabManager2& tabManager)
{
	auto it = m_savedDbIdToTabId.find(dbId);
	if (it == m_savedDbIdToTabId.end())
		return nullptr;

	BrowserTab2* tab = tabManager.GetTab(it->second);
	if (!tab)
		m_savedDbIdToTabId.erase(it);
	return tab;
}

void Sidebar::Render(TabManager2& tabManager, SavedTabsManager& savedTabsManager)
{
	ImGui::Begin("##Sidebar", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

	RenderSavedSection(tabManager, savedTabsManager);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	RenderTemporarySection(tabManager);

	ImGui::Spacing();
	if (ImGui::Button(ICON_FA_PLUS " New Tab", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		tabManager.AddTab();

	ImGui::End();
}

void Sidebar::RenderSavedSection(TabManager2& tabManager, SavedTabsManager& savedTabsManager)
{
	ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), ICON_FA_BOOKMARK " Saved");
	ImGui::Separator();

	int removeSavedIndex = -1;
	for (int i = 0; i < static_cast<int>(m_savedEntries.size()); i++)
	{
		const SavedTabRecord& entry = m_savedEntries[i];
		ImGui::PushID(entry.dbId);

		BrowserTab2* tab = ResolveTab(entry.dbId, tabManager);

		if (tab && tab->IsOpen())
		{
			Walnut::WebViewState state = tab->GetWebViewState();
			if (UrlUtils::IsHttp(state.URL))
			{
				if (UrlUtils::IsSameDomain(UrlUtils::GetHost(state.URL), UrlUtils::GetHost(entry.baseUrl)))
					m_savedLastUrls[entry.dbId] = state.URL;
				else if (!state.IsLoading)
				{
					m_savedDbIdToTabId.erase(entry.dbId);
					tab = nullptr;
				}
			}
		}

		bool isTabOpen = (tab != nullptr);

		std::string label;
		if (!isTabOpen)
		{
			auto it = m_savedLastUrls.find(entry.dbId);
			std::string lastUrl = (it != m_savedLastUrls.end()) ? it->second : "";
			label = ICON_FA_BOOKMARK " " + BuildSavedEntryLabel(entry.baseUrl, lastUrl);
		}
		else
		{
			Walnut::WebViewState state = tab->GetWebViewState();
			const char* icon = state.IsLoading ? ICON_FA_SPINNER : ICON_FA_GLOBE;
			label = std::string(icon) + " " + BuildSavedEntryLabel(entry.baseUrl, state.URL);
		}
		label = Truncate(label, 30);

		bool isActive = isTabOpen && (tab->GetId() == tabManager.GetActiveTabId());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
		{
			if (!isTabOpen)
			{
				int newTabId = tabManager.AddTab();
				tabManager.GetTab(newTabId)->Open(entry.baseUrl);
				m_savedDbIdToTabId[entry.dbId] = newTabId;
				tab = tabManager.GetTab(newTabId);
			}
			tabManager.SetActiveTab(tab->GetId());
		}

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton(ICON_FA_XMARK))
			removeSavedIndex = i;

		ImGui::PopID();
	}

	if (removeSavedIndex >= 0)
	{
		const SavedTabRecord& entry = m_savedEntries[removeSavedIndex];
		auto it = m_savedDbIdToTabId.find(entry.dbId);
		if (it != m_savedDbIdToTabId.end())
		{
			tabManager.RemoveTab(it->second);
			m_savedDbIdToTabId.erase(it);
		}
		else
		{
			savedTabsManager.RemoveEntry(entry.dbId);
			m_savedEntries.erase(m_savedEntries.begin() + removeSavedIndex);
		}
	}

	ImGui::Spacing();

	if (ImGui::Button(ICON_FA_PLUS " Add Saved", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		ImGui::OpenPopup("AddSavedTab");

	RenderAddSavedPopup(savedTabsManager);
}

void Sidebar::RenderAddSavedPopup(SavedTabsManager& savedTabsManager)
{
	if (!ImGui::BeginPopupModal("AddSavedTab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::Text("Base URL:");
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputText("##savedUrl", m_savedUrlBuffer, sizeof(m_savedUrlBuffer));

	if (ImGui::Button("Add") && m_savedUrlBuffer[0] != '\0')
	{
		int dbId = savedTabsManager.AddEntry(m_savedUrlBuffer);
		m_savedEntries.push_back({ .dbId = dbId, .baseUrl = m_savedUrlBuffer });
		m_savedUrlBuffer[0] = '\0';
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		m_savedUrlBuffer[0] = '\0';
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void Sidebar::RenderTemporarySection(TabManager2& tabManager)
{
	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), ICON_FA_CLOCK " Temporary");
	ImGui::Separator();

	std::unordered_set<int> savedTabIds;
	for (const auto& [dbId, tabId] : m_savedDbIdToTabId)
		savedTabIds.insert(tabId);

	int removeId = -1;
	for (const auto& tabPtr : tabManager.Tabs())
	{
		if (savedTabIds.count(tabPtr->GetId()))
			continue;

		auto& tab = *tabPtr;
		ImGui::PushID(&tab);

		std::string title = BuildTemporaryTabLabel(tab);

		bool isActive = (tab.GetId() == tabManager.GetActiveTabId());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(title.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
			tabManager.SetActiveTab(tab.GetId());

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton(ICON_FA_XMARK))
			removeId = tab.GetId();

		ImGui::PopID();
	}

	if (removeId >= 0)
		tabManager.RemoveTab(removeId);
}
