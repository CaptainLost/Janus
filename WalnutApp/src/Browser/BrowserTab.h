#pragma once

#include "Tab.h"

class BrowserTab : public Tab
{
public:
	explicit BrowserTab(int id);

	int GetId() const override;
	bool IsOpen() const override;

	void Open(const std::string& url) override;
	void Close() override;
	void NavigateToUrl(const std::string& url) override;

	int GetViewWidth() const override;
	int GetViewHeight() const override;
	void SetViewSize(int width, int height) override;

	CefRefPtr<Walnut::WebView> GetWebView() const override;
	std::shared_ptr<Walnut::Image> GetBrowserImage() const override;
	void SetBrowserImage(std::shared_ptr<Walnut::Image> image) override;
	std::shared_ptr<Walnut::Image> GetFaviconImage() const override;
	void SetFaviconImage(std::shared_ptr<Walnut::Image> image) override;
	Walnut::WebViewState GetWebViewState() const override;

	std::string& GetUrlInput() override;
	const std::string& GetUrlInput() const override;

	std::string GetTabLabel() const override;

private:
	int m_id = -1;

	std::string m_urlInput;

	int m_viewWidth = 1280;
	int m_viewHeight = 720;

	CefRefPtr<Walnut::WebView> m_webView;
	std::shared_ptr<Walnut::Image> m_browserImage;
	std::shared_ptr<Walnut::Image> m_faviconImage;
};
