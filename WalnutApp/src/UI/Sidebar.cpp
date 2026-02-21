#include "Sidebar.h"
#include "../Browser/Tabs/SavedTab.h"
#include "../Utils/UrlUtils.h"
#include "../Utils/StringUtils.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

static std::string BuildSavedEntryLabel(const std::string& baseUrl, const std::string& currentUrl)
{
	std::string host = UrlUtils::GetHost(baseUrl);
	if (host.empty())
	{
		host = baseUrl;
	}

	if (currentUrl.empty())
	{
		return host;
	}

	std::string path = UrlUtils::GetPath(currentUrl);
	if (path == "/" || path.empty())
	{
		return host;
	}

	return host + " " + path;
}

static std::string BuildTemporaryTabLabel(const Tab& tab)
{
	if (!tab.IsOpen())
	{
		return ICON_FA_FILE " New Tab";
	}

	Walnut::WebViewState state = tab.GetWebViewState();
	const char* icon = state.IsLoading ? ICON_FA_SPINNER : ICON_FA_GLOBE;
	std::string title = state.Title.empty() ? "Loading..." : state.Title;
	return StringUtils::Truncate(std::string(icon) + " " + title, 30);
}

void Sidebar::Load(SavedTabsManager& savedTabsManager)
{
	m_savedEntries = savedTabsManager.GetAll();
}

SavedTab* Sidebar::FindSavedTab(int dbId, TabManager& tabManager)
{
	for (const auto& tabPtr : tabManager.Tabs())
	{
		SavedTab* savedTab = dynamic_cast<SavedTab*>(tabPtr.get());
		if (savedTab && savedTab->GetDbId() == dbId && savedTab->IsSaved())
		{
			return savedTab;
		}
	}

	return nullptr;
}

void Sidebar::Render(TabManager& tabManager, SavedTabsManager& savedTabsManager)
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
	{
		tabManager.AddTab();
	}

	ImGui::End();
}

void Sidebar::RenderSavedSection(TabManager& tabManager, SavedTabsManager& savedTabsManager)
{
	ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), ICON_FA_BOOKMARK " Saved");
	ImGui::Separator();

	int removeSavedIndex = -1;
	for (int i = 0; i < static_cast<int>(m_savedEntries.size()); i++)
	{
		const SavedTabRecord& entry = m_savedEntries[i];
		ImGui::PushID(entry.dbId);

		SavedTab* savedTab = FindSavedTab(entry.dbId, tabManager);
		Tab* tab = savedTab;

		if (tab && tab->IsOpen())
		{
			Walnut::WebViewState state = tab->GetWebViewState();
			if (!state.URL.empty())
			{
				m_savedLastUrls[entry.dbId] = state.URL;
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
		label = StringUtils::Truncate(label, 30);

		bool isActive = isTabOpen && (tab->GetId() == tabManager.GetActiveTabId());
		if (isActive)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}

		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
		{
			if (!isTabOpen)
			{
				int newTabId = tabManager.AddSavedTab(entry.dbId, entry.baseUrl);
				Tab* newTab = tabManager.GetTab(newTabId);
				newTab->Open(entry.baseUrl);

				TabManager* tabManagerPtr = &tabManager;
				static_cast<SavedTab*>(newTab)->SetDomainExitHandler([tabManagerPtr](const std::string& escapedUrl) {
					int id = tabManagerPtr->AddTab();
					tabManagerPtr->GetTab(id)->Open(escapedUrl);
					tabManagerPtr->SetActiveTab(id);
				});

				tab = newTab;
			}

			tabManager.SetActiveTab(tab->GetId());
		}

		if (isActive)
		{
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();
		if (ImGui::SmallButton(ICON_FA_XMARK))
		{
			removeSavedIndex = i;
		}

		ImGui::PopID();
	}

	if (removeSavedIndex >= 0)
	{
		const SavedTabRecord& entry = m_savedEntries[removeSavedIndex];
		SavedTab* savedTab = FindSavedTab(entry.dbId, tabManager);

		if (savedTab)
		{
			tabManager.RemoveTab(savedTab->GetId());
		}
		else
		{
			savedTabsManager.RemoveEntry(entry.dbId);
			m_savedEntries.erase(m_savedEntries.begin() + removeSavedIndex);
		}
	}

	ImGui::Spacing();

	if (ImGui::Button(ICON_FA_PLUS " Add Saved", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
	{
		ImGui::OpenPopup("AddSavedTab");
	}

	RenderAddSavedPopup(savedTabsManager);
}

void Sidebar::RenderAddSavedPopup(SavedTabsManager& savedTabsManager)
{
	if (!ImGui::BeginPopupModal("AddSavedTab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

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

void Sidebar::RenderTemporarySection(TabManager& tabManager)
{
	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), ICON_FA_CLOCK " Temporary");
	ImGui::Separator();

	int removeId = -1;
	for (const auto& tabPtr : tabManager.Tabs())
	{
		if (tabPtr->IsSaved())
		{
			continue;
		}

		Tab& tab = *tabPtr;
		ImGui::PushID(&tab);

		std::string title = BuildTemporaryTabLabel(tab);

		bool isActive = (tab.GetId() == tabManager.GetActiveTabId());
		if (isActive)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}

		if (ImGui::Button(title.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
		{
			tabManager.SetActiveTab(tab.GetId());
		}

		if (isActive)
		{
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();
		if (ImGui::SmallButton(ICON_FA_XMARK))
		{
			removeId = tab.GetId();
		}

		ImGui::PopID();
	}

	if (removeId >= 0)
	{
		tabManager.RemoveTab(removeId);
	}
}
