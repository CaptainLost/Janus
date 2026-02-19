#include "BrowserLayer.h"
#include "CefInputBridge.h"

#include "include/cef_app.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome6.h"

#include <algorithm>
#include <cstring>

void BrowserLayer::OnAttach()
{
	m_viewport.SetHistoryManager(&m_historyManager);
}

void BrowserLayer::OnDetach()
{
	m_tabManager.CloseAll();
}

void BrowserLayer::OnUpdate(float ts)
{
	CefDoMessageLoopWork();

	for (const auto& tab : m_tabManager.Tabs())
	{
		if (!tab->IsOpen())
			continue;

		m_viewport.UpdateBrowserImage(*tab);

		Walnut::WebViewState state = tab->GetWebViewState();
		if (!state.IsLoading && !state.URL.empty() && !state.Title.empty())
		{
			auto& lastUrl = m_lastRecordedUrls[tab->GetId()];
			if (lastUrl != state.URL)
			{
				m_historyManager.AddVisit(state.URL, state.Title);
				lastUrl = state.URL;
			}
		}
	}
}

void BrowserLayer::OnUIRender()
{
	ImGui::ShowDemoWindow();

	BuildDockLayout();
	RenderSidebar();
	RenderMainViewport();
}

void BrowserLayer::BuildDockLayout()
{
	if (m_layoutBuilt)
		return;

	ImGuiID dockspaceId = ImGui::GetID("VulkanAppDockspace");

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

	ImGuiID dockLeft = 0, dockRight = 0;
	ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockRight);

	ImGui::DockBuilderDockWindow("##Sidebar",         dockLeft);
	ImGui::DockBuilderDockWindow("##BrowserViewport", dockRight);
	ImGui::DockBuilderFinish(dockspaceId);

	auto lockNode = [](ImGuiID id)
	{
		if (ImGuiDockNode* dockNode = ImGui::DockBuilderGetNode(id))
			dockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar
			               |  ImGuiDockNodeFlags_NoDocking
			               |  ImGuiDockNodeFlags_NoResize;
	};

	lockNode(dockLeft);
	lockNode(dockRight);
	m_layoutBuilt = true;
}

void BrowserLayer::RenderSidebar()
{
	ImGui::Begin("##Sidebar", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

	auto renderTabList = [](TabManager2& tabManager, const char* sectionLabel)
	{
		if (tabManager.Tabs().empty())
			return;

		if (sectionLabel)
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", sectionLabel);
			ImGui::Separator();
		}

		int removeId = -1;
		for (const auto& tabPtr : tabManager.Tabs())
		{
			auto& tab = *tabPtr;
			ImGui::PushID(&tab);

			std::string title;
			if (!tab.IsOpen())
			{
				title = ICON_FA_FILE " New Tab";
			}
			else
			{
				auto state = tab.GetWebViewState();
				title = state.Title.empty() ? ICON_FA_SPINNER " Loading..." : ICON_FA_GLOBE " " + state.Title;
				if (title.size() > 30)
					title = title.substr(0, 27) + "...";
				if (state.IsLoading)
					title = ICON_FA_SPINNER " " + title;
			}

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
	};

	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Main");
	ImGui::Separator();
	renderTabList(m_tabManager, nullptr);

	ImGui::Spacing();
	if (ImGui::Button(ICON_FA_PLUS " New Tab", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		m_tabManager.AddTab();

	ImGui::End();
}

void BrowserLayer::RenderMainViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::Begin("##BrowserViewport", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse);

	m_viewport.Render(m_tabManager);

	ImGui::End();
	ImGui::PopStyleVar();
}