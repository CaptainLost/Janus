#include "BrowserLayer.h"
#include "CefInputBridge.h"

#include "include/cef_app.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstring>

void BrowserLayer::OnAttach()
{
	m_tabManager.AddTab();

	m_viewport.SetDetachCallback(
		[this](TabManager2& src, int tabId, ImVec2 mousePos)
		{ OnTabDetachRequested(src, tabId, mousePos); });
}

void BrowserLayer::OnDetach()
{
	for (auto& dv : m_detachedViewports)
	{
		dv->tabManager.CloseAll();
	}

	m_detachedViewports.clear();
	m_tabManager.CloseAll();
}

void BrowserLayer::OnUpdate(float ts)
{
	CefDoMessageLoopWork();

	for (const auto& tab : m_tabManager.Tabs())
	{
		if (tab->GetState() != TabState::Blank)
		{
			m_viewport.UpdateBrowserImage(*tab);
		}
	}

	for (auto& dv : m_detachedViewports)
		for (const auto& tab : dv->tabManager.Tabs())
			if (tab->GetState() != TabState::Blank)
				dv->viewport->UpdateBrowserImage(*tab);
}

void BrowserLayer::OnUIRender()
{
	ImGui::ShowDemoWindow();

	BuildDockLayout();
	RenderSidebar();
	RenderMainViewport();
	RenderDetachedWindows();
}

void BrowserLayer::BuildDockLayout()
{
	if (m_layoutBuilt)
		return;

	ImGuiID dockspaceId = ImGui::GetID("VulkanAppDockspace");

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);

	const ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(dockspaceId, vp->WorkSize);

	ImGuiID dockLeft = 0, dockRight = 0;
	ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockRight);

	ImGui::DockBuilderDockWindow("##Sidebar",         dockLeft);
	ImGui::DockBuilderDockWindow("##BrowserViewport", dockRight);
	ImGui::DockBuilderFinish(dockspaceId);

	auto lockNode = [](ImGuiID id)
	{
		if (auto* n = ImGui::DockBuilderGetNode(id))
			n->LocalFlags |= ImGuiDockNodeFlags_NoTabBar
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

	auto renderTabList = [](TabManager2& mgr, const char* sectionLabel)
	{
		if (mgr.Tabs().empty())
			return;

		if (sectionLabel)
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", sectionLabel);
			ImGui::Separator();
		}

		int removeId = -1;
		for (const auto& tabPtr : mgr.Tabs())
		{
			auto& tab = *tabPtr;
			ImGui::PushID(&tab);

			std::string title;
			if (tab.GetState() == TabState::Blank)
			{
				title = "New Tab";
			}
			else
			{
				auto state = tab.GetWebViewState();
				title = state.Title.empty() ? "Loading..." : state.Title;
				if (title.size() > 25)
					title = title.substr(0, 22) + "...";
				if (state.IsLoading)
					title = "[*] " + title;
			}

			bool isActive = (tab.GetId() == mgr.GetActiveTabId());
			if (isActive)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

			if (ImGui::Button(title.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
				mgr.SetActiveTab(tab.GetId());

			if (isActive)
				ImGui::PopStyleColor();

			ImGui::SameLine();
			if (ImGui::SmallButton("X"))
				removeId = tab.GetId();

			ImGui::PopID();
		}

		if (removeId >= 0)
			mgr.RemoveTab(removeId);
	};

	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Main");
	ImGui::Separator();
	renderTabList(m_tabManager, nullptr);

	ImGui::Spacing();
	if (ImGui::Button("+ New Tab", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		m_tabManager.AddTab();

	for (int i = 0; i < static_cast<int>(m_detachedViewports.size()); ++i)
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		std::string label = "Window " + std::to_string(i + 1);
		renderTabList(m_detachedViewports[i]->tabManager, label.c_str());
	}

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

void BrowserLayer::RenderDetachedWindows()
{
	for (auto it = m_detachedViewports.begin(); it != m_detachedViewports.end(); )
	{
		auto& dv = **it;

		if (!dv.tabManager.HasAnyTab())
		{
			it = m_detachedViewports.erase(it);
			continue;
		}

		std::string windowTitle = "Browser";
		if (auto* activeTab = dv.tabManager.GetActiveTab())
		{
			if (activeTab->GetState() != TabState::Blank)
			{
				auto state = activeTab->GetWebViewState();
				if (!state.Title.empty())
					windowTitle = state.Title;
			}
		}

		// ### keeps ImGui window identity stable while title changes
		int idx = static_cast<int>(it - m_detachedViewports.begin());
		windowTitle += "###detached_" + std::to_string(idx);

		ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_Appearing);

		if (ImGui::Begin(windowTitle.c_str(), &dv.open,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
		{
			dv.viewport->Render(dv.tabManager);
		}
		ImGui::End();

		if (!dv.open)
		{
			dv.tabManager.CloseAll();
			it = m_detachedViewports.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void BrowserLayer::OnTabDetachRequested(TabManager2& srcManager, int tabId, ImVec2 mousePos)
{
	auto tab = srcManager.DetachTab(tabId);
	if (!tab)
		return;

	auto dv = std::make_unique<DetachedViewport>();
	std::string vpId = "detached_" + std::to_string(m_nextDetachedId++);
	dv->viewport = std::make_unique<BrowserViewport>(vpId);
	dv->open = true;

	dv->viewport->SetDetachCallback(
		[this](TabManager2& src, int id, ImVec2 pos)
		{ OnTabDetachRequested(src, id, pos); });

	dv->tabManager.AcceptTab(std::move(tab));
	ImGui::SetNextWindowPos(mousePos, ImGuiCond_Appearing);

	m_detachedViewports.push_back(std::move(dv));
}
