#include "Walnut/WebView.h"

#include "include/cef_app.h"
#include "include/cef_browser.h"

#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#else
#include <thread>
#endif

namespace Walnut {

	// -------------------------------------------------------------------------
	// Construction
	// -------------------------------------------------------------------------

	WebView::WebView(int width, int height)
		: m_ViewWidth(width), m_ViewHeight(height)
	{
	}

	void WebView::Create(const std::string& startURL)
	{
		CefWindowInfo windowInfo;
		windowInfo.SetAsWindowless(nullptr);

		CefBrowserSettings settings;
		settings.windowless_frame_rate = 60;

		CefBrowserHost::CreateBrowser(windowInfo, this, startURL, settings, nullptr, nullptr);
	}

	void WebView::Close()
	{
		if (!m_Browser)
			return;

		m_Browser->GetHost()->CloseBrowser(true);

		// Pump CEF messages until the browser has been fully destroyed.
		constexpr int kMaxIterations = 200; // ~2 s safety limit
		int remaining = kMaxIterations;
		while (m_Browser && remaining-- > 0)
		{
			CefDoMessageLoopWork();
			Sleep(10);
		}
	}

	// -------------------------------------------------------------------------
	// Navigation
	// -------------------------------------------------------------------------

	void WebView::Navigate(const std::string& url)
	{
		if (!m_Browser)
			return;

		std::string finalURL = url;
		if (finalURL.find("://") == std::string::npos)
			finalURL = "https://" + finalURL;

		m_Browser->GetMainFrame()->LoadURL(finalURL);
	}

	void WebView::GoBack()
	{
		if (m_Browser) m_Browser->GoBack();
	}

	void WebView::GoForward()
	{
		if (m_Browser) m_Browser->GoForward();
	}

	void WebView::Reload()
	{
		if (m_Browser) m_Browser->Reload();
	}

	void WebView::StopLoading()
	{
		if (m_Browser) m_Browser->StopLoad();
	}

	// -------------------------------------------------------------------------
	// View management
	// -------------------------------------------------------------------------

	void WebView::SetViewSize(int width, int height)
	{
		if (width == m_ViewWidth && height == m_ViewHeight)
			return;

		m_ViewWidth  = width;
		m_ViewHeight = height;

		if (m_Browser)
			m_Browser->GetHost()->WasResized();
	}

	// -------------------------------------------------------------------------
	// State queries
	// -------------------------------------------------------------------------

	WebViewState WebView::GetState() const
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		return { m_CurrentURL, m_Title, m_IsLoading, m_CanGoBack, m_CanGoForward };
	}

	bool WebView::GetPixelBuffer(std::vector<uint8_t>& outBuffer, int& outWidth, int& outHeight)
	{
		std::lock_guard<std::mutex> lock(m_BufferMutex);
		if (!m_BufferDirty)
			return false;

		outBuffer = m_PixelBuffer;
		outWidth  = m_BufferWidth;
		outHeight = m_BufferHeight;
		m_BufferDirty = false;
		return true;
	}

	CefRefPtr<CefBrowser> WebView::GetBrowser() const
	{
		return m_Browser;
	}

	// -------------------------------------------------------------------------
	// CefRenderHandler
	// -------------------------------------------------------------------------

	void WebView::GetViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect)
	{
		rect = CefRect(0, 0, m_ViewWidth, m_ViewHeight);
	}

	void WebView::OnPaint(CefRefPtr<CefBrowser> /*browser*/,
	                      PaintElementType type,
	                      const RectList& /*dirtyRects*/,
	                      const void* buffer,
	                      int width, int height)
	{
		if (type != PET_VIEW)
			return;

		std::lock_guard<std::mutex> lock(m_BufferMutex);

		const size_t bufferSize = static_cast<size_t>(width) * height * 4;
		m_PixelBuffer.resize(bufferSize);

		// CEF delivers BGRA — convert to RGBA for Vulkan / ImGui.
		const uint8_t* src = static_cast<const uint8_t*>(buffer);
		for (size_t i = 0; i < bufferSize; i += 4)
		{
			m_PixelBuffer[i + 0] = src[i + 2]; // R ? B
			m_PixelBuffer[i + 1] = src[i + 1]; // G
			m_PixelBuffer[i + 2] = src[i + 0]; // B ? R
			m_PixelBuffer[i + 3] = src[i + 3]; // A
		}

		m_BufferWidth  = width;
		m_BufferHeight = height;
		m_BufferDirty  = true;
	}

	// -------------------------------------------------------------------------
	// CefLifeSpanHandler
	// -------------------------------------------------------------------------

	void WebView::OnAfterCreated(CefRefPtr<CefBrowser> browser)
	{
		m_Browser = browser;
	}

	void WebView::OnBeforeClose(CefRefPtr<CefBrowser> /*browser*/)
	{
		m_Browser = nullptr;
	}

	// -------------------------------------------------------------------------
	// CefLoadHandler
	// -------------------------------------------------------------------------

	void WebView::OnLoadingStateChange(CefRefPtr<CefBrowser> /*browser*/,
	                                   bool isLoading, bool canGoBack, bool canGoForward)
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		m_IsLoading   = isLoading;
		m_CanGoBack   = canGoBack;
		m_CanGoForward = canGoForward;
	}

	// -------------------------------------------------------------------------
	// CefDisplayHandler
	// -------------------------------------------------------------------------

	void WebView::OnAddressChange(CefRefPtr<CefBrowser> /*browser*/,
	                              CefRefPtr<CefFrame> frame,
	                              const CefString& url)
	{
		if (frame->IsMain())
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			m_CurrentURL = url.ToString();
		}
	}

	void WebView::OnTitleChange(CefRefPtr<CefBrowser> /*browser*/,
	                            const CefString& title)
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		m_Title = title.ToString();
	}

} // namespace Walnut
