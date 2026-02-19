#pragma once

#include "Layer.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

struct GLFWwindow;
struct VkInstance_T;
struct VkPhysicalDevice_T;
struct VkDevice_T;
struct VkCommandBuffer_T;
typedef VkInstance_T* VkInstance;
typedef VkPhysicalDevice_T* VkPhysicalDevice;
typedef VkDevice_T* VkDevice;
typedef VkCommandBuffer_T* VkCommandBuffer;

#ifdef WL_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace Walnut {

	class CustomTitlebar;

	struct ApplicationSpecification
	{
		std::string Name = "Walnut App";
		uint32_t Width = 1600;
		uint32_t Height = 900;
		bool CustomTitlebar = false;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
		~Application();

		static Application& Get();

		void Run();
		void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }

		void SetTitlebarLeftCallback(const std::function<void()>& cb);
		void SetTitlebarRightCallback(const std::function<void()>& cb);

		template<typename T>
		void PushLayer()
		{
			static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
			m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
		}

		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

		void Close();

		float GetTime();
		GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }

		static VkInstance GetInstance();
		static VkPhysicalDevice GetPhysicalDevice();
		static VkDevice GetDevice();

		static VkCommandBuffer GetCommandBuffer(bool begin);
		static void FlushCommandBuffer(VkCommandBuffer commandBuffer);

		static void SubmitResourceFree(std::function<void()>&& func);

		float GetTitlebarHeight() const;
	private:
		void Init();
		void Shutdown();

#ifdef WL_PLATFORM_WINDOWS
		static LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		WNDPROC m_OriginalWndProc = nullptr;
#endif

	private:
		ApplicationSpecification m_Specification;
		GLFWwindow* m_WindowHandle = nullptr;
		bool m_Running = false;

		float m_TimeStep = 0.0f;
		float m_FrameTime = 0.0f;
		float m_LastFrameTime = 0.0f;

		std::vector<std::shared_ptr<Layer>> m_LayerStack;
		std::function<void()> m_MenubarCallback;

		std::unique_ptr<Walnut::CustomTitlebar> m_CustomTitlebar;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}
