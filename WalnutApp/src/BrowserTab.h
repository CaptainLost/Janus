#pragma once

#include "Walnut/Image.h"
#include "Walnut/WebView.h"

#include "include/cef_app.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

enum class TabType
{
	Dynamic,    // Top bar – vanishes when the app is closed
	Permanent,  // Left sidebar – never removed automatically
	Temporary   // Left sidebar – auto-removed after a deadline
};

/// A single browser tab with its own WebView, pixel buffer, and metadata.
struct BrowserTab
{
	// Identity
	int Id = 0;
	TabType Type = TabType::Dynamic;
	std::string StartURL  = "https://www.google.com";

	// URL bar state
	char        URLBuffer[2048] = {};
	bool        URLBarFocused   = false;

	// View
	CefRefPtr<Walnut::WebView>     WebView;
	std::shared_ptr<Walnut::Image> BrowserImage;
	int  ViewWidth  = 1280;
	int  ViewHeight = 720;

	// For temporary tabs: the point in time when this tab should be removed.
	// Ignored for Dynamic / Permanent tabs.
	std::chrono::steady_clock::time_point Deadline{};

	// Whether the tab is currently "open" (has an active WebView).
	// Permanent / Temporary tabs can be listed but not necessarily open.
	bool IsOpen = false;

	// ---- helpers -----------------------------------------------------------

	void Create()
	{
		if (WebView)
			return; // already created
		WebView = new Walnut::WebView(ViewWidth, ViewHeight);
		WebView->Create(StartURL);
		std::strncpy(URLBuffer, StartURL.c_str(), sizeof(URLBuffer) - 1);
		URLBuffer[sizeof(URLBuffer) - 1] = '\0';
		IsOpen = true;
	}

	void Close()
	{
		BrowserImage.reset();
		if (WebView)
		{
			WebView->Close();
			WebView = nullptr;
		}
		IsOpen = false;
	}

	Walnut::WebViewState GetState() const
	{
		if (WebView)
			return WebView->GetState();

		return {};
	}

	/// Returns remaining time in seconds for temporary tabs, or -1.
	double GetRemainingSeconds() const
	{
		if (Type != TabType::Temporary)
			return -1.0;
		auto now = std::chrono::steady_clock::now();
		auto remaining = std::chrono::duration<double>(Deadline - now);
		return remaining.count();
	}

	/// Human-readable remaining-time string (e.g. "23h 14m").
	std::string GetRemainingTimeString() const
	{
		double secs = GetRemainingSeconds();
		if (secs <= 0.0)
			return "expired";

		int totalSec = static_cast<int>(secs);
		int h = totalSec / 3600;
		int m = (totalSec % 3600) / 60;
		int s = totalSec % 60;

		if (h > 0)
			return std::to_string(h) + "h " + std::to_string(m) + "m";
		if (m > 0)
			return std::to_string(m) + "m " + std::to_string(s) + "s";
		return std::to_string(s) + "s";
	}
};
