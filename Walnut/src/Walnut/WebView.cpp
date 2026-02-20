#include "Walnut/WebView.h"

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_image.h"

#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#else
#include <thread>
#endif

namespace {

class FaviconDownloadCallback : public CefDownloadImageCallback
{
public:
	explicit FaviconDownloadCallback(Walnut::WebView* webView) : m_webView(webView) {}

	void OnDownloadImageFinished(const CefString& /*imageUrl*/, int httpStatusCode, CefRefPtr<CefImage> image) override
	{
		if (!image || httpStatusCode != 200)
			return;

		int width = 0, height = 0;
		CefRefPtr<CefBinaryValue> pixels = image->GetAsBitmap(
			1.0f, CEF_COLOR_TYPE_RGBA_8888, CEF_ALPHA_TYPE_POSTMULTIPLIED, width, height);

		if (!pixels || width <= 0 || height <= 0)
			return;

		size_t dataSize = pixels->GetSize();
		std::vector<uint8_t> data(dataSize);
		pixels->GetData(data.data(), dataSize, 0);

		m_webView->SetFaviconData(std::move(data), width, height);
	}

private:
	Walnut::WebView* m_webView;
	IMPLEMENT_REFCOUNTING(FaviconDownloadCallback);
};

} // anonymous namespace

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

	bool WebView::GetFaviconPixels(std::vector<uint8_t>& outBuffer, int& outWidth, int& outHeight)
	{
		std::lock_guard<std::mutex> lock(m_FaviconMutex);
		if (!m_FaviconDirty)
			return false;

		outBuffer = m_FaviconPixels;
		outWidth  = m_FaviconWidth;
		outHeight = m_FaviconHeight;
		m_FaviconDirty = false;
		return true;
	}

	void WebView::SetFaviconData(std::vector<uint8_t> pixels, int width, int height)
	{
		std::lock_guard<std::mutex> lock(m_FaviconMutex);
		m_FaviconPixels = std::move(pixels);
		m_FaviconWidth  = width;
		m_FaviconHeight = height;
		m_FaviconDirty  = true;
	}

	void WebView::SetAddressChangeCallback(std::function<void(const std::string&)> callback)
	{
		m_onAddressChange = std::move(callback);
	}

	void WebView::SetBeforeBrowseCallback(std::function<bool(const std::string&)> callback)
	{
		m_onBeforeBrowse = std::move(callback);
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

		// CEF delivers BGRA � convert to RGBA for Vulkan / ImGui.
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
		if (!frame->IsMain())
			return;

		std::string urlStr;
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			m_CurrentURL = url.ToString();
			urlStr = m_CurrentURL;
		}

		if (m_onAddressChange)
			m_onAddressChange(urlStr);
	}

	void WebView::OnTitleChange(CefRefPtr<CefBrowser> /*browser*/,
	                            const CefString& title)
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		m_Title = title.ToString();
	}

	// -------------------------------------------------------------------------
	// CefRequestHandler
	// -------------------------------------------------------------------------

	bool WebView::OnBeforeBrowse(CefRefPtr<CefBrowser> /*browser*/,
	                             CefRefPtr<CefFrame> frame,
	                             CefRefPtr<CefRequest> request,
	                             bool /*userGesture*/,
	                             bool /*isRedirect*/)
	{
		if (!frame->IsMain())
			return false;

		if (m_onBeforeBrowse)
			return m_onBeforeBrowse(request->GetURL().ToString());

		return false;
	}

	void WebView::OnFaviconURLChange(CefRefPtr<CefBrowser> /*browser*/,
	                                 const std::vector<CefString>& iconURLs)
	{
		if (iconURLs.empty() || !m_Browser)
			return;

		m_Browser->GetHost()->DownloadImage(
			iconURLs[0], true, 0, false,
			new FaviconDownloadCallback(this));
	}

} // namespace Walnut
