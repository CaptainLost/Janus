#pragma once

/// Walnut::WebView — a reusable, off-screen CEF browser wrapper.
///
/// This class owns a single CEF browser instance rendered off-screen.
/// It is independent of any UI framework — it simply maintains a pixel buffer
/// (RGBA) that higher-level code can blit onto a texture / ImGui image.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_display_handler.h"

namespace Walnut {

	/// Snapshot of the browser's navigational state.
	/// Returned atomically so UI code can read it without locking.
	struct WebViewState
	{
		std::string URL;
		std::string Title;
		bool        IsLoading  = false;
		bool        CanGoBack  = false;
		bool        CanGoForward = false;
	};

	class WebView : public CefClient,
	                public CefRenderHandler,
	                public CefLifeSpanHandler,
	                public CefLoadHandler,
	                public CefDisplayHandler
	{
	public:
		/// Construct a WebView with an initial viewport size.
		WebView(int width, int height);

		/// Create the underlying CEF browser and navigate to `startURL`.
		/// Call this once after constructing the WebView.
		void Create(const std::string& startURL);

		/// Close the browser and wait for CEF to finish tearing it down.
		/// Safe to call multiple times or on a null browser.
		void Close();

		// -- Navigation -------------------------------------------------------

		void Navigate(const std::string& url);
		void GoBack();
		void GoForward();
		void Reload();
		void StopLoading();

		// -- View management --------------------------------------------------

		void SetViewSize(int width, int height);
		int  GetViewWidth()  const { return m_ViewWidth; }
		int  GetViewHeight() const { return m_ViewHeight; }

		// -- State queries ----------------------------------------------------

		/// Returns a consistent snapshot of URL, title, loading state, etc.
		WebViewState GetState() const;

		/// Copy the latest rendered frame into `outBuffer`.
		/// Returns true if a new frame was available (dirty flag cleared).
		bool GetPixelBuffer(std::vector<uint8_t>& outBuffer, int& outWidth, int& outHeight);

		// -- Low-level CEF access (for input forwarding, etc.) ----------------

		CefRefPtr<CefBrowser> GetBrowser() const;

	private:
		// -- CefClient --------------------------------------------------------
		CefRefPtr<CefRenderHandler>   GetRenderHandler()   override { return this; }
		CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
		CefRefPtr<CefLoadHandler>     GetLoadHandler()     override { return this; }
		CefRefPtr<CefDisplayHandler>  GetDisplayHandler()  override { return this; }

		// -- CefRenderHandler -------------------------------------------------
		void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
		void OnPaint(CefRefPtr<CefBrowser> browser,
		             PaintElementType type,
		             const RectList& dirtyRects,
		             const void* buffer,
		             int width, int height) override;

		// -- CefLifeSpanHandler -----------------------------------------------
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
		void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

		// -- CefLoadHandler ---------------------------------------------------
		void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
		                          bool isLoading, bool canGoBack, bool canGoForward) override;

		// -- CefDisplayHandler ------------------------------------------------
		void OnAddressChange(CefRefPtr<CefBrowser> browser,
		                     CefRefPtr<CefFrame> frame,
		                     const CefString& url) override;
		void OnTitleChange(CefRefPtr<CefBrowser> browser,
		                   const CefString& title) override;

	private:
		// Viewport dimensions (atomic-ish — only written from UI thread)
		int m_ViewWidth;
		int m_ViewHeight;

		// Pixel buffer produced by OnPaint (BGRA ? RGBA converted)
		mutable std::mutex      m_BufferMutex;
		std::vector<uint8_t>    m_PixelBuffer;
		int                     m_BufferWidth  = 0;
		int                     m_BufferHeight = 0;
		bool                    m_BufferDirty  = false;

		// Navigation / display state
		mutable std::mutex m_StateMutex;
		std::string        m_CurrentURL;
		std::string        m_Title;
		bool               m_IsLoading   = false;
		bool               m_CanGoBack   = false;
		bool               m_CanGoForward = false;

		// The underlying CEF browser
		CefRefPtr<CefBrowser> m_Browser;

		IMPLEMENT_REFCOUNTING(WebView);
	};

} // namespace Walnut
