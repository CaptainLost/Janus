#include "BrowserViewport.h"
#include "CefInputBridge.h"

#include "Walnut/Image.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstring>

static constexpr const char* kTabDndType = "TAB_DND";

TabDragState BrowserViewport::s_dragState;

int BrowserViewport::StringResizeCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		auto* str = static_cast<std::string*>(data->UserData);
		str->resize(data->BufTextLen);
		data->Buf = str->data();
	}
	return 0;
}

BrowserViewport::BrowserViewport(const std::string& uniqueId)
	: m_uniqueId(uniqueId)
{
}

void BrowserViewport::SetDetachCallback(DetachCallback cb)
{
	m_detachCallback = std::move(cb);
}

void BrowserViewport::Render(TabManager2& tabManager)
{
	ImGui::PushID(m_uniqueId.c_str());

	RenderAddressBar(tabManager);
	ImGui::Separator();
	RenderTabBar(tabManager);

	if (auto* activeTab = tabManager.GetActiveTab())
		RenderTabContent(*activeTab);

	if (s_dragState.active && s_dragState.srcViewport == this)
		RenderDragOverlay();

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

void BrowserViewport::RenderAddressBar(TabManager2& tabManager)
{
	auto* tab = tabManager.GetActiveTab();
	if (!tab)
	{
		ImGui::TextDisabled("No active tab");
		return;
	}

	bool hasWebView = tab->GetState() != TabState::Blank;
	Walnut::WebViewState state;
	CefRefPtr<Walnut::WebView> webView;

	if (hasWebView)
	{
		state   = tab->GetWebViewState();
		webView = tab->GetWebView();
	}

	ImGui::BeginDisabled(!hasWebView || !state.CanGoBack);
	if (ImGui::Button("<") && webView) webView->GoBack();
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(!hasWebView || !state.CanGoForward);
	if (ImGui::Button(">") && webView) webView->GoForward();
	ImGui::EndDisabled();

	ImGui::SameLine();

	if (hasWebView && state.IsLoading)
	{
		if (ImGui::Button("X") && webView) webView->StopLoading();
	}
	else
	{
		ImGui::BeginDisabled(!hasWebView);
		if (ImGui::Button("O") && webView) webView->Reload();
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	std::string& url = tab->GetUrlInput();
	if (url.capacity() < 2048)
		url.reserve(2048);

	if (hasWebView && !m_urlBarFocused && !state.URL.empty())
		url = state.URL;

	float goWidth = ImGui::CalcTextSize("Go").x
	              + ImGui::GetStyle().ItemSpacing.x * 2
	              + ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - goWidth);

	bool enterPressed = ImGui::InputText(
		"##nav_url",
		url.data(),
		url.capacity() + 1,
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize,
		StringResizeCallback,
		&url);

	m_urlBarFocused = ImGui::IsItemActive();
	url.resize(std::strlen(url.c_str()));

	ImGui::SameLine();

	bool goPressed = ImGui::Button("Go");

	if ((enterPressed || goPressed) && !url.empty())
	{
		int currentTabId = tab->GetId();

		if (hasWebView && webView)
			tab->NavigateToUrl(url);
		else
			tab->Open(url);

		// Force this tab to stay selected even if TabManager's active ID changes
		tabManager.SetActiveTab(currentTabId);
		m_pendingSelectTabId = currentTabId;
	}
}

void BrowserViewport::RenderTabBar(TabManager2& tabManager)
{
	if (!ImGui::BeginTabBar("##BrowserTabs"))
		return;

	const auto& tabs = tabManager.Tabs();
	bool tabWasRemoved = false;

	for (int i = 0; i < static_cast<int>(tabs.size()) && !tabWasRemoved; )
	{
		auto& tab = *tabs[i];
		int id = tab.GetId();

		std::string label;
		if (tab.GetState() == TabState::Blank)
		{
			label = "New Tab";
		}
		else
		{
			auto state = tab.GetWebViewState();
			label = state.Title.empty() ? "Loading..." : state.Title;
			if (label.size() > 25)
				label = label.substr(0, 22) + "...";
			if (state.IsLoading)
				label = "[*] " + label;
		}
		label += "##tab_" + std::to_string(id);

		bool isOpen = true;
		ImGuiTabItemFlags itemFlags = ImGuiTabItemFlags_None;

		if (id == m_pendingSelectTabId)
			itemFlags |= ImGuiTabItemFlags_SetSelected;

		bool tabIsVisible = ImGui::BeginTabItem(label.c_str(), &isOpen, itemFlags);

		if (tabIsVisible)
		{
			// Let ImGui's tab bar decide which tab is active — BUT:
			// if we have a pending selection, enforce it and ignore ImGui.
			if (m_pendingSelectTabId >= 0)
			{
				// We're forcing a selection. Only allow SetActiveTab if it's the right one.
				if (id == m_pendingSelectTabId)
				{
					tabManager.SetActiveTab(id);
					m_pendingSelectTabId = -1;
				}
				// else: ignore this tab becoming visible — we're overriding selection
			}
			else
			{
				// Normal mode: let ImGui's visible tab become active
				if (tabManager.GetActiveTabId() != id)
					tabManager.SetActiveTab(id);
			}

			ImGui::EndTabItem();
		}

		bool itemHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 8.0f) && !s_dragState.active)
		{
			s_dragState.active      = true;
			s_dragState.tabId       = id;
			s_dragState.srcManager  = &tabManager;
			s_dragState.srcViewport = this;
		}

		if (s_dragState.active && s_dragState.srcManager != &tabManager && itemHovered
		    && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			if (auto movedTab = s_dragState.srcManager->DetachTab(s_dragState.tabId))
				m_pendingSelectTabId = tabManager.AcceptTab(std::move(movedTab));
			s_dragState.Clear();
			tabWasRemoved = true;
		}

		if (!isOpen)
		{
			tabManager.RemoveTab(id);
			tabWasRemoved = true;
		}
		else
		{
			++i;
		}
	}

	// Accept drop on tab bar background (from another viewport)
	if (s_dragState.active && s_dragState.srcManager != &tabManager
	    && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
	    && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (auto movedTab = s_dragState.srcManager->DetachTab(s_dragState.tabId))
			m_pendingSelectTabId = tabManager.AcceptTab(std::move(movedTab));
		s_dragState.Clear();
	}

	// Finish drag: mouse released by the source viewport
	if (s_dragState.active && s_dragState.srcViewport == this && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		ImVec2 windowPos  = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 mousePos   = ImGui::GetMousePos();

		constexpr float margin = 20.0f;
		bool outside =
			mousePos.x < windowPos.x - margin ||
			mousePos.y < windowPos.y - margin ||
			mousePos.x > windowPos.x + windowSize.x + margin ||
			mousePos.y > windowPos.y + windowSize.y + margin;

		if (outside && m_detachCallback)
			m_detachCallback(tabManager, s_dragState.tabId, mousePos);

		s_dragState.Clear();
	}

	if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
		m_pendingSelectTabId = tabManager.AddTab();

	ImGui::EndTabBar();
}

void BrowserViewport::RenderDragOverlay()
{
	auto* tab = s_dragState.srcManager ? s_dragState.srcManager->FindTab(s_dragState.tabId) : nullptr;
	if (!tab)
		return;

	std::string dragLabel;
	if (tab->GetState() == TabState::Blank)
	{
		dragLabel = "New Tab";
	}
	else
	{
		auto st = tab->GetWebViewState();
		dragLabel = st.Title.empty() ? "Tab" : st.Title;
		if (dragLabel.size() > 30)
			dragLabel = dragLabel.substr(0, 27) + "...";
	}

	ImVec2 mousePos = ImGui::GetMousePos();
	ImGui::SetNextWindowPos(ImVec2(mousePos.x + 15, mousePos.y + 10));
	ImGui::SetNextWindowBgAlpha(0.85f);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoInputs;

	ImGui::Begin("##TabDragTooltip", nullptr, flags);
	ImGui::TextUnformatted(dragLabel.c_str());
	ImGui::End();
}

void BrowserViewport::RenderTabContent(BrowserTab2& tab)
{
	if (tab.GetState() == TabState::Blank)
		return; // address bar handles URL entry for blank tabs
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
	if (s_dragState.active)
		return;

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
