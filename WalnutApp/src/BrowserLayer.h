#pragma once

#include "Walnut/Layer.h"
#include "Walnut/Image.h"
#include "Walnut/WebView.h"

#include "include/cef_base.h"

#include <memory>
#include <string>

class BrowserLayer : public Walnut::Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float ts) override;
	void OnUIRender() override;

private:
	void UpdateBrowserImage();
	void SyncURLFromBrowser();
	void RenderAddressBar();
	void RenderBrowserViewport();
	void ForwardInputToBrowser();

	CefRefPtr<Walnut::WebView>         m_WebView;
	std::shared_ptr<Walnut::Image>     m_BrowserImage;

	int  m_ViewWidth  = 1280;
	int  m_ViewHeight = 720;

	char m_URLBuffer[2048] = {};
	bool m_URLBarFocused   = false;

	std::string m_StartURL = "https://www.google.com";
};
