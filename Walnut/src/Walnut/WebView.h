#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_display_handler.h"
#include "include/cef_request_handler.h"
#include "include/cef_context_menu_handler.h"
#include "include/cef_download_handler.h"

namespace Walnut {

	struct WebViewState
	{
		std::string URL;
		std::string Title;
		bool IsLoading = false;
		bool CanGoBack = false;
		bool CanGoForward = false;
	};

	struct ContextMenuRequest
	{
		int x = 0;
		int y = 0;
		std::string linkUrl;
		std::string sourceUrl;
		std::string selectionText;
		std::string pageUrl;
		bool hasLink = false;
		bool hasImage = false;
		bool hasSelection = false;
		bool isEditable = false;
		bool canGoBack = false;
		bool canGoForward = false;
	};

	class WebView : public CefClient,
	                public CefRenderHandler,
	                public CefLifeSpanHandler,
	                public CefLoadHandler,
	                public CefDisplayHandler,
	                public CefRequestHandler,
	                public CefContextMenuHandler,
	                public CefDownloadHandler
	{
	public:
		WebView(int width, int height);

		void Create(const std::string& startURL);
		void Close();

		void Navigate(const std::string& url);
		void GoBack();
		void GoForward();
		void Reload();
		void StopLoading();

		void SetViewSize(int width, int height);
		int GetViewWidth() const { return m_ViewWidth; }
		int GetViewHeight() const { return m_ViewHeight; }

		WebViewState GetState() const;
		bool SwapPixelBuffer(std::vector<uint8_t>& buffer, int& outWidth, int& outHeight);
		bool GetContextMenuRequest(ContextMenuRequest& out);
		void SetPendingDownloadPath(const std::string& path);

		bool GetFaviconPixels(std::vector<uint8_t>& outBuffer, int& outWidth, int& outHeight);
		void SetFaviconData(std::vector<uint8_t> pixels, int width, int height);

		void SetAddressChangeCallback(std::function<void(const std::string&)> callback);
		void SetBeforeBrowseCallback(std::function<bool(const std::string&)> callback);

		CefRefPtr<CefBrowser> GetBrowser() const;

	private:
		CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
		CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
		CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
		CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
		CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
		CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
		CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }

		void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
		void OnPaint(CefRefPtr<CefBrowser> browser,
		             PaintElementType type,
		             const RectList& dirtyRects,
		             const void* buffer,
		             int width, int height) override;

		void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
		void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

		void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
		                          bool isLoading, bool canGoBack, bool canGoForward) override;

		void OnAddressChange(CefRefPtr<CefBrowser> browser,
		                     CefRefPtr<CefFrame> frame,
		                     const CefString& url) override;
		void OnTitleChange(CefRefPtr<CefBrowser> browser,
		                   const CefString& title) override;
		void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
		                        const std::vector<CefString>& iconURLs) override;

		bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
		                    CefRefPtr<CefFrame> frame,
		                    CefRefPtr<CefRequest> request,
		                    bool userGesture,
		                    bool isRedirect) override;

		void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
		                         CefRefPtr<CefFrame> frame,
		                         CefRefPtr<CefContextMenuParams> params,
		                         CefRefPtr<CefMenuModel> model) override;
		bool RunContextMenu(CefRefPtr<CefBrowser> browser,
		                    CefRefPtr<CefFrame> frame,
		                    CefRefPtr<CefContextMenuParams> params,
		                    CefRefPtr<CefMenuModel> model,
		                    CefRefPtr<CefRunContextMenuCallback> callback) override;

		bool OnBeforeDownload(CefRefPtr<CefBrowser> browser,
		                      CefRefPtr<CefDownloadItem> downloadItem,
		                      const CefString& suggestedName,
		                      CefRefPtr<CefBeforeDownloadCallback> callback) override;
		void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
		                       CefRefPtr<CefDownloadItem> downloadItem,
		                       CefRefPtr<CefDownloadItemCallback> callback) override;

	private:
		int m_ViewWidth;
		int m_ViewHeight;

		mutable std::mutex m_BufferMutex;
		std::vector<uint8_t> m_PixelBuffer;
		int m_BufferWidth = 0;
		int m_BufferHeight = 0;
		bool m_BufferDirty = false;

		mutable std::mutex m_StateMutex;
		std::string m_CurrentURL;
		std::string m_Title;
		bool m_IsLoading = false;
		bool m_CanGoBack = false;
		bool m_CanGoForward = false;

		mutable std::mutex m_FaviconMutex;
		std::vector<uint8_t> m_FaviconPixels;
		int m_FaviconWidth = 0;
		int m_FaviconHeight = 0;
		bool m_FaviconDirty = false;

		CefRefPtr<CefBrowser> m_Browser;

		std::function<void(const std::string&)> m_onAddressChange;
		std::function<bool(const std::string&)> m_onBeforeBrowse;

		mutable std::mutex m_contextMenuMutex;
		ContextMenuRequest m_contextMenuRequest;
		bool m_contextMenuDirty = false;

		std::mutex m_downloadMutex;
		std::string m_pendingDownloadPath;

		IMPLEMENT_REFCOUNTING(WebView);
	};
}
