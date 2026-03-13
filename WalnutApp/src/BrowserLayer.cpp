#include "BrowserLayer.h"

#include "UI/BrowserViewport.h"
#include "History/HistoryManager.h"
#include "SavedTabs/SavedDatabase.h"
#include "SavedTabs/SavedTabsManager.h"
#include "SavedTabs/SavedFoldersManager.h"

#include "include/cef_app.h"

#include "imgui.h"
#include "imgui_internal.h"

BrowserLayer::BrowserLayer()
	: m_viewport(std::make_unique<BrowserViewport>("main"))
	, m_historyManager(std::make_unique<HistoryManager>())
	, m_savedDatabase(std::make_unique<SavedDatabase>())
	, m_savedTabsManager(std::make_unique<SavedTabsManager>(*m_savedDatabase))
	, m_savedFoldersManager(std::make_unique<SavedFoldersManager>(*m_savedDatabase))
	, m_sidebarPanel(m_tabManager)
	, m_viewportPanel(m_tabManager)
{
}

BrowserLayer::~BrowserLayer() = default;

void BrowserLayer::OnAttach()
{
	m_viewport->SetHistoryManager(m_historyManager.get());
}

void BrowserLayer::OnDetach()
{
	m_tabManager->CloseAll();
}

void BrowserLayer::OnUpdate(float ts)
{
	CefDoMessageLoopWork();

	for (const auto& tab : m_tabManager->Tabs())
	{
		if (!tab->IsOpen())
		{
			continue;
		}

		tab->UpdateBrowserImage();
		tab->UpdateFaviconImage();

		Walnut::WebViewState state = tab->GetWebViewState();
		if (!state.IsLoading && !state.URL.empty() && !state.Title.empty())
		{
			auto& lastUrl = m_lastRecordedUrls[tab->GetId()];
			if (lastUrl != state.URL)
			{
				m_historyManager->AddVisit(state.URL, state.Title);
				lastUrl = state.URL;
			}
		}
	}
}

void BrowserLayer::OnUIRender()
{
	BuildDockLayout();

	// m_viewportPanel.Render();
	m_sidebarPanel.Render();

	m_viewportPanel.Render();
	// RenderMainViewport();
}

void BrowserLayer::OpenHistoryWindow()
{
	m_viewport->OpenHistoryWindow();
}

void BrowserLayer::BuildDockLayout()
{
	if (m_layoutBuilt)
	{
		return;
	}

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
		{
			dockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDocking;
		}
	};

	lockNode(dockLeft);
	lockNode(dockRight);
	m_layoutBuilt = true;
}

void BrowserLayer::RenderMainViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::Begin("##BrowserViewport", nullptr,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse);

	m_viewport->Render(*m_tabManager); // FIX ME LATER

	ImGui::End();
	ImGui::PopStyleVar();
}
