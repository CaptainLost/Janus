#pragma once

#ifdef WL_PLATFORM_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <filesystem>

#include "include/cef_app.h"
#include "include/cef_browser.h"

extern Walnut::Application* Walnut::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

// Minimal CefApp for browser process
class WalnutCefApp : public CefApp, public CefBrowserProcessHandler
{
public:
	CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
	IMPLEMENT_REFCOUNTING(WalnutCefApp);
};

namespace Walnut {

	int Main(int argc, char** argv)
	{
		// CEF sub-process check — must be the very first thing.
		CefMainArgs cefArgs(GetModuleHandle(nullptr));
		int exitCode = CefExecuteProcess(cefArgs, nullptr, nullptr);
		if (exitCode >= 0)
		{
			return exitCode;
		}

		// Build absolute paths required by CEF.
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

		std::filesystem::path cachePath = exeDir / "cef_cache";
		std::filesystem::create_directories(cachePath);

		// Initialize CEF for the browser (main) process.
		CefSettings cefSettings;
		cefSettings.no_sandbox = true;
		cefSettings.multi_threaded_message_loop = false;
		cefSettings.windowless_rendering_enabled = true;

		// root_cache_path MUST be an absolute path
		CefString(&cefSettings.root_cache_path).FromWString(cachePath.wstring());

		// Point CEF to the locales and resources next to the exe
		CefString(&cefSettings.locales_dir_path).FromWString((exeDir / "locales").wstring());
		CefString(&cefSettings.resources_dir_path).FromWString(exeDir.wstring());

		CefRefPtr<WalnutCefApp> cefApp(new WalnutCefApp());
		if (!CefInitialize(cefArgs, cefSettings, cefApp, nullptr))
		{
			// CefInitialize failed — fall through without CEF
			return 1;
		}

		// Normal Walnut application loop
		while (g_ApplicationRunning)
		{
			Walnut::Application* app = Walnut::CreateApplication(argc, argv);
			app->Run();
			delete app;
		}

		// Shut down CEF after the application loop ends
		CefShutdown();

		return 0;
	}

}

#ifdef WL_DIST

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return Walnut::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return Walnut::Main(argc, argv);
}

#endif // WL_DIST

#endif // WL_PLATFORM_WINDOWS
