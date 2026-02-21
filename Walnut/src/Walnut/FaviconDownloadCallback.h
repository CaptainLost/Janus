#pragma once

#include "WebView.h"

#include "include/cef_browser.h"
#include "include/cef_image.h"

namespace Walnut
{
	class FaviconDownloadCallback : public CefDownloadImageCallback
	{
	public:
		explicit FaviconDownloadCallback(WebView* webView) : m_webView(webView) {}

		void OnDownloadImageFinished(const CefString& /*imageUrl*/, int httpStatusCode, CefRefPtr<CefImage> image) override
		{
			if (!image || httpStatusCode != 200)
			{
				return;
			}

			int width = 0, height = 0;
			CefRefPtr<CefBinaryValue> pixels = image->GetAsBitmap(
				1.0f, CEF_COLOR_TYPE_RGBA_8888, CEF_ALPHA_TYPE_POSTMULTIPLIED, width, height);

			if (!pixels || width <= 0 || height <= 0)
			{
				return;
			}

			size_t dataSize = pixels->GetSize();
			std::vector<uint8_t> data(dataSize);
			pixels->GetData(data.data(), dataSize, 0);

			m_webView->SetFaviconData(std::move(data), width, height);
		}

	private:
		WebView* m_webView;
		IMPLEMENT_REFCOUNTING(FaviconDownloadCallback);
	};
}
