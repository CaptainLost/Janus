#define IMGUI_DEFINE_MATH_OPERATORS
#include "BrowserLayer.h"

#include "include/cef_app.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

// ============================================================================
// Lifecycle
// ============================================================================

void BrowserLayer::OnAttach()
{
	m_TabManager.AddDynamicTab("https://www.google.com");
}

void BrowserLayer::OnDetach()
{
	m_TabManager.CloseAll();
}

// ============================================================================
// Frame update
// ============================================================================

void BrowserLayer::OnUpdate(float ts)
{
	CefDoMessageLoopWork();

	m_TabManager.PurgeExpiredTabs();

	for (auto& tab : m_TabManager.DynamicTabs())
		if (tab.IsOpen) m_Viewport.UpdateBrowserImage(tab);
	for (auto& tab : m_TabManager.PermanentTabs())
		if (tab.IsOpen) m_Viewport.UpdateBrowserImage(tab);
	for (auto& tab : m_TabManager.TemporaryTabs())
		if (tab.IsOpen) m_Viewport.UpdateBrowserImage(tab);

	if (auto* active = m_TabManager.GetActiveTab())
		SyncURLFromBrowser(*active);
}

void BrowserLayer::OnUIRender()
{
	ImGui::ShowDemoWindow();

	BuildDockLayout();

	RenderSidebar();
	RenderAddressBar();
	m_Viewport.Render(m_TabManager, m_NewTabURL);
	RenderNewTabPopup();
}

// ============================================================================
// Fixed Dock Layout (built once on startup)
// ============================================================================

void BrowserLayer::BuildDockLayout()
{
	if (m_LayoutBuilt)
		return;

	ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");

	// Always force-rebuild the layout to avoid stale imgui.ini placements
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

	// Split: left sidebar (20%) | right remainder (80%)
	ImGuiID dockLeft = 0, dockRight = 0;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, &dockLeft, &dockRight);

	// Split right: top navigation bar (~50px worth) | center browser viewport
	ImGuiID dockTop = 0, dockCenter = 0;
	ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Up, 0.1f, &dockTop, &dockCenter);

	// Dock our windows into their slots
	ImGui::DockBuilderDockWindow("##Sidebar", dockLeft);
	ImGui::DockBuilderDockWindow("##Navigation", dockTop);
	ImGui::DockBuilderDockWindow("##BrowserViewport", dockCenter);

	ImGui::DockBuilderFinish(dockspace_id);

	// Configure nodes: hide tab bars, prevent undocking/resizing
	auto LockNode = [](ImGuiID id) {
		if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(id))
		{
			n->LocalFlags |= ImGuiDockNodeFlags_NoTabBar
			               |  ImGuiDockNodeFlags_NoDocking
			               |  ImGuiDockNodeFlags_NoResize;
		}
	};

	LockNode(dockLeft);
	LockNode(dockTop);
	LockNode(dockCenter);

	m_LayoutBuilt = true;
}

// ============================================================================
// Per-tab helpers
// ============================================================================

void BrowserLayer::SyncURLFromBrowser(BrowserTab& tab)
{
	if (tab.URLBarFocused || !tab.WebView)
		return;

	const auto state = tab.WebView->GetState();
	if (!state.URL.empty())
	{
		std::strncpy(tab.URLBuffer, state.URL.c_str(), sizeof(tab.URLBuffer) - 1);
		tab.URLBuffer[sizeof(tab.URLBuffer) - 1] = '\0';
	}
}

// ============================================================================
// Left Sidebar
// ============================================================================

