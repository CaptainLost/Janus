#include "Walnut/WebView.h"
#include "Walnut/FaviconDownloadCallback.h"

#include "include/cef_app.h"
#include "include/cef_browser.h"

#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#else
#include <thread>
#endif

static int GetMonitorRefreshRate()
{
#ifdef _WIN32
	DEVMODE devMode = {};
	devMode.dmSize = sizeof(devMode);
	if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode) && devMode.dmDisplayFrequency > 1)
	{
		return static_cast<int>(devMode.dmDisplayFrequency);
	}
#endif
	return 60;
}

namespace Walnut
{

	WebView::WebView(int width, int height)
		: m_ViewWidth(width), m_ViewHeight(height)
	{
	}

	void WebView::Create(const std::string& startURL)
	{
		CefWindowInfo windowInfo;
		windowInfo.SetAsWindowless(nullptr);

		CefBrowserSettings settings;
		settings.windowless_frame_rate = GetMonitorRefreshRate();

		CefBrowserHost::CreateBrowser(windowInfo, this, startURL, settings, nullptr, nullptr);
	}

	void WebView::Close()
	{
		if (!m_Browser)
		{
			return;
		}

		m_Browser->GetHost()->CloseBrowser(true);

		constexpr int kMaxIterations = 200; // ~2 s safety limit
		int remaining = kMaxIterations;
		while (m_Browser && remaining-- > 0)
		{
			CefDoMessageLoopWork();
			Sleep(10);
		}
	}

	void WebView::Navigate(const std::string& url)
	{
		if (!m_Browser)
		{
			return;
		}

		std::string finalURL = url;
		if (finalURL.find("://") == std::string::npos)
		{
			finalURL = "https://" + finalURL;
		}

		m_Browser->GetMainFrame()->LoadURL(finalURL);
	}

	void WebView::GoBack()
	{
		if (m_Browser)
		{
			m_Browser->GoBack();
		}
	}

	void WebView::GoForward()
	{
		if (m_Browser)
		{
			m_Browser->GoForward();
		}
	}

	void WebView::Reload()
	{
		if (m_Browser)
		{
			m_Browser->Reload();
		}
	}

	void WebView::StopLoading()
	{
		if (m_Browser)
		{
			m_Browser->StopLoad();
		}
	}

	void WebView::SetViewSize(int width, int height)
	{
		if (width == m_ViewWidth && height == m_ViewHeight)
		{
			return;
		}

		m_ViewWidth = width;
		m_ViewHeight = height;

		if (m_Browser)
		{
			m_Browser->GetHost()->WasResized();
		}
	}

