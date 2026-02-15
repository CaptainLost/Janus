#define IMGUI_DEFINE_MATH_OPERATORS
#include "BrowserViewport.h"
#include "CefInputBridge.h"

#include "imgui.h"

#include <algorithm>
#include <iostream>

// ============================================================================
// Public API
// ============================================================================

void BrowserViewport::Render(TabManager& tabManager, const std::string& newTabURL)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar;

	ImGui::Begin("##BrowserViewport", nullptr, flags);

	RenderTabBar(tabManager, newTabURL);

	auto* activeTab = tabManager.GetActiveTab();
	RenderBrowserContent(activeTab);

	if (activeTab && activeTab->IsOpen)
		ForwardInputToBrowser(*activeTab);

	ImGui::End();
	ImGui::PopStyleVar();
}

void BrowserViewport::UpdateBrowserImage(BrowserTab& tab)
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

void BrowserViewport::ForwardInputToBrowser(BrowserTab& tab)
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
// Private Helpers
// ============================================================================

void BrowserViewport::RenderTabBar(TabManager& tabManager, const std::string& newTabURL)
{
	if (ImGui::BeginTabBar("TabTabs"))
	{
		for (int i = 0; i < tabManager.DynamicTabs().size();)
		{
			BrowserTab& tab = tabManager.DynamicTabs()[i];

			Walnut::WebViewState state = tab.GetState();

			bool isOpen = true;

			ImGui::PushID(tab.Id);

			if (ImGui::BeginTabItem(state.Title.c_str(), &isOpen, ImGuiTabItemFlags_None))
			{
				if (tabManager.GetActiveTabID() != tab.Id)
					tabManager.SetActiveTab(tab.Id);

				ImGui::EndTabItem();
			}

			ImGui::PopID();

			if (!isOpen)
			{
				tab.Close();
				tabManager.DynamicTabs().erase(tabManager.DynamicTabs().begin() + i);
			}
			else
			{
				i++;
			}
		}

		if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
			tabManager.AddDynamicTab(newTabURL);

		ImGui::EndTabBar();
	}
}

void BrowserViewport::RenderBrowserContent(BrowserTab* activeTab)
{
	if (!activeTab || !activeTab->IsOpen)
	{
		ImGui::Text("No active tab. Create one with '+ New Tab' in the sidebar.");
		return;
	}

	// Resize WebView to match available area
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const int newW = (std::max)(static_cast<int>(region.x), 64);
	const int newH = (std::max)(static_cast<int>(region.y), 64);

	if ((newW != activeTab->ViewWidth || newH != activeTab->ViewHeight) && activeTab->WebView)
	{
		activeTab->ViewWidth  = newW;
		activeTab->ViewHeight = newH;
		activeTab->WebView->SetViewSize(activeTab->ViewWidth, activeTab->ViewHeight);
	}

	// Render the page
	if (activeTab->BrowserImage)
	{
		ImGui::Image(activeTab->BrowserImage->GetDescriptorSet(),
		             ImVec2(static_cast<float>(activeTab->BrowserImage->GetWidth()),
		                    static_cast<float>(activeTab->BrowserImage->GetHeight())));
	}
	else
	{
		// TODO: Center
		ImGui::Text("Browser starting...");
	}
}
