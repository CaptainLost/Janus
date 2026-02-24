#include "Sidebar.h"
#include "../Browser/Tabs/SavedBrowserTab.h"
#include "../Utils/UrlUtils.h"
#include "../Utils/StringUtils.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"


void Sidebar::Load(SavedTabsManager& savedTabsManager)
{
	m_savedEntries = savedTabsManager.GetAll();
	m_folders = savedTabsManager.GetAllFolders();
}

SavedBrowserTab* Sidebar::FindSavedTab(int dbId, TabManager& tabManager)
{
	for (const auto& tabPtr : tabManager.Tabs())
	{
		SavedBrowserTab* savedTab = dynamic_cast<SavedBrowserTab*>(tabPtr.get());
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

void Sidebar::RenderSavedTabItem(const SavedTabRecord& entry,
	TabManager& tabManager, SavedTabsManager& savedTabsManager, int& outRemoveTabDbId)
{
	ImGui::PushID(entry.dbId);

	SavedBrowserTab* savedTab = FindSavedTab(entry.dbId, tabManager);
	Tab* tab = savedTab;

	bool isTabOpen = (tab != nullptr);

	std::string label;
	if (!isTabOpen)
	{
		std::string displayName = UrlUtils::GetDomainDisplayName(entry.baseUrl);
		label = ICON_FA_BOOKMARK " " + (displayName.empty() ? entry.baseUrl : displayName);
	}
	else
	{
		label = tab->GetSidebarLabel();
	}

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
			static_cast<SavedBrowserTab*>(newTab)->SetDomainExitHandler([tabManagerPtr](const std::string& escapedUrl) {
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

	if (ImGui::BeginDragDropSource())
	{
		int tabDbId = entry.dbId;
		ImGui::SetDragDropPayload("SAVED_TAB", &tabDbId, sizeof(int));
		ImGui::Text("%s", label.c_str());
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVED_TAB"))
		{
			int draggedTabDbId = *static_cast<const int*>(payload->Data);
			savedTabsManager.SwapTabSortOrders(draggedTabDbId, entry.dbId);
			Load(savedTabsManager);
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	if (ImGui::SmallButton(ICON_FA_XMARK))
	{
		outRemoveTabDbId = entry.dbId;
	}

	ImGui::PopID();
}

void Sidebar::RenderFolderNode(const SavedFolderRecord& folder,
	TabManager& tabManager, SavedTabsManager& savedTabsManager, int& outRemoveTabDbId)
{
	ImGui::PushID(folder.dbId);

	ImVec4 bgColor = (ImVec4)folder.color;

	float luminance = 0.299f * bgColor.x + 0.587f * bgColor.y + 0.114f * bgColor.z;
	ImVec4 darkText = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	darkText.w = 1.0f;
	ImVec4 textColor = (luminance > 0.5f)
		? darkText
		: ImGui::GetStyleColorVec4(ImGuiCol_Text);

	ImGui::PushStyleColor(ImGuiCol_Header,        bgColor);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, bgColor);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive,  bgColor);
	ImGui::PushStyleColor(ImGuiCol_Text,          textColor);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth
		| ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_OpenOnDoubleClick;

	std::string nodeLabel = ICON_FA_FOLDER " " + folder.name;
	ImGui::SetNextItemStorageID((ImGuiID)folder.dbId);
	bool nodeOpen = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);

	ImGui::PopStyleColor(4);

	if (ImGui::BeginDragDropSource())
	{
		int folderId = folder.dbId;
		ImGui::SetDragDropPayload("SAVED_FOLDER", &folderId, sizeof(int));
		ImGui::Text("%s", folder.name.c_str());
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVED_TAB"))
		{
			int tabDbId = *static_cast<const int*>(payload->Data);
			savedTabsManager.MoveTabToFolder(tabDbId, folder.dbId);
			Load(savedTabsManager);
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVED_FOLDER"))
		{
			int draggedFolderId = *static_cast<const int*>(payload->Data);
			if (!IsFolderDescendant(draggedFolderId, folder.dbId))
			{
				savedTabsManager.MoveFolder(draggedFolderId, folder.dbId);
				Load(savedTabsManager);
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Rename"))
		{
			m_editingFolderId = folder.dbId;
			strncpy(m_editFolderNameBuffer, folder.name.c_str(), sizeof(m_editFolderNameBuffer) - 1);
			m_editFolderNameBuffer[sizeof(m_editFolderNameBuffer) - 1] = '\0';
			m_editFolderColor = folder.color;
			m_pendingOpenEditFolder = true;
		}
		if (ImGui::MenuItem("New Subfolder"))
		{
			m_newFolderParentId = folder.dbId;
			m_newFolderNameBuffer[0] = '\0';
			m_newFolderColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
			m_pendingOpenNewFolder = true;
		}
		if (ImGui::MenuItem("Delete"))
		{
			savedTabsManager.DeleteFolder(folder.dbId);
			Load(savedTabsManager);
		}
		ImGui::EndPopup();
	}

	if (nodeOpen)
	{
		// Snapshot tabs and child folders before rendering (Load may invalidate m_savedEntries/m_folders)
		std::vector<SavedTabRecord> tabsInFolder;
		for (const SavedTabRecord& entry : m_savedEntries)
		{
			if (entry.folderId == folder.dbId)
			{
				tabsInFolder.push_back(entry);
			}
		}

		std::vector<SavedFolderRecord> childFolders;
		for (const SavedFolderRecord& f : m_folders)
		{
			if (f.parentId == folder.dbId)
			{
				childFolders.push_back(f);
			}
		}

		for (const SavedTabRecord& entry : tabsInFolder)
		{
			RenderSavedTabItem(entry, tabManager, savedTabsManager, outRemoveTabDbId);
		}

		for (const SavedFolderRecord& child : childFolders)
		{
			RenderFolderNode(child, tabManager, savedTabsManager, outRemoveTabDbId);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

bool Sidebar::IsFolderDescendant(int ancestorId, int nodeId) const
{
	if (nodeId == ancestorId)
	{
		return true;
	}

	for (const SavedFolderRecord& f : m_folders)
	{
		if (f.dbId == nodeId)
		{
			if (f.parentId == -1)
			{
				return false;
			}

			return IsFolderDescendant(ancestorId, f.parentId);
		}
	}

	return false;
}

void Sidebar::RenderSavedSection(TabManager& tabManager, SavedTabsManager& savedTabsManager)
{
	ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.4f, 1.0f), ICON_FA_BOOKMARK " Saved");

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVED_TAB"))
		{
			int tabDbId = *static_cast<const int*>(payload->Data);
			savedTabsManager.MoveTabToFolder(tabDbId, -1);
			Load(savedTabsManager);
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SAVED_FOLDER"))
		{
			int folderId = *static_cast<const int*>(payload->Data);
			savedTabsManager.MoveFolder(folderId, -1);
			Load(savedTabsManager);
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::Separator();

	int removeTabDbId = -1;

	// Snapshots prevent iterator invalidation if Load is called inside render helpers
	std::vector<SavedTabRecord> uncategorizedTabs;
	for (const SavedTabRecord& entry : m_savedEntries)
	{
		if (entry.folderId == -1)
		{
			uncategorizedTabs.push_back(entry);
		}
	}

	std::vector<SavedFolderRecord> rootFolders;
	for (const SavedFolderRecord& folder : m_folders)
	{
		if (folder.parentId == -1)
		{
			rootFolders.push_back(folder);
		}
	}

	for (const SavedTabRecord& entry : uncategorizedTabs)
	{
		RenderSavedTabItem(entry, tabManager, savedTabsManager, removeTabDbId);
	}

	for (const SavedFolderRecord& folder : rootFolders)
	{
		RenderFolderNode(folder, tabManager, savedTabsManager, removeTabDbId);
	}

	if (removeTabDbId >= 0)
	{
		SavedBrowserTab* savedTab = FindSavedTab(removeTabDbId, tabManager);

		if (savedTab)
		{
			tabManager.RemoveTab(savedTab->GetId());
		}
		else
		{
			savedTabsManager.RemoveEntry(removeTabDbId);
			Load(savedTabsManager);
		}
	}

	ImGui::Spacing();

	float availableWidth = ImGui::GetContentRegionAvail().x;
	float halfWidth = (availableWidth - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	if (ImGui::Button(ICON_FA_PLUS " Add Saved", ImVec2(halfWidth, 0)))
	{
		ImGui::OpenPopup("AddSavedTab");
	}

	ImGui::SameLine();

	if (ImGui::Button(ICON_FA_FOLDER_PLUS " New Folder", ImVec2(halfWidth, 0)))
	{
		m_newFolderNameBuffer[0] = '\0';
		m_newFolderColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
		ImGui::OpenPopup("NewFolder");
	}

	// Open deferred popups at this scope so the ID stack matches BeginPopupModal
	if (m_pendingOpenEditFolder)
	{
		ImGui::OpenPopup("EditFolder");
		m_pendingOpenEditFolder = false;
	}
	if (m_pendingOpenNewFolder)
	{
		ImGui::OpenPopup("NewFolder");
		m_pendingOpenNewFolder = false;
	}

	RenderAddSavedPopup(savedTabsManager);
	RenderNewFolderPopup(savedTabsManager);
	RenderEditFolderPopup(savedTabsManager);
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

void Sidebar::RenderNewFolderPopup(SavedTabsManager& savedTabsManager)
{
	if (!ImGui::BeginPopupModal("NewFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	ImGui::Text("Folder name:");
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputText("##newFolderName", m_newFolderNameBuffer, sizeof(m_newFolderNameBuffer));

	ImGui::Text("Color:");
	ImGui::SetNextItemWidth(260.0f);
	ImGui::ColorEdit4("##newFolderColor", (float*)&m_newFolderColor.Value);

	if (ImGui::Button("Add") && m_newFolderNameBuffer[0] != '\0')
	{
		int dbId = savedTabsManager.CreateFolder(m_newFolderNameBuffer, m_newFolderParentId);
		savedTabsManager.SetFolderColor(dbId, m_newFolderColor);
		m_newFolderParentId = -1;
		Load(savedTabsManager);
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		m_newFolderParentId = -1;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

void Sidebar::RenderEditFolderPopup(SavedTabsManager& savedTabsManager)
{
	if (!ImGui::BeginPopupModal("EditFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	ImGui::Text("Folder name:");
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputText("##editFolderName", m_editFolderNameBuffer, sizeof(m_editFolderNameBuffer));

	ImGui::Text("Color:");
	ImGui::SetNextItemWidth(260.0f);
	ImGui::ColorEdit4("##editFolderColor", (float*)&m_editFolderColor.Value);

	if (ImGui::Button("Save") && m_editFolderNameBuffer[0] != '\0')
	{
		savedTabsManager.RenameFolder(m_editingFolderId, m_editFolderNameBuffer);
		savedTabsManager.SetFolderColor(m_editingFolderId, m_editFolderColor);
		m_editingFolderId = -1;
		Load(savedTabsManager);
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		m_editingFolderId = -1;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

void Sidebar::RenderTemporarySection(TabManager& tabManager)
{
	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), ICON_FA_CLOCK " Temporary");
	ImGui::Separator();

	int removeId = -1;
	int draggedTempTabId = -1;
	int targetTempTabId = -1;

	for (const auto& tabPtr : tabManager.Tabs())
	{
		if (tabPtr->IsSaved())
		{
			continue;
		}

		Tab& tab = *tabPtr;
		ImGui::PushID(&tab);

		std::string title = tab.GetSidebarLabel();

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

		if (ImGui::BeginDragDropSource())
		{
			int tabId = tab.GetId();
			ImGui::SetDragDropPayload("TEMP_TAB", &tabId, sizeof(int));
			ImGui::Text("%s", title.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEMP_TAB"))
			{
				draggedTempTabId = *static_cast<const int*>(payload->Data);
				targetTempTabId = tab.GetId();
			}
			ImGui::EndDragDropTarget();
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

	if (draggedTempTabId >= 0 && targetTempTabId >= 0)
	{
		tabManager.MoveTabBefore(draggedTempTabId, targetTempTabId);
	}
}
