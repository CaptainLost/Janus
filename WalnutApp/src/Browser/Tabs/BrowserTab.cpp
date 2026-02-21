#include "BrowserTab.h"
#include "../CefInputBridge.h"

#include "../../Utils/UrlUtils.h"
#include "../../Utils/StringUtils.h"

#include <IconsFontAwesome6.h>

BrowserTab::BrowserTab(int id)
	: m_id(id)
{
}

int BrowserTab::GetId() const
{
	return m_id;
}

bool BrowserTab::IsOpen() const
{
	return m_webView != nullptr;
}

void BrowserTab::Open(const std::string& url)
{
	if (m_webView)
	{
		NavigateToUrl(url);

		return;
	}

	std::string finalUrl = UrlUtils::EnsureProtocol(url);

	m_webView = new Walnut::WebView(m_viewWidth, m_viewHeight);
	m_webView->Create(finalUrl);
	m_urlInput = finalUrl;
}

void BrowserTab::Close()
{
	m_browserImage.reset();
	m_faviconImage.reset();

	if (m_webView)
	{
		m_webView->Close();
		m_webView = nullptr;
	}
}

void BrowserTab::NavigateToUrl(const std::string& url)
{
	if (!m_webView)
	{
		return;
	}

	m_urlInput = url;
	m_webView->Navigate(url);
}

int BrowserTab::GetViewWidth() const
{
	return m_viewWidth;
}

int BrowserTab::GetViewHeight() const
{
	return m_viewHeight;
}

void BrowserTab::SetViewSize(int width, int height)
{
	m_viewWidth = width;
	m_viewHeight = height;

	if (m_webView)
	{
		m_webView->SetViewSize(width, height);
	}
}

void BrowserTab::GoBack()
{
	if (m_webView)
	{
		m_webView->GoBack();
	}
}

void BrowserTab::GoForward()
{
	if (m_webView)
	{
		m_webView->GoForward();
	}
}

void BrowserTab::ReloadPage()
{
	if (m_webView)
	{
		m_webView->Reload();
	}
}

void BrowserTab::StopLoading()
{
	if (m_webView)
	{
		m_webView->StopLoading();
	}
}

bool BrowserTab::GetContextMenuRequest(Walnut::ContextMenuRequest& out)
{
	if (!m_webView)
	{
		return false;
	}

	return m_webView->GetContextMenuRequest(out);
}

void BrowserTab::SetPendingDownloadPath(const std::string& path)
{
	if (m_webView)
	{
		m_webView->SetPendingDownloadPath(path);
	}
}

void BrowserTab::StartDownload(const std::string& url)
{
	if (!m_webView)
	{
		return;
	}

	auto browser = m_webView->GetBrowser();
	if (browser)
	{
		browser->GetHost()->StartDownload(url);
	}
}

void BrowserTab::BrowserCut()
{
	if (!m_webView)
	{
		return;
	}

	auto browser = m_webView->GetBrowser();
	if (browser)
	{
		browser->GetFocusedFrame()->Cut();
	}
}

void BrowserTab::BrowserCopy()
{
	if (!m_webView)
	{
		return;
	}

	auto browser = m_webView->GetBrowser();
	if (browser)
	{
		browser->GetFocusedFrame()->Copy();
	}
}

void BrowserTab::BrowserPaste()
{
	if (!m_webView)
	{
		return;
	}

	auto browser = m_webView->GetBrowser();
	if (browser)
	{
		browser->GetFocusedFrame()->Paste();
	}
}

void BrowserTab::BrowserSelectAll()
{
	if (!m_webView)
	{
		return;
	}

	auto browser = m_webView->GetBrowser();
	if (browser)
	{
		browser->GetFocusedFrame()->SelectAll();
	}
}

bool BrowserTab::UpdateBrowserImage()
{
	if (!m_webView)
	{
		return false;
	}

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;

	if (!m_webView->GetPixelBuffer(buffer, width, height) || width <= 0 || height <= 0)
	{
		return false;
	}

	bool needsRecreate =
		!m_browserImage ||
		m_browserImage->GetWidth()  != static_cast<uint32_t>(width) ||
		m_browserImage->GetHeight() != static_cast<uint32_t>(height);

	if (needsRecreate)
	{
		m_browserImage = std::make_shared<Walnut::Image>(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			Walnut::ImageFormat::BGRA);
	}

	m_browserImage->SetData(buffer.data());
	return true;
}

bool BrowserTab::UpdateFaviconImage()
{
	if (!m_webView)
	{
		return false;
	}

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;

	if (!m_webView->GetFaviconPixels(buffer, width, height) || width <= 0 || height <= 0)
	{
		return false;
	}

	m_faviconImage = std::make_shared<Walnut::Image>(
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		Walnut::ImageFormat::RGBA);

	m_faviconImage->SetData(buffer.data());
	return true;
}

void BrowserTab::ForwardInput(int mouseX, int mouseY, bool isHovered, bool isFocused)
{
	if (!m_webView)
	{
		return;
	}

	auto browser = m_webView->GetBrowser();
	if (!browser)
	{
		return;
	}

	auto host = browser->GetHost();
	CefInputBridge::ForwardMouseEvents(host, mouseX, mouseY, isHovered);
	CefInputBridge::ForwardKeyboardEvents(host, isFocused);
}

CefRefPtr<Walnut::WebView> BrowserTab::GetWebView() const
{
	return m_webView;
}

std::shared_ptr<Walnut::Image> BrowserTab::GetBrowserImage() const
{
	return m_browserImage;
}

void BrowserTab::SetBrowserImage(std::shared_ptr<Walnut::Image> image)
{
	m_browserImage = std::move(image);
}

std::shared_ptr<Walnut::Image> BrowserTab::GetFaviconImage() const
{
	return m_faviconImage;
}

void BrowserTab::SetFaviconImage(std::shared_ptr<Walnut::Image> image)
{
	m_faviconImage = std::move(image);
}

Walnut::WebViewState BrowserTab::GetWebViewState() const
{
	return m_webView ? m_webView->GetState() : Walnut::WebViewState{};
}

std::string& BrowserTab::GetUrlInput()
{
	return m_urlInput;
}

const std::string& BrowserTab::GetUrlInput() const
{
	return m_urlInput;
}

std::string BrowserTab::GetTabBarLabel() const
{
	if (IsOpen())
	{
		Walnut::WebViewState viewState = GetWebViewState();

		return viewState.Title.empty() ? "No Title" : viewState.Title;
	}

	return "Empty Tab";
}

std::string BrowserTab::GetSidebarLabel() const
{
	if (!IsOpen())
	{
		return ICON_FA_FILE " New Tab";
	}

	Walnut::WebViewState state = GetWebViewState();
	const char* icon = state.IsLoading ? ICON_FA_SPINNER : ICON_FA_GLOBE;
	std::string title = state.Title.empty() ? "Loading..." : state.Title;

	return StringUtils::Truncate(std::string(icon) + " " + title, 30);
}