void BrowserLayer::RenderSidebar()
{
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar;

	ImGui::Begin("##Sidebar", nullptr, flags);

	if (ImGui::Button("+ New Tab", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		m_ShowNewTabPopup = true;

	ImGui::Separator();
	ImGui::Spacing();

	// ---- Dynamic tabs ----
	ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Tabs");
	ImGui::Separator();

	int removeDynID = -1;
	for (auto& tab : m_TabManager.DynamicTabs())
	{
		ImGui::PushID(tab.Id);

		Walnut::WebViewState state = tab.GetState();
		std::string title = state.Title.empty() ? "New Tab" : state.Title;
		if (title.size() > 25)
			title = title.substr(0, 22) + "...";
		if (state.IsLoading)
			title = "[*] " + title;

		bool isActive = (tab.Id == m_TabManager.GetActiveTabID());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(title.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
			m_TabManager.SetActiveTab(tab.Id);

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("X"))
			removeDynID = tab.Id;

		ImGui::PopID();
	}

	if (removeDynID >= 0)
		m_TabManager.RemoveDynamicTab(removeDynID);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// ---- Permanent tabs ----
	ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Permanent");
	ImGui::Separator();

	int removePermID = -1;
	for (auto& tab : m_TabManager.PermanentTabs())
	{
		ImGui::PushID(tab.Id);

		auto state = tab.GetState();
		std::string title = state.Title.empty() ? tab.StartURL : state.Title;
		if (title.size() > 25)
			title = title.substr(0, 22) + "...";

		const char* icon = tab.IsOpen ? "[O]" : "[-]";
		std::string label = std::string(icon) + " " + title;
		if (tab.IsOpen && state.IsLoading)
			label += " [*]";

		bool isActive = (tab.Id == m_TabManager.GetActiveTabID());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
			m_TabManager.OpenTab(tab.Id);

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("X"))
			removePermID = tab.Id;

		ImGui::PopID();
	}

	if (removePermID >= 0)
		m_TabManager.RemovePermanentTab(removePermID);

	if (ImGui::SmallButton("+ Permanent"))
	{
		m_ShowNewTabPopup = true;
		m_NewTabKind = 1;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// ---- Temporary tabs ----
	ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Temporary");
	ImGui::Separator();

	int removeTempID = -1;
	for (auto& tab : m_TabManager.TemporaryTabs())
	{
		ImGui::PushID(tab.Id);

		auto state = tab.GetState();
		std::string title = state.Title.empty() ? tab.StartURL : state.Title;
		if (title.size() > 20)
			title = title.substr(0, 17) + "...";

		const char* icon = tab.IsOpen ? "[O]" : "[-]";
		std::string remaining = tab.GetRemainingTimeString();
		std::string label = std::string(icon) + " " + title;
		if (tab.IsOpen && state.IsLoading)
			label += " [*]";
		label += " (" + remaining + ")";

		bool isActive = (tab.Id == m_TabManager.GetActiveTabID());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
			m_TabManager.OpenTab(tab.Id);

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("X"))
			removeTempID = tab.Id;

		ImGui::PopID();
	}

	if (removeTempID >= 0)
		m_TabManager.RemoveTemporaryTab(removeTempID);

	if (ImGui::SmallButton("+ Temporary"))
	{
		m_ShowNewTabPopup = true;
		m_NewTabKind = 2;
	}

	ImGui::End();
}

// ============================================================================
// Address / Navigation Bar
// ============================================================================

void BrowserLayer::RenderAddressBar()
{
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::Begin("##Navigation", nullptr, flags);

	auto* tab = m_TabManager.GetActiveTab();

	if (!tab || !tab->IsOpen)
	{
		ImGui::TextDisabled("No active tab");
		ImGui::End();
		return;
	}

	const auto state = tab->GetState();

	// Back
	ImGui::BeginDisabled(!state.CanGoBack);
	if (ImGui::Button("<") && tab->WebView) tab->WebView->GoBack();
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Forward
	ImGui::BeginDisabled(!state.CanGoForward);
	if (ImGui::Button(">") && tab->WebView) tab->WebView->GoForward();
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Stop / Reload
	if (state.IsLoading)
	{
		if (ImGui::Button("X") && tab->WebView) tab->WebView->StopLoading();
	}
	else
	{
		if (ImGui::Button("O") && tab->WebView) tab->WebView->Reload();
	}

	ImGui::SameLine();

	// URL input
	const float goWidth = ImGui::CalcTextSize("Go").x +
	                      ImGui::GetStyle().ItemSpacing.x * 2 +
	                      ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - goWidth);

	const bool enterPressed = ImGui::InputText(
		"##url", tab->URLBuffer, sizeof(tab->URLBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue);
	tab->URLBarFocused = ImGui::IsItemActive();

	ImGui::SameLine();

	if ((enterPressed || ImGui::Button("Go")) && tab->WebView)
		tab->WebView->Navigate(tab->URLBuffer);

	ImGui::End();
}

// ============================================================================
// "New Tab" Popup
// ============================================================================

void BrowserLayer::RenderNewTabPopup()
{
	if (!m_ShowNewTabPopup)
		return;

	ImGui::OpenPopup("New Tab");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(450, 220), ImGuiCond_Appearing);

	if (ImGui::BeginPopupModal("New Tab", &m_ShowNewTabPopup, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Tab type:");
		ImGui::RadioButton("Dynamic",   &m_NewTabKind, 0); ImGui::SameLine();
		ImGui::RadioButton("Permanent", &m_NewTabKind, 1); ImGui::SameLine();
		ImGui::RadioButton("Temporary", &m_NewTabKind, 2);

		ImGui::Spacing();

		ImGui::Text("URL:");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##newurl", m_NewTabURL, sizeof(m_NewTabURL));

		if (m_NewTabKind == 2)
		{
			ImGui::Spacing();
			ImGui::Text("Lifetime (hours):");
			ImGui::SetNextItemWidth(150);
			ImGui::InputFloat("##lifetime", &m_NewTabLifetimeHours, 1.0f, 6.0f, "%.1f");
			if (m_NewTabLifetimeHours < 0.01f) m_NewTabLifetimeHours = 0.01f;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			switch (m_NewTabKind)
			{
			case 0:
				m_TabManager.AddDynamicTab(m_NewTabURL);
				break;
			case 1:
			{
				int id = m_TabManager.AddPermanentTab(m_NewTabURL);
				m_TabManager.OpenTab(id);
				break;
			}
			case 2:
			{
				double secs = static_cast<double>(m_NewTabLifetimeHours) * 3600.0;
				int id = m_TabManager.AddTemporaryTab(m_NewTabURL, secs);
				m_TabManager.OpenTab(id);
				break;
			}
			}

			std::strncpy(m_NewTabURL, "https://www.google.com", sizeof(m_NewTabURL));
			m_NewTabLifetimeHours = 24.0f;
			m_ShowNewTabPopup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			m_ShowNewTabPopup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
