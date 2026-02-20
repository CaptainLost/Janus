#include "BrowserViewport.h"
#include "../Browser/CefInputBridge.h"

#include "Walnut/Image.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <IconsFontAwesome6.h>

BrowserViewport::BrowserViewport(const std::string& uniqueId)
	: m_uniqueId(uniqueId)
{
}

void BrowserViewport::Render(TabManager& tabManager)
{
	ImGui::PushID(m_uniqueId.c_str());

	RenderTabBar(tabManager);

	ImGui::PopID();
}

void BrowserViewport::UpdateBrowserImage(Tab& tab)
{
	auto webView = tab.GetWebView();
	if (!webView)
	{
		return;
	}

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;

	if (!webView->GetPixelBuffer(buffer, width, height) || width <= 0 || height <= 0)
	{
		return;
	}

	auto image = tab.GetBrowserImage();

	bool needsRecreate =
		!image ||
		image->GetWidth()  != static_cast<uint32_t>(width) ||
		image->GetHeight() != static_cast<uint32_t>(height);

	if (needsRecreate)
	{
		image = std::make_shared<Walnut::Image>(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			Walnut::ImageFormat::RGBA);

		tab.SetBrowserImage(image);
	}

	image->SetData(buffer.data());
}

void BrowserViewport::UpdateFaviconImage(Tab& tab)
{
	auto webView = tab.GetWebView();
	if (!webView)
	{
		return;
	}

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;
	if (!webView->GetFaviconPixels(buffer, width, height) || width <= 0 || height <= 0)
	{
		return;
	}

	auto image = std::make_shared<Walnut::Image>(
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		Walnut::ImageFormat::RGBA);

	image->SetData(buffer.data());
	tab.SetFaviconImage(image);
}

void BrowserViewport::DrawFaviconInTab(Tab& tab)
{
	auto favicon = tab.GetFaviconImage();
	if (!favicon)
	{
		return;
	}

	ImGuiWindow* win = ImGui::GetCurrentWindow();
	ImGuiDockNode* dockNode = win->DockNode;
	if (!dockNode || !dockNode->TabBar)
	{
		return;
	}

	ImGuiTabBar* tabBar = dockNode->TabBar;
	for (ImGuiTabItem& item : tabBar->Tabs)
	{
		if (item.Window != win)
		{
			continue;
		}

		bool isSelected = (tabBar->SelectedTabId == item.ID);
		ImU32 tabBgColor = ImGui::GetColorU32(isSelected ? ImGuiCol_TabSelected : ImGuiCol_Tab);

		float iconSize = tabBar->BarRect.GetHeight() - 6.0f;
		float tabX = tabBar->BarRect.Min.x + item.Offset - tabBar->ScrollingAnim + tabBar->FramePadding.x;
		float tabY = tabBar->BarRect.Min.y + (tabBar->BarRect.GetHeight() - iconSize) * 0.5f;

		ImVec2 iconMin(tabX, tabY);
		ImVec2 iconMax(tabX + iconSize, tabY + iconSize);

		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		drawList->PushClipRect(tabBar->BarRect.Min, tabBar->BarRect.Max, true);
		drawList->AddRectFilled(iconMin, iconMax, tabBgColor);
		drawList->AddImage(favicon->GetDescriptorSet(), iconMin, iconMax);
		drawList->PopClipRect();
		break;
	}
}

void BrowserViewport::RenderTabBar(TabManager& tabManager)
{
	int activeTabId = tabManager.GetActiveTabId();

	ImGuiID dockspaceId = ImGui::GetID("MainViewport");
	ImGui::DockSpace(dockspaceId);

	for (size_t i = 0; i < tabManager.Tabs().size();)
	{
		std::shared_ptr<Tab> tab = tabManager.Tabs()[i];

		ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_Once);

		bool isOpen = true;

		Walnut::WebViewState tabState = tab->GetWebViewState();
		const char* tabIcon = !tab->IsOpen() ? ICON_FA_FILE : (tabState.IsLoading ? ICON_FA_SPINNER : ICON_FA_GLOBE);
		std::string windowTitle = std::string(tabIcon) + " " + tab->GetTabLabel() + "###TabWindow_" + std::to_string(tab->GetId());

		bool contentVisible = ImGui::Begin(windowTitle.c_str(), &isOpen, ImGuiWindowFlags_NoCollapse);
		DrawFaviconInTab(*tab);

		if (contentVisible)
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && activeTabId != tab->GetId())
			{
				tabManager.SetActiveTab(tab->GetId());
			}

			m_addressBar.RenderForTab(tab.get());
			ImGui::Separator();

			RenderTabContent(*tab);
		}
		ImGui::End();

		if (!isOpen)
		{
			tabManager.RemoveTab(tab->GetId());

			continue;
		}

		i++;
	}

	if (m_historyManager)
	{
		m_historyWindow.Render(tabManager, *m_historyManager, dockspaceId);
	}
}

void BrowserViewport::RenderTabContent(Tab& tab)
{
	if (!tab.IsOpen())
	{
		return;
	}

	RenderBrowserContent(tab);
}

void BrowserViewport::RenderBrowserContent(Tab& tab)
{
	ImVec2 region = ImGui::GetContentRegionAvail();
	int newW = (std::max)(static_cast<int>(region.x), 64);
	int newH = (std::max)(static_cast<int>(region.y), 64);

	if (newW != tab.GetViewWidth() || newH != tab.GetViewHeight())
	{
		tab.SetViewSize(newW, newH);
	}

	auto image = tab.GetBrowserImage();
	if (image)
	{
		ImVec2 imagePos = ImGui::GetCursorScreenPos();

		ImGui::Image(
			image->GetDescriptorSet(),
			ImVec2(static_cast<float>(image->GetWidth()),
			       static_cast<float>(image->GetHeight())));

		ForwardInputToBrowser(tab, imagePos);
	}
	else
	{
		ImGui::Text("Loading...");
	}
}

void BrowserViewport::ForwardInputToBrowser(Tab& tab, ImVec2 imagePos)
{
	auto webView = tab.GetWebView();
	if (!webView)
	{
		return;
	}

	auto browser = webView->GetBrowser();
	if (!browser)
	{
		return;
	}

	bool isHovered = ImGui::IsWindowHovered();
	bool isFocused = ImGui::IsWindowFocused();
	if (!isHovered && !isFocused)
	{
		return;
	}

	auto host = browser->GetHost();
	const ImGuiIO& io = ImGui::GetIO();

	int mouseX = static_cast<int>(io.MousePos.x - imagePos.x);
	int mouseY = static_cast<int>(io.MousePos.y - imagePos.y);

	CefInputBridge::ForwardMouseEvents(host, mouseX, mouseY, isHovered);
	CefInputBridge::ForwardKeyboardEvents(host, isFocused);
}
