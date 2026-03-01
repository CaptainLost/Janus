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
	// ImGui::Text("Hello from window %d!", tab->GetId());

	m_adressBarPanel.Render(tab);
	// Test
	//std::string& url = tab->GetUrlInput();
	//url.reserve(2048);

	//bool enterPressed = ImGui::InputTextWithHint(
	//	"##nav_url",
	//	"Enter web adress...",
	//	url.data(),
	//	url.capacity() + 1,
	//	ImGuiInputTextFlags_EnterReturnsTrue,
	//	nullptr,
	//	&url);

	//if (enterPressed)
	//{
	//	tab->Open(url);
	//}
	// Test end

	std::shared_ptr<Walnut::Image> browserImage = tab->GetBrowserImage();

	if (!browserImage)
	{
		return;
	}

	ImGui::Image(browserImage->GetDescriptorSet(),
		ImVec2(static_cast<float>(browserImage->GetWidth()),
			static_cast<float>(browserImage->GetHeight())));
}