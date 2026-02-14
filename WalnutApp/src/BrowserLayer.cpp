#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "BrowserLayer.h"
#include "CefInputBridge.h"

#include "include/cef_app.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

// ============================================================================
// Lifecycle
// ============================================================================

void BrowserLayer::OnAttach()
{
	// Start with one dynamic tab
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

	// Purge expired temporary tabs
	m_TabManager.PurgeExpiredTabs();

	// Update every open tab's pixel buffer
	for (auto& tab : m_TabManager.DynamicTabs())
		if (tab.IsOpen) UpdateBrowserImage(tab);
	for (auto& tab : m_TabManager.PermanentTabs())
		if (tab.IsOpen) UpdateBrowserImage(tab);
	for (auto& tab : m_TabManager.TemporaryTabs())
		if (tab.IsOpen) UpdateBrowserImage(tab);

	// Sync URL bar only for the active tab
	if (auto* active = m_TabManager.GetActiveTab())
		SyncURLFromBrowser(*active);
}

void BrowserLayer::OnUIRender()
{
	ImGui::ShowDemoWindow();

	RenderTopTabBar();
	RenderSidebar();
	RenderAddressBar();
	RenderBrowserViewport();
	RenderNewTabPopup();
}

// ============================================================================
// Per-tab helpers
// ============================================================================

void BrowserLayer::UpdateBrowserImage(BrowserTab& tab)
{
	if (!tab.WebView)
		return;

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;

	if (!tab.WebView->GetPixelBuffer(buffer, width, height))
		return;
	if (width <= 0 || height <= 0)
		return;

	const bool needsRecreate =
		!tab.BrowserImage ||
		tab.BrowserImage->GetWidth()  != static_cast<uint32_t>(width) ||
		tab.BrowserImage->GetHeight() != static_cast<uint32_t>(height);

	if (needsRecreate)
	{
		tab.BrowserImage = std::make_shared<Walnut::Image>(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			Walnut::ImageFormat::RGBA);
	}

	tab.BrowserImage->SetData(buffer.data());
}

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

void BrowserLayer::ForwardInputToBrowser(BrowserTab& tab)
{
	if (!tab.WebView)
		return;

	auto browser = tab.WebView->GetBrowser();
	if (!browser)
		return;

	const bool isHovered = ImGui::IsWindowHovered();
	const bool isFocused = ImGui::IsWindowFocused();

	if (!isHovered && !isFocused)
		return;

	auto host = browser->GetHost();

	const ImVec2 windowPos  = ImGui::GetWindowPos();
	const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
	const ImGuiIO& io       = ImGui::GetIO();

	const int mouseX = static_cast<int>(io.MousePos.x - windowPos.x - contentMin.x);
	const int mouseY = static_cast<int>(io.MousePos.y - windowPos.y - contentMin.y);

	CefInputBridge::ForwardMouseEvents(host, mouseX, mouseY, isHovered);
	CefInputBridge::ForwardKeyboardEvents(host, isFocused);
}

// ============================================================================
// Top Tab Bar (Dynamic Tabs)
// ============================================================================

