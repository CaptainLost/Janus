#pragma once

#include "TabState.h"

#include "Walnut/Image.h"
#include "Walnut/WebView.h"

#include <memory>
#include <string>

class BrowserTab2
{
public:
	explicit BrowserTab2(int id);

	int      GetId()    const;
	TabState GetState() const;

	void Open(const std::string& url);
	void Close();
	void NavigateToUrl(const std::string& url);

	int  GetViewWidth()  const;
	int  GetViewHeight() const;
	void SetViewSize(int width, int height);

	CefRefPtr<Walnut::WebView>     GetWebView() const;
	std::shared_ptr<Walnut::Image> GetBrowserImage() const;
	void                           SetBrowserImage(std::shared_ptr<Walnut::Image> image);
	Walnut::WebViewState           GetWebViewState() const;

	std::string&       GetUrlInput();
	const std::string& GetUrlInput() const;

	std::string GetTabLabel() const;

private:
	int      m_id    = -1;
	TabState m_state = TabState::Blank;

	std::string m_urlInput;

	int m_viewWidth  = 1280;
	int m_viewHeight = 720;

	CefRefPtr<Walnut::WebView>     m_webView;
	std::shared_ptr<Walnut::Image> m_browserImage;
};