	WebViewState WebView::GetState() const
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		return { m_CurrentURL, m_Title, m_IsLoading, m_CanGoBack, m_CanGoForward };
	}

	bool WebView::GetPixelBuffer(std::vector<uint8_t>& outBuffer, int& outWidth, int& outHeight)
	{
		std::lock_guard<std::mutex> lock(m_BufferMutex);
		if (!m_BufferDirty)
		{
			return false;
		}

		outBuffer = m_PixelBuffer;
		outWidth = m_BufferWidth;
		outHeight = m_BufferHeight;
		m_BufferDirty = false;
		return true;
	}

	bool WebView::GetFaviconPixels(std::vector<uint8_t>& outBuffer, int& outWidth, int& outHeight)
	{
		std::lock_guard<std::mutex> lock(m_FaviconMutex);
		if (!m_FaviconDirty)
		{
			return false;
		}

		outBuffer = m_FaviconPixels;
		outWidth = m_FaviconWidth;
		outHeight = m_FaviconHeight;
		m_FaviconDirty = false;
		return true;
	}

	void WebView::SetFaviconData(std::vector<uint8_t> pixels, int width, int height)
	{
		std::lock_guard<std::mutex> lock(m_FaviconMutex);
		m_FaviconPixels = std::move(pixels);
		m_FaviconWidth = width;
		m_FaviconHeight = height;
		m_FaviconDirty = true;
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
		{
			return;
		}

		std::lock_guard<std::mutex> lock(m_BufferMutex);

		const size_t bufferSize = static_cast<size_t>(width) * height * 4;
		m_PixelBuffer.resize(bufferSize);
		memcpy(m_PixelBuffer.data(), buffer, bufferSize);

		m_BufferWidth = width;
		m_BufferHeight = height;
		m_BufferDirty = true;
	}

	void WebView::OnAfterCreated(CefRefPtr<CefBrowser> browser)
	{
		m_Browser = browser;
	}

	void WebView::OnBeforeClose(CefRefPtr<CefBrowser> /*browser*/)
	{
		m_Browser = nullptr;
	}

	void WebView::OnLoadingStateChange(CefRefPtr<CefBrowser> /*browser*/,
	                                   bool isLoading, bool canGoBack, bool canGoForward)
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		m_IsLoading = isLoading;
		m_CanGoBack = canGoBack;
		m_CanGoForward = canGoForward;
	}

	void WebView::OnAddressChange(CefRefPtr<CefBrowser> /*browser*/,
	                              CefRefPtr<CefFrame> frame,
	                              const CefString& url)
	{
		if (!frame->IsMain())
		{
			return;
		}

		std::string urlStr;
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			m_CurrentURL = url.ToString();
			urlStr = m_CurrentURL;
		}

		if (m_onAddressChange)
		{
			m_onAddressChange(urlStr);
		}
	}

	void WebView::OnTitleChange(CefRefPtr<CefBrowser> /*browser*/,
	                            const CefString& title)
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		m_Title = title.ToString();
	}

	bool WebView::OnBeforeBrowse(CefRefPtr<CefBrowser> /*browser*/,
	                             CefRefPtr<CefFrame> frame,
	                             CefRefPtr<CefRequest> request,
	                             bool /*userGesture*/,
	                             bool /*isRedirect*/)
	{
		if (!frame->IsMain())
		{
			return false;
		}

		if (m_onBeforeBrowse)
		{
			return m_onBeforeBrowse(request->GetURL().ToString());
		}

		return false;
	}

	void WebView::OnBeforeContextMenu(CefRefPtr<CefBrowser> /*browser*/,
	                                  CefRefPtr<CefFrame> /*frame*/,
	                                  CefRefPtr<CefContextMenuParams> params,
	                                  CefRefPtr<CefMenuModel> model)
	{
		model->Clear();

		ContextMenuRequest request;
		request.x = params->GetXCoord();
		request.y = params->GetYCoord();
		request.pageUrl = params->GetPageUrl().ToString();
		request.linkUrl = params->GetLinkUrl().ToString();
		request.sourceUrl = params->GetSourceUrl().ToString();
		request.selectionText = params->GetSelectionText().ToString();

		int flags = params->GetTypeFlags();
		request.hasLink = (flags & CM_TYPEFLAG_LINK) != 0;
		request.hasImage = (flags & CM_TYPEFLAG_MEDIA) != 0
		                && params->GetMediaType() == CM_MEDIATYPE_IMAGE;
		request.hasSelection = (flags & CM_TYPEFLAG_SELECTION) != 0;
		request.isEditable = (flags & CM_TYPEFLAG_EDITABLE) != 0;
		request.canGoBack = m_CanGoBack;
		request.canGoForward = m_CanGoForward;

		std::lock_guard<std::mutex> lock(m_contextMenuMutex);
		m_contextMenuRequest = std::move(request);
		m_contextMenuDirty = true;
	}

	bool WebView::RunContextMenu(CefRefPtr<CefBrowser> /*browser*/,
	                             CefRefPtr<CefFrame> /*frame*/,
	                             CefRefPtr<CefContextMenuParams> /*params*/,
	                             CefRefPtr<CefMenuModel> /*model*/,
	                             CefRefPtr<CefRunContextMenuCallback> /*callback*/)
	{
		return true;
	}

	bool WebView::GetContextMenuRequest(ContextMenuRequest& out)
	{
		std::lock_guard<std::mutex> lock(m_contextMenuMutex);
		if (!m_contextMenuDirty)
		{
			return false;
		}

		out = m_contextMenuRequest;
		m_contextMenuDirty = false;
		return true;
	}

	void WebView::SetPendingDownloadPath(const std::string& path)
	{
		std::lock_guard<std::mutex> lock(m_downloadMutex);
		m_pendingDownloadPath = path;
	}

	bool WebView::OnBeforeDownload(CefRefPtr<CefBrowser> /*browser*/,
	                               CefRefPtr<CefDownloadItem> /*downloadItem*/,
	                               const CefString& /*suggestedName*/,
	                               CefRefPtr<CefBeforeDownloadCallback> callback)
	{
		std::string path;
		{
			std::lock_guard<std::mutex> lock(m_downloadMutex);
			path = std::move(m_pendingDownloadPath);
		}

		if (!path.empty())
		{
			callback->Continue(path, false);
		}

		return true;
	}

	void WebView::OnDownloadUpdated(CefRefPtr<CefBrowser> /*browser*/,
	                                CefRefPtr<CefDownloadItem> /*downloadItem*/,
	                                CefRefPtr<CefDownloadItemCallback> /*callback*/)
	{
	}

	void WebView::OnFaviconURLChange(CefRefPtr<CefBrowser> /*browser*/,
	                                 const std::vector<CefString>& iconURLs)
	{
		if (iconURLs.empty() || !m_Browser)
		{
			return;
		}

		m_Browser->GetHost()->DownloadImage(
			iconURLs[0], true, 0, false,
			new FaviconDownloadCallback(this));
	}
}
