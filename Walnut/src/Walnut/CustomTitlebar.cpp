#include "CustomTitlebar.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <IconsFontAwesome6.h>

namespace Walnut {

	CustomTitlebar::CustomTitlebar(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	void CustomTitlebar::Render()
	{
		const float frameHeight = ImGui::GetFrameHeight();
		const float verticalPadding = 3.0f;
		m_TitlebarHeight = frameHeight + verticalPadding * 2.0f;
		const float buttonSize = m_TitlebarHeight; // square buttons

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const float windowWidth = viewport->Size.x;

		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(ImVec2(windowWidth, m_TitlebarHeight));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus;

		ImGui::Begin("##CustomTitlebar", nullptr, flags);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		// ---- Logo ----
		const float logoY = (m_TitlebarHeight - ImGui::GetFontSize()) * 0.5f;
		ImGui::SetCursorPos(ImVec2(8.0f, logoY));
		ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "J");

		// ---- Left callback ----
		if (m_LeftCallback)
		{
			ImGui::SameLine(0.0f, 6.0f);
			ImGui::SetCursorPosY(0.0f);
			m_LeftCallback();
		}
		ImGui::SameLine(0.0f, 0.0f);
		m_LeftContentEndX = ImGui::GetCursorPosX();

		// ---- Right callback ----
		const float windowControlsWidth = buttonSize * 3.0f;
		const float rightSectionX = windowWidth - windowControlsWidth - m_PrevRightCallbackWidth;
		m_RightContentStartX = rightSectionX;

		if (m_RightCallback)
		{
			ImGui::SetCursorPos(ImVec2(rightSectionX, 0.0f));
			float beforeX = ImGui::GetCursorPosX();
			m_RightCallback();
			ImGui::SameLine(0.0f, 0.0f);
			m_PrevRightCallbackWidth = ImGui::GetCursorPosX() - beforeX;
		}
		else
		{
			m_PrevRightCallbackWidth = 0.0f;
		}

		// ---- Window controls (explicit positions, no SameLine) ----
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

		// Minimize
		ImGui::SetCursorPos(ImVec2(windowWidth - buttonSize * 3.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
		if (ImGui::Button(ICON_FA_WINDOW_MINIMIZE "##min", ImVec2(buttonSize, buttonSize)))
			glfwIconifyWindow(m_WindowHandle);
		ImGui::PopStyleColor(3);

		// Maximize/Restore
		ImGui::SetCursorPos(ImVec2(windowWidth - buttonSize * 2.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
		const bool isMaximized = glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED) != 0;
		if (ImGui::Button(isMaximized ? ICON_FA_WINDOW_RESTORE " ##max" : ICON_FA_WINDOW_MAXIMIZE " ##max", ImVec2(buttonSize, buttonSize)))
		{
			if (isMaximized)
				glfwRestoreWindow(m_WindowHandle);
			else
				glfwMaximizeWindow(m_WindowHandle);
		}
		ImGui::PopStyleColor(3);

		// Close
		ImGui::SetCursorPos(ImVec2(windowWidth - buttonSize * 1.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.86f, 0.2f, 0.2f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
		if (ImGui::Button(ICON_FA_XMARK "##close", ImVec2(buttonSize, buttonSize)))
			glfwSetWindowShouldClose(m_WindowHandle, GLFW_TRUE);
		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar(); // FrameRounding

		ImGui::PopStyleVar(); // ItemSpacing (global for titlebar)
		ImGui::End();
		ImGui::PopStyleColor(); // WindowBg
		ImGui::PopStyleVar(3);  // WindowRounding, WindowBorderSize, WindowPadding
	}

	bool CustomTitlebar::IsInDragArea(int screenX, int screenY) const
	{
		int windowX, windowY;
		glfwGetWindowPos(m_WindowHandle, &windowX, &windowY);

		float localX = (float)(screenX - windowX);
		float localY = (float)(screenY - windowY);

		if (localY < 0.0f || localY >= m_TitlebarHeight)
			return false;

		return localX > m_LeftContentEndX && localX < m_RightContentStartX;
	}

}
