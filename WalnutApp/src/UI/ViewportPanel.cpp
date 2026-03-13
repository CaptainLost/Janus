#include "ViewportPanel.h"
#include <imgui.h>
#include <string>

ViewportPanel::ViewportPanel(const std::shared_ptr<TabManager>& tabManager)
	: m_tabManager(tabManager)
{

}

void ViewportPanel::Render()
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::Begin("##BrowserViewport", nullptr, viewportFlags);

	ImGuiID dockspace_id = ImGui::GetID("MainViewport");
	ImGui::DockSpace(dockspace_id);

	for (const std::shared_ptr<Tab>& tab : m_tabManager->Tabs())
	{
		ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Once);

		RenderTabComplete(tab);
	}

	ImGui::End();
}

void ViewportPanel::RenderTabComplete(const std::shared_ptr<Tab>& tab)
{
	int activeTabId = m_tabManager->GetActiveTabId();

	if (tab->GetId() == activeTabId && activeTabId != m_lastFocusedTabId)
	{
		m_lastFocusedTabId = activeTabId;
	}

	bool isOpen = true;

	bool contentVisible = ImGui::Begin(("Test Window " + std::to_string(tab->GetId())).c_str(), &isOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);

	if (ImGui::IsWindowFocused() && tab->GetId() != activeTabId)
	{
		m_tabManager->SetActiveTab(tab->GetId());
		m_lastFocusedTabId = tab->GetId();
	}

	if (contentVisible)
	{
		RenderTabContent(tab);
	}

	ImGui::End();
}

void ViewportPanel::RenderTabContent(const std::shared_ptr<Tab>& tab)
{
	m_adressBarPanel.Render(tab);

	ImVec2 availableSize = ImGui::GetContentRegionAvail();
	int viewWidth = static_cast<int>(availableSize.x);
	int viewHeight = static_cast<int>(availableSize.y);

	if (viewWidth > 0 && viewHeight > 0)
	{
		tab->SetViewSize(viewWidth, viewHeight);
	}

	if (availableSize.x <= 0.0f || availableSize.y <= 0.0f)
	{
		return;
	}

	std::shared_ptr<Walnut::Image> browserImage = tab->GetBrowserImage();

	if (!browserImage)
	{
		return;
	}

	float imageWidth = static_cast<float>(browserImage->GetWidth());
	float imageHeight = static_cast<float>(browserImage->GetHeight());

	float displayWidth = availableSize.x < imageWidth ? availableSize.x : imageWidth;
	float displayHeight = availableSize.y < imageHeight ? availableSize.y : imageHeight;

	float uMax = displayWidth / imageWidth;
	float vMax = displayHeight / imageHeight;

	ImGui::Image(browserImage->GetDescriptorSet(), ImVec2(displayWidth, displayHeight), ImVec2(0.0f, 0.0f), ImVec2(uMax, vMax));
}