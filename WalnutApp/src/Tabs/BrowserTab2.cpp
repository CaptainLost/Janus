#include "BrowserTab2.h"

BrowserTab2::BrowserTab2(int id)
	: m_id(id)
{

}

int BrowserTab2::GetId() const
{
	return m_id;
}

bool BrowserTab2::IsOpen() const
{
	return m_webView != nullptr;
}

void BrowserTab2::Open(const std::string& url)
{
	if (m_webView)
	{
		NavigateToUrl(url);
		return;
	}

	m_webView = new Walnut::WebView(m_viewWidth, m_viewHeight);
	m_webView->Create(url);
	m_urlInput = url;
}

void BrowserTab2::Close()
{
	m_browserImage.reset();

	if (m_webView)
	{
		m_webView->Close();
		m_webView = nullptr;
	}
}

void BrowserTab2::NavigateToUrl(const std::string& url)
{
	if (!m_webView)
		return;

	m_urlInput = url;
	m_webView->Navigate(url);
}

int BrowserTab2::GetViewWidth() const
{
	return m_viewWidth;
}

int BrowserTab2::GetViewHeight() const
{
	return m_viewHeight;
}

void BrowserTab2::SetViewSize(int width, int height)
{
	m_viewWidth = width;
	m_viewHeight = height;
	if (m_webView)
		m_webView->SetViewSize(width, height);
}

CefRefPtr<Walnut::WebView> BrowserTab2::GetWebView() const
{
	return m_webView;
}

std::shared_ptr<Walnut::Image> BrowserTab2::GetBrowserImage() const
{
	return m_browserImage;
}

void BrowserTab2::SetBrowserImage(std::shared_ptr<Walnut::Image> image)
{
	m_browserImage = std::move(image);
}

Walnut::WebViewState BrowserTab2::GetWebViewState() const
{
	return m_webView ? m_webView->GetState() : Walnut::WebViewState{};
}

std::string& BrowserTab2::GetUrlInput()
{
	return m_urlInput;
}

const std::string& BrowserTab2::GetUrlInput() const
{
	return m_urlInput;
}

std::string BrowserTab2::GetTabLabel() const
{
	Walnut::WebViewState viewState = GetWebViewState();

	std::string label = "";
	label += viewState.Title;
	label += "Tab ";
	label += std::to_string(m_id);

	return label;
}
