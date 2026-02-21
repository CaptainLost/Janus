#include "BrowserViewport.h"
#include "../Utils/UrlUtils.h"
#include "../Utils/FileDialogs.h"

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

	ImGuiID targetDockId = dockspaceId;
	{
		auto it = m_tabWindowIds.find(activeTabId);
		if (it != m_tabWindowIds.end())
		{
			ImGuiWindow* activeWindow = ImGui::FindWindowByID(it->second);
			if (activeWindow && activeWindow->DockId != 0)
			{
				targetDockId = activeWindow->DockId;
			}
		}
	}

	for (size_t i = 0; i < tabManager.Tabs().size();)
	{
		std::shared_ptr<Tab> tab = tabManager.Tabs()[i];

		if (!m_dockedTabIds.count(tab->GetId()))
		{
			ImGui::SetNextWindowDockID(targetDockId, ImGuiCond_Always);
			m_dockedTabIds.insert(tab->GetId());
		}

		bool isOpen = true;

		Walnut::WebViewState tabState = tab->GetWebViewState();
		const char* tabIcon = !tab->IsOpen() ? ICON_FA_FILE : (tabState.IsLoading ? ICON_FA_SPINNER : ICON_FA_GLOBE);
		std::string windowTitle = std::string(tabIcon) + " " + tab->GetTabLabel() + "###TabWindow_" + std::to_string(tab->GetId());

		bool contentVisible = ImGui::Begin(windowTitle.c_str(), &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

		m_tabWindowIds[tab->GetId()] = ImGui::GetCurrentWindow()->ID;

		DrawFaviconInTab(*tab);

		if (contentVisible)
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && activeTabId != tab->GetId())
			{
				tabManager.SetActiveTab(tab->GetId());
			}

			m_addressBar.RenderForTab(tab.get());
			ImGui::Separator();

			RenderTabContent(*tab, tabManager);
		}
		ImGui::End();

		if (!isOpen)
		{
			tabManager.RemoveTab(tab->GetId());
			m_dockedTabIds.erase(tab->GetId());
			m_tabWindowIds.erase(tab->GetId());

			continue;
		}

		i++;
	}

	if (m_historyManager)
	{
		m_historyWindow.Render(tabManager, *m_historyManager, dockspaceId);
	}
}

void BrowserViewport::RenderTabContent(Tab& tab, TabManager& tabManager)
{
	if (!tab.IsOpen())
	{
		return;
	}

	RenderBrowserContent(tab, tabManager);
}

void BrowserViewport::RenderBrowserContent(Tab& tab, TabManager& tabManager)
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

		Walnut::ContextMenuRequest contextRequest;
		if (tab.GetContextMenuRequest(contextRequest))
		{
			m_activeContextMenu.params = contextRequest;
			m_activeContextMenu.screenPosition = {
				imagePos.x + static_cast<float>(contextRequest.x),
				imagePos.y + static_cast<float>(contextRequest.y)
			};
			m_activeContextMenu.requestOpen = true;
		}

		if (m_activeContextMenu.requestOpen)
		{
			m_activeContextMenu.requestOpen = false;
			ImGui::OpenPopup("##BrowserCtxMenu");
		}

		ImGui::SetNextWindowPos(m_activeContextMenu.screenPosition, ImGuiCond_Appearing);
		if (ImGui::BeginPopup("##BrowserCtxMenu"))
		{
			RenderContextMenu(tab, tabManager);
			ImGui::EndPopup();
		}
	}
	else
	{
		ImGui::Text("Loading...");
	}
}

void BrowserViewport::RenderContextMenu(Tab& tab, TabManager& tabManager)
{
	const Walnut::ContextMenuRequest& params = m_activeContextMenu.params;

	if (params.hasLink)
	{
		if (ImGui::MenuItem("Open Link in New Tab"))
		{
			int newTabId = tabManager.AddTab();
			tabManager.GetTab(newTabId)->Open(params.linkUrl);
			tabManager.SetActiveTab(newTabId);
		}

		if (ImGui::MenuItem("Copy Link Address"))
		{
			ImGui::SetClipboardText(params.linkUrl.c_str());
		}

		ImGui::Separator();
	}

	if (params.hasImage)
	{
		if (ImGui::MenuItem("Open Image in New Tab"))
		{
			int newTabId = tabManager.AddTab();
			tabManager.GetTab(newTabId)->Open(params.sourceUrl);
			tabManager.SetActiveTab(newTabId);
		}

		if (ImGui::MenuItem("Copy Image Address"))
		{
			ImGui::SetClipboardText(params.sourceUrl.c_str());
		}

		if (ImGui::MenuItem("Save Image As..."))
		{
			std::string savePath = FileDialogs::ShowSaveImageDialog(params.sourceUrl);
			if (!savePath.empty())
			{
				tab.SetPendingDownloadPath(savePath);
				tab.StartDownload(params.sourceUrl);
			}
		}

		ImGui::Separator();
	}

	if (params.isEditable)
	{
		if (params.hasSelection)
		{
			if (ImGui::MenuItem("Cut"))
			{
				tab.BrowserCut();
			}

			if (ImGui::MenuItem("Copy"))
			{
				tab.BrowserCopy();
			}

			ImGui::Separator();
		}

		if (ImGui::MenuItem("Paste"))
		{
			tab.BrowserPaste();
		}

		if (ImGui::MenuItem("Select All"))
		{
			tab.BrowserSelectAll();
		}

		ImGui::Separator();
	}
	else if (params.hasSelection)
	{
		if (ImGui::MenuItem("Copy"))
		{
			tab.BrowserCopy();
		}

		std::string searchLabel = "Search Google for \"" + params.selectionText.substr(0, 32) +
		                          (params.selectionText.size() > 32 ? "..." : "") + "\"";
		if (ImGui::MenuItem(searchLabel.c_str()))
		{
			std::string searchUrl = "https://www.google.com/search?q=" + UrlUtils::UrlEncodeQuery(params.selectionText);
			int newTabId = tabManager.AddTab();
			tabManager.GetTab(newTabId)->Open(searchUrl);
			tabManager.SetActiveTab(newTabId);
		}

		ImGui::Separator();
	}

	if (params.canGoBack)
	{
		if (ImGui::MenuItem("Back"))
		{
			tab.GoBack();
		}
	}

	if (params.canGoForward)
	{
		if (ImGui::MenuItem("Forward"))
		{
			tab.GoForward();
		}
	}

	if (ImGui::MenuItem("Reload"))
	{
		tab.ReloadPage();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("View Page Source"))
	{
		std::string sourceUrl = "view-source:" + params.pageUrl;
		int newTabId = tabManager.AddTab();
		tabManager.GetTab(newTabId)->Open(sourceUrl);
		tabManager.SetActiveTab(newTabId);
	}
}

void BrowserViewport::ForwardInputToBrowser(Tab& tab, ImVec2 imagePos)
{
	bool isHovered = ImGui::IsWindowHovered();
	bool isFocused = ImGui::IsWindowFocused();
	if (!isHovered && !isFocused)
	{
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	int mouseX = static_cast<int>(io.MousePos.x - imagePos.x);
	int mouseY = static_cast<int>(io.MousePos.y - imagePos.y);

	tab.ForwardInput(mouseX, mouseY, isHovered, isFocused);
}