void BrowserLayer::RenderTopTabBar()
{
	ImGui::Begin("Tabs", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	// "+" button to add a new tab
	if (ImGui::Button("+"))
		m_ShowNewTabPopup = true;

	ImGui::SameLine();

	// Toggle sidebar button
	if (ImGui::Button(m_SidebarOpen ? "<<" : ">>"))
		m_SidebarOpen = !m_SidebarOpen;

	ImGui::SameLine();
	ImGui::Text("|");
	ImGui::SameLine();

	// Dynamic tabs rendered like browser tabs
	int removeID = -1;
	for (auto& tab : m_TabManager.DynamicTabs())
	{
		auto state = tab.GetState();
		std::string title = state.Title.empty() ? "New Tab" : state.Title;

		// Truncate long titles
		if (title.size() > 25)
			title = title.substr(0, 22) + "...";

		// Loading indicator
		std::string label;
		if (state.IsLoading)
			label = "[*] " + title;
		else
			label = title;

		// Highlight active tab
		bool isActive = (tab.ID == m_TabManager.GetActiveTabID());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		ImGui::PushID(tab.ID);

		if (ImGui::Button(label.c_str()))
			m_TabManager.SetActiveTab(tab.ID);

		ImGui::SameLine();

		// Close button
		if (ImGui::SmallButton("X"))
			removeID = tab.ID;

		ImGui::PopID();

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::Text("|");
		ImGui::SameLine();
	}

	ImGui::End();

	// Remove tab after iteration
	if (removeID >= 0)
		m_TabManager.RemoveDynamicTab(removeID);
}

// ============================================================================
// Left Sidebar (Permanent + Temporary Tabs)
// ============================================================================

void BrowserLayer::RenderSidebar()
{
	if (!m_SidebarOpen)
		return;

	ImGui::Begin("Pinned Tabs", &m_SidebarOpen, ImGuiWindowFlags_NoCollapse);

	// ---- Permanent section ----
	ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Permanent");
	ImGui::Separator();

	int removePermID = -1;
	for (auto& tab : m_TabManager.PermanentTabs())
	{
		ImGui::PushID(tab.ID);

		auto state = tab.GetState();
		std::string title = state.Title.empty() ? tab.StartURL : state.Title;
		if (title.size() > 30)
			title = title.substr(0, 27) + "...";

		// Open/closed indicator
		const char* statusIcon = tab.IsOpen ? "[O]" : "[-]";
		// Loading indicator
		bool loading = tab.IsOpen && state.IsLoading;

		// Compose label
		std::string label = std::string(statusIcon) + " " + title;
		if (loading)
			label += " [*]";

		bool isActive = (tab.ID == m_TabManager.GetActiveTabID());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
		{
			m_TabManager.OpenTab(tab.ID);
		}

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("X"))
			removePermID = tab.ID;

		ImGui::PopID();
	}

	if (removePermID >= 0)
		m_TabManager.RemovePermanentTab(removePermID);

	// Button to add permanent tab
	if (ImGui::SmallButton("+ Permanent"))
	{
		m_ShowNewTabPopup = true;
		m_NewTabKind = 1; // Permanent
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// ---- Separator between permanent and temporary ----
	ImGui::Separator();
	ImGui::Separator();

	ImGui::Spacing();

	// ---- Temporary section ----
	ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Temporary");
	ImGui::Separator();

	int removeTempID = -1;
	for (auto& tab : m_TabManager.TemporaryTabs())
	{
		ImGui::PushID(tab.ID);

		auto state = tab.GetState();
		std::string title = state.Title.empty() ? tab.StartURL : state.Title;
		if (title.size() > 25)
			title = title.substr(0, 22) + "...";
		// Open/closed indicator
		const char* statusIcon = tab.IsOpen ? "[O]" : "[-]";
		bool loading = tab.IsOpen && state.IsLoading;

		// Remaining time
		std::string remaining = tab.GetRemainingTimeString();

		std::string label = std::string(statusIcon) + " " + title;
		if (loading)
			label += " [*]";
		label += " (" + remaining + ")";

		bool isActive = (tab.ID == m_TabManager.GetActiveTabID());
		if (isActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
		{
			m_TabManager.OpenTab(tab.ID);
		}

		if (isActive)
			ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::SmallButton("X"))
			removeTempID = tab.ID;

		ImGui::PopID();
	}

	if (removeTempID >= 0)
		m_TabManager.RemoveTemporaryTab(removeTempID);

	// Button to add temporary tab
	if (ImGui::SmallButton("+ Temporary"))
	{
		m_ShowNewTabPopup = true;
		m_NewTabKind = 2; // Temporary
	}

	ImGui::End();
}

// ============================================================================
// Address Bar (for the active tab)
// ============================================================================

void BrowserLayer::RenderAddressBar()
{
	auto* tab = m_TabManager.GetActiveTab();

	ImGui::Begin("Navigation", nullptr, ImGuiWindowFlags_NoCollapse);

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

	// Title
	if (!state.Title.empty())
		ImGui::TextWrapped("Title: %s", state.Title.c_str());

	if (state.IsLoading)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), " Loading...");
	}

	ImGui::End();
}

// ============================================================================
// Browser Viewport (for the active tab)
// ============================================================================

void BrowserLayer::RenderBrowserViewport()
{
	auto* tab = m_TabManager.GetActiveTab();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Browser", nullptr,
	             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	if (!tab || !tab->IsOpen)
	{
		ImGui::Text("No active tab. Create one with '+' button.");
		ImGui::End();
		ImGui::PopStyleVar();
		return;
	}

	// Resize
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const int newW = (std::max)(static_cast<int>(region.x), 64);
	const int newH = (std::max)(static_cast<int>(region.y), 64);

	if ((newW != tab->ViewWidth || newH != tab->ViewHeight) && tab->WebView)
	{
		tab->ViewWidth  = newW;
		tab->ViewHeight = newH;
		tab->WebView->SetViewSize(tab->ViewWidth, tab->ViewHeight);
	}

	// Display
	if (tab->BrowserImage)
	{
		ImGui::Image(tab->BrowserImage->GetDescriptorSet(),
		             ImVec2(static_cast<float>(tab->BrowserImage->GetWidth()),
		                    static_cast<float>(tab->BrowserImage->GetHeight())));
	}
	else
	{
		ImGui::Text("Browser starting...");
	}

	// Input forwarding
	ForwardInputToBrowser(*tab);

	ImGui::End();
	ImGui::PopStyleVar();
}

// ============================================================================
// "New Tab" Popup Dialog
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
		// Tab type selector
		ImGui::Text("Tab type:");
		ImGui::RadioButton("Dynamic",   &m_NewTabKind, 0); ImGui::SameLine();
		ImGui::RadioButton("Permanent", &m_NewTabKind, 1); ImGui::SameLine();
		ImGui::RadioButton("Temporary", &m_NewTabKind, 2);

		ImGui::Spacing();

		// URL
		ImGui::Text("URL:");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##newurl", m_NewTabURL, sizeof(m_NewTabURL));

		// Lifetime (only for temporary)
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

			// Reset
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
