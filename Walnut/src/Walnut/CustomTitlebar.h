#pragma once

#include <functional>

struct GLFWwindow;

namespace Walnut {

	class CustomTitlebar
	{
	public:
		CustomTitlebar(GLFWwindow* windowHandle);
		~CustomTitlebar() = default;

		void Render();

		float GetHeight() const { return m_TitlebarHeight; }

		void SetLeftCallback(const std::function<void()>& cb) { m_LeftCallback = cb; }
		void SetRightCallback(const std::function<void()>& cb) { m_RightCallback = cb; }

		bool IsInDragArea(int screenX, int screenY) const;

	private:
		GLFWwindow* m_WindowHandle = nullptr;

		float m_TitlebarHeight = 0.0f;
		float m_LeftContentEndX = 0.0f;
		float m_RightContentStartX = 0.0f;
		float m_PrevRightCallbackWidth = 0.0f;

		std::function<void()> m_LeftCallback;
		std::function<void()> m_RightCallback;
	};

}
