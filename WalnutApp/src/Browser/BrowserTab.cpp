#include "BrowserTab.h"

#include "../Utils/UrlUtils.h"

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

std::string BrowserTab::GetTabLabel() const
{
	if (IsOpen())
	{
		Walnut::WebViewState viewState = GetWebViewState();

		return viewState.Title.empty() ? "No Title" : viewState.Title;
	}

	return "Empty Tab";
}
