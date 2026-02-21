#pragma once

#include "Walnut/WebView.h"
#include "Walnut/Image.h"

#include <memory>
#include <string>

class Tab
{
public:
	virtual ~Tab() = default;

	virtual int GetId() const = 0;
	virtual bool IsOpen() const = 0;

	virtual void Open(const std::string& url) = 0;
	virtual void Close() = 0;
	virtual void NavigateToUrl(const std::string& url) = 0;

	virtual int GetViewWidth() const = 0;
	virtual int GetViewHeight() const = 0;
	virtual void SetViewSize(int width, int height) = 0;

	virtual void GoBack() {}
	virtual void GoForward() {}
	virtual void ReloadPage() {}
	virtual void StopLoading() {}

	virtual bool GetContextMenuRequest(Walnut::ContextMenuRequest& out) { return false; }
	virtual void SetPendingDownloadPath(const std::string& path) {}
	virtual void StartDownload(const std::string& url) {}
	virtual void BrowserCut() {}
	virtual void BrowserCopy() {}
	virtual void BrowserPaste() {}
	virtual void BrowserSelectAll() {}

	virtual bool UpdateBrowserImage() { return false; }
	virtual bool UpdateFaviconImage() { return false; }
	virtual void ForwardInput(int mouseX, int mouseY, bool isHovered, bool isFocused) {}

	virtual std::shared_ptr<Walnut::Image> GetBrowserImage() const = 0;
	virtual void SetBrowserImage(std::shared_ptr<Walnut::Image> image) = 0;
	virtual std::shared_ptr<Walnut::Image> GetFaviconImage() const = 0;
	virtual void SetFaviconImage(std::shared_ptr<Walnut::Image> image) = 0;
	virtual Walnut::WebViewState GetWebViewState() const = 0;

	virtual std::string& GetUrlInput() = 0;
	virtual const std::string& GetUrlInput() const = 0;

	virtual std::string GetTabBarLabel() const = 0;
	virtual std::string GetSidebarLabel() const = 0;

	virtual bool IsSaved() const { return false; }
};
