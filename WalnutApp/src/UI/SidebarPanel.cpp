#include "SidebarPanel.h"

#include "imgui.h"
#include "IconsFontAwesome6.h"

constexpr const char* TabDragPayloadType = "TAB_DRAG_DROP_PAYLOAD";

SidebarPanel::SidebarPanel(const std::shared_ptr<TabManager>& tabManager)
	: m_tabManager(tabManager)
{
}

void SidebarPanel::Render()
{
	if (!ImGui::Begin("##Sidebar", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::End();

		return;
	}

	RenderTabSeparator(ICON_FA_BOOKMARK " Persistent",
		[this](const std::shared_ptr<Tab>& tab)
		{
			OnSavedTabMoveToSection(tab);
		});
	RenderPersistentTabSection();

	RenderTabSeparator(ICON_FA_CLOCK " Temporary",
		[this](const std::shared_ptr<Tab>& tab)
		{
			OnTemporaryTabMoveToSection(tab);
		});
	RenderTemporaryTabSection();

	ImGui::Separator();

	if (ImGui::Button(ICON_FA_PLUS " New Tab", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
	{
		m_tabManager->AddTab();
	}

	ImGui::End();
}

void SidebarPanel::RenderTabSeparator(const std::string& label, tabCallbackFn& onTabDropCallback)
{
	ImGui::SeparatorText(label.c_str());

	if (onTabDropCallback && ImGui::BeginDragDropTarget())
	{
		ImGuiDragDropFlags drop_target_flags = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoPreviewTooltip;

		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TabDragPayloadType))
		{
			std::shared_ptr<Tab> droppedTab = *(std::shared_ptr<Tab>*)payload->Data;

			onTabDropCallback(droppedTab);
		}

		ImGui::EndDragDropTarget();
	}
}

void SidebarPanel::RenderPersistentTabSection()
{
	ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanLabelWidth
		| ImGuiTreeNodeFlags_DrawLinesFull;

	ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, 1.0f);

	if (ImGui::TreeNodeEx(ICON_FA_FOLDER " TestLabel 1", treeNodeFlags))
	{
		std::shared_ptr<Tab> tabToClose;

		for (const std::shared_ptr<Tab>& tab : m_tabManager->Tabs())
		{
			RenderCompleteTab(tab,
				[this](const std::shared_ptr<Tab>& tab)
				{
					OnSavedTabClicked(tab);
				},
				[&tabToClose](const std::shared_ptr<Tab>& tab)
				{
					tabToClose = tab;
				});
		}

		ImGui::TreePop();

		if (tabToClose)
		{
			OnSavedTabClose(tabToClose);
		}
	}

	ImGui::PopStyleVar();
}

void SidebarPanel::RenderTemporaryTabSection()
{
	std::shared_ptr<Tab> tabToClose;

	for (const std::shared_ptr<Tab>& tab : m_tabManager->Tabs())
	{
		RenderCompleteTab(tab,
			[this](const std::shared_ptr<Tab>& tab)
			{
				OnTemporaryTabClicked(tab);
			},
			[&tabToClose](const std::shared_ptr<Tab>& tab)
			{
				tabToClose = tab;
			});
	}

	if (tabToClose)
	{
		OnTemporaryTabClose(tabToClose);
	}
}

void SidebarPanel::RenderCompleteTab(const std::shared_ptr<Tab>& tab, tabCallbackFn& onTabClickedCallback, tabCallbackFn& onCloseCallback)
{
	const bool isActive = (tab->GetId() == m_tabManager->GetActiveTabId());

	if (isActive)
	{
		ImGui::SetNextItemAllowOverlap();
	}

	RenderTab(tab, isActive, onTabClickedCallback);

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		ImGui::SetDragDropPayload(TabDragPayloadType, &tab, sizeof(std::shared_ptr<Tab>));
		ImGui::Text(tab->GetSidebarLabel().c_str());

		ImGui::EndDragDropSource();
	}

	if (isActive)
	{
		ImGui::SameLine();

		RenderTabCloseButton(tab, ImGui::IsItemHovered(), ImGui::IsItemActive(), onCloseCallback);
	}

	RenderSavedTabContextPopup(tab, onCloseCallback);
}

void SidebarPanel::RenderTab(const std::shared_ptr<Tab>& tab, bool isActive, tabCallbackFn& onTabClickedCallback)
{
	ImGui::PushID(tab.get());

	if (ImGui::Selectable(tab->GetSidebarLabel().c_str(), isActive))
	{
		onTabClickedCallback(tab);
	}

	ImGui::PopID();
}

void SidebarPanel::RenderTabCloseButton(const std::shared_ptr<Tab>& tab, bool isHovered, bool isClicked, tabCallbackFn& onCloseCallback)
{
	if (isClicked)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
	}
	else if (isHovered)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
	}

	float buttonWidth = ImGui::CalcTextSize(ICON_FA_MINUS).x + ImGui::GetStyle().FramePadding.x * 2;

	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonWidth);

	if (ImGui::SmallButton(ICON_FA_MINUS "##SavedTabXClose"))
	{
		onCloseCallback(tab);
	}

	if (isHovered || isClicked)
	{
		ImGui::PopStyleColor();
	}
}

void SidebarPanel::RenderSavedTabContextPopup(const std::shared_ptr<Tab>& tab, tabCallbackFn& onCloseCallback)
{
	ImGui::PushID(tab.get());

	if (ImGui::BeginPopupContextItem("SavedTabContextPopup"))
	{
		if (onCloseCallback && ImGui::Selectable("Close##SavedTabContextPopup"))
		{
			ImGui::CloseCurrentPopup();

			onCloseCallback(tab);
		}

		ImGui::Separator();

		if (ImGui::Selectable("Duplicate##SavedTabContextPopup"))
		{

		}

		ImGui::EndPopup();
	}

	ImGui::PopID();
}

void SidebarPanel::OnTemporaryTabClicked(const std::shared_ptr<Tab>& tab)
{
	m_tabManager->SetActiveTab(tab->GetId());
}

void SidebarPanel::OnTemporaryTabClose(const std::shared_ptr<Tab>& tab)
{
	m_tabManager->RemoveTab(tab->GetId());
}

void SidebarPanel::OnTemporaryTabMoveToSection(const std::shared_ptr<Tab>& tab)
{

}

void SidebarPanel::OnSavedTabClicked(const std::shared_ptr<Tab>& tab)
{
	m_tabManager->SetActiveTab(tab->GetId());
}

void SidebarPanel::OnSavedTabClose(const std::shared_ptr<Tab>& tab)
{
	m_tabManager->RemoveTab(tab->GetId());
}

void SidebarPanel::OnSavedTabMoveToSection(const std::shared_ptr<Tab>& tab)
{

}