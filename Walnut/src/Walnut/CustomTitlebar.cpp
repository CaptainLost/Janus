#include "CustomTitlebar.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <IconsFontAwesome6.h>

namespace Walnut {

	CustomTitlebar::CustomTitlebar(GLFWwindow* windowHandle, const std::string& title, ImFont* titleFont)
		: m_WindowHandle(windowHandle), m_WindowTitle(title), m_TitleFont(titleFont)
	{
	}

	void CustomTitlebar::Render(const std::function<void()>& menubarCallback)
	{
		const float titlebarHeight = m_TitlebarHeight;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Draw a fixed window at the top of the screen for the titlebar
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, titlebarHeight));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

		ImGuiWindowFlags titlebarFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus;

		ImGui::Begin("##CustomTitlebar", nullptr, titlebarFlags);

		const float buttonWidth = 46.0f;
		const float windowWidth = viewport->Size.x;
		const float contentY = (titlebarHeight - ImGui::GetFrameHeight()) * 0.5f;

		// ---- Icon (left) ----
		ImGui::SetCursorPos(ImVec2(8.0f, contentY));
		ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "J");

		// ---- Menu items rendered inline right after the icon ----
		if (menubarCallback)
		{
			ImGui::SameLine();
			ImGui::SetCursorPosY(contentY);
			menubarCallback();
		}

		// ---- Window title in center ----
		{
			if (m_TitleFont)
				ImGui::PushFont(m_TitleFont);

			const char* title = m_WindowTitle.c_str();
			const float titleTextWidth = ImGui::CalcTextSize(title).x;
			const float titleX = (windowWidth - titleTextWidth) * 0.5f;
			ImGui::SetCursorPos(ImVec2(titleX, (titlebarHeight - ImGui::GetFontSize()) * 0.5f));
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", title);

			if (m_TitleFont)
				ImGui::PopFont();
		}

		// ---- Window control buttons (right side) ----
		const float controlsStartX = windowWidth - buttonWidth * 3.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		// Minimize button
		ImGui::SetCursorPos(ImVec2(controlsStartX, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
		if (ImGui::Button(ICON_FA_WINDOW_MINIMIZE "##min", ImVec2(buttonWidth, titlebarHeight)))
		{
			glfwIconifyWindow(m_WindowHandle);
		}
		ImGui::PopStyleColor(3);

		// Maximize/Restore button
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
		const bool isMaximized = glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED) != 0;
		if (ImGui::Button(isMaximized ? ICON_FA_WINDOW_RESTORE " ##max" : ICON_FA_WINDOW_MAXIMIZE " ##max", ImVec2(buttonWidth, titlebarHeight)))
		{
			if (isMaximized)
				glfwRestoreWindow(m_WindowHandle);
			else
				glfwMaximizeWindow(m_WindowHandle);
		}
		ImGui::PopStyleColor(3);

		// Close button (red on hover)
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.86f, 0.2f, 0.2f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
		if (ImGui::Button("  X  ##close", ImVec2(buttonWidth, titlebarHeight)))
		{
			glfwSetWindowShouldClose(m_WindowHandle, GLFW_TRUE);
		}
		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar(2); // FrameRounding, ItemSpacing

		// Handle dragging logic
		HandleDragging(viewport->Pos, viewport->Size, controlsStartX);

		ImGui::End();

		ImGui::PopStyleColor(); // WindowBg
		ImGui::PopStyleVar(3);  // WindowRounding, WindowBorderSize, WindowPadding
	}

	void CustomTitlebar::HandleDragging(const ImVec2& viewportPos, const ImVec2& viewportSize, float controlsStartX)
	{
		const ImGuiIO& io = ImGui::GetIO();
		const ImVec2 mousePos = io.MousePos;
		const float absTop = viewportPos.y;
		const float absBottom = viewportPos.y + m_TitlebarHeight;
		const float absLeft = viewportPos.x;
		const float absControlsStart = viewportPos.x + controlsStartX;

		const bool mouseInTitlebar =
			mousePos.x >= absLeft && mousePos.x < absControlsStart &&
			mousePos.y >= absTop && mousePos.y < absBottom;

		// Don't start dragging if:
		// - Hovering over any ImGui widget (menus, buttons, etc.)
		// - Any popup menu is currently open (File, View, etc.)
		const bool overImGuiItem = ImGui::IsAnyItemHovered();
		const bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

		if (mouseInTitlebar && !overImGuiItem && !popupOpen && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			m_TitlebarDragging = true;

			// Store the screen-absolute mouse position as drag anchor
			int winX, winY;
			double curX, curY;
			glfwGetWindowPos(m_WindowHandle, &winX, &winY);
			glfwGetCursorPos(m_WindowHandle, &curX, &curY);
			m_DragStartX = winX + curX;  // absolute screen X of mouse
			m_DragStartY = winY + curY;  // absolute screen Y of mouse
			m_DragWinStartX = winX;
			m_DragWinStartY = winY;

			// If maximized, restore before dragging (snap-to-cursor behavior)
			const bool isMaximized = glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED) != 0;
			if (isMaximized)
			{
				glfwRestoreWindow(m_WindowHandle);
				int restoredW, restoredH;
				glfwGetWindowSize(m_WindowHandle, &restoredW, &restoredH);

				// Re-read to get position after restore
				glfwGetWindowPos(m_WindowHandle, &winX, &winY);
				glfwGetCursorPos(m_WindowHandle, &curX, &curY);
				double absMouseX = winX + curX;
				double absMouseY = winY + curY;

				// Place window so cursor is roughly centered horizontally
				int newX = (int)(absMouseX - restoredW * 0.5);
				int newY = (int)(absMouseY - m_TitlebarHeight * 0.5);
				glfwSetWindowPos(m_WindowHandle, newX, newY);

				// Re-anchor after reposition
				m_DragStartX = absMouseX;
				m_DragStartY = absMouseY;
				m_DragWinStartX = newX;
				m_DragWinStartY = newY;
			}
		}

		if (m_TitlebarDragging)
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				// Compute current screen-absolute mouse position
				int winX, winY;
				double curX, curY;
				glfwGetWindowPos(m_WindowHandle, &winX, &winY);
				glfwGetCursorPos(m_WindowHandle, &curX, &curY);
				double absMouseX = winX + curX;
				double absMouseY = winY + curY;

				double deltaX = absMouseX - m_DragStartX;
				double deltaY = absMouseY - m_DragStartY;
				glfwSetWindowPos(m_WindowHandle,
					m_DragWinStartX + (int)deltaX,
					m_DragWinStartY + (int)deltaY);
			}
			else
			{
				m_TitlebarDragging = false;
			}
		}

		// Double-click to maximize/restore
		if (mouseInTitlebar && !overImGuiItem && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED))
				glfwRestoreWindow(m_WindowHandle);
			else
				glfwMaximizeWindow(m_WindowHandle);
			m_TitlebarDragging = false;
		}
	}

}
