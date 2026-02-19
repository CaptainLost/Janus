#include "BrowserViewport.h"
#include "../Browser/CefInputBridge.h"

#include "Walnut/Image.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <IconsFontAwesome6.h>

static constexpr const char* kTabDndType = "TAB_DND";

BrowserViewport::BrowserViewport(const std::string& uniqueId)
	: m_uniqueId(uniqueId)
{
}

void BrowserViewport::Render(TabManager2& tabManager)
{
	ImGui::PushID(m_uniqueId.c_str());

	RenderTabBar(tabManager);

	ImGui::PopID();
}

void BrowserViewport::UpdateBrowserImage(BrowserTab2& tab)
{
	auto webView = tab.GetWebView();
	if (!webView)
		return;

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;

	if (!webView->GetPixelBuffer(buffer, width, height) || width <= 0 || height <= 0)
		return;

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

void BrowserViewport::RenderTabBar(TabManager2& tabManager)
{
	int activeTabId = tabManager.GetActiveTabId();

	ImGuiID dockspaceId = ImGui::GetID("MainViewport");
	ImGui::DockSpace(dockspaceId);

	for (size_t i = 0; i < tabManager.Tabs().size();)
	{
		std::shared_ptr<BrowserTab2> tab = tabManager.Tabs()[i];

		ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_Once);

		bool isOpen = true;

		std::string windowTitle = ICON_FA_FILE " " + tab->GetTabLabel() + "###TabWindow_" + std::to_string(tab->GetId());

		if (ImGui::Begin(windowTitle.c_str(), &isOpen, ImGuiWindowFlags_NoCollapse))
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
		m_historyWindow.Render(tabManager, *m_historyManager, dockspaceId);
}

void BrowserViewport::RenderTabContent(BrowserTab2& tab)
{
	if (!tab.IsOpen())
		return;

	RenderBrowserContent(tab);
}

void BrowserViewport::RenderBrowserContent(BrowserTab2& tab)
{
	ImVec2 region = ImGui::GetContentRegionAvail();
	int newW = (std::max)(static_cast<int>(region.x), 64);
	int newH = (std::max)(static_cast<int>(region.y), 64);

	if (newW != tab.GetViewWidth() || newH != tab.GetViewHeight())
		tab.SetViewSize(newW, newH);

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

void BrowserViewport::ForwardInputToBrowser(BrowserTab2& tab, ImVec2 imagePos)
{
	auto webView = tab.GetWebView();
	if (!webView)
		return;

	auto browser = webView->GetBrowser();
	if (!browser)
		return;

	bool isHovered = ImGui::IsWindowHovered();
	bool isFocused = ImGui::IsWindowFocused();
	if (!isHovered && !isFocused)
		return;

	auto host = browser->GetHost();
	const ImGuiIO& io = ImGui::GetIO();

	int mouseX = static_cast<int>(io.MousePos.x - imagePos.x);
	int mouseY = static_cast<int>(io.MousePos.y - imagePos.y);

	CefInputBridge::ForwardMouseEvents(host, mouseX, mouseY, isHovered);
	CefInputBridge::ForwardKeyboardEvents(host, isFocused);
}
