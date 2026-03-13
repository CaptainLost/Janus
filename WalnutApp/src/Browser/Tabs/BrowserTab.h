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

	void GoBack() override;
	void GoForward() override;
	void ReloadPage() override;
	void StopLoading() override;

	bool GetContextMenuRequest(Walnut::ContextMenuRequest& out) override;
	void SetPendingDownloadPath(const std::string& path) override;
	void StartDownload(const std::string& url) override;
	void BrowserCut() override;
	void BrowserCopy() override;
	void BrowserPaste() override;
	void BrowserSelectAll() override;

	bool UpdateBrowserImage() override;
	bool UpdateFaviconImage() override;
	void ForwardInput(int mouseX, int mouseY, bool isHovered, bool isFocused) override;

	std::shared_ptr<Walnut::Image> GetBrowserImage() const override;
	void SetBrowserImage(std::shared_ptr<Walnut::Image> image) override;
	std::shared_ptr<Walnut::Image> GetFaviconImage() const override;
	void SetFaviconImage(std::shared_ptr<Walnut::Image> image) override;
	Walnut::WebViewState GetWebViewState() const override;

	std::string& GetUrlInput() override;
	const std::string& GetUrlInput() const override;

	std::string GetTabBarLabel() const override;
	std::string GetSidebarLabel() const override;

protected:
	CefRefPtr<Walnut::WebView> GetWebView() const;

private:
	int m_id = -1;

	std::string m_urlInput;

	int m_viewWidth = 1280;
	int m_viewHeight = 720;

	CefRefPtr<Walnut::WebView> m_webView;
	std::shared_ptr<Walnut::Image> m_browserImage;
	std::shared_ptr<Walnut::Image> m_faviconImage;
	std::vector<uint8_t> m_uploadBuffer;
};
