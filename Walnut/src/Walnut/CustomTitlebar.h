#pragma once

#include <functional>
#include <string>

struct GLFWwindow;
struct ImVec2;
struct ImFont;

namespace Walnut {

	class CustomTitlebar
	{
	public:
		CustomTitlebar(GLFWwindow* windowHandle, const std::string& title, ImFont* titleFont = nullptr);
		~CustomTitlebar() = default;

		// Render the custom titlebar and handle all interaction logic
		void Render(const std::function<void()>& menubarCallback = nullptr);

		// Get the height of the titlebar
		float GetHeight() const { return m_TitlebarHeight; }

		// Update the window title
		void SetTitle(const std::string& title) { m_WindowTitle = title; }

	private:
		void HandleDragging(const ImVec2& viewportPos, const ImVec2& viewportSize, float controlsStartX);

		GLFWwindow* m_WindowHandle = nullptr;
		std::string m_WindowTitle;
		ImFont* m_TitleFont = nullptr;

		// Titlebar dimensions and state
		float m_TitlebarHeight = 40.0f;
		bool m_TitlebarDragging = false;
		double m_DragStartX = 0.0;
		double m_DragStartY = 0.0;
		int m_DragWinStartX = 0;
		int m_DragWinStartY = 0;
	};

}
