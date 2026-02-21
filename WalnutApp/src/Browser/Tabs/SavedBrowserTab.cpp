#include "SavedBrowserTab.h"

#include "../../Utils/UrlUtils.h"
#include "../../Utils/StringUtils.h"

#include <IconsFontAwesome6.h>

SavedBrowserTab::SavedBrowserTab(int id, int dbId, const std::string& baseUrl)
	: BrowserTab(id), m_dbId(dbId), m_baseUrl(baseUrl)
{
}

void SavedBrowserTab::Open(const std::string& url)
{
	BrowserTab::Open(url);

	GetWebView()->SetBeforeBrowseCallback([this](const std::string& navigatedUrl) -> bool {
		if (!UrlUtils::IsHttp(navigatedUrl))
		{
			return false;
		}

		if (!UrlUtils::IsSameDomain(UrlUtils::GetHost(navigatedUrl), UrlUtils::GetHost(m_baseUrl)))
		{
			if (m_onDomainExit)
			{
				m_onDomainExit(navigatedUrl);
			}

			return true;
		}

		return false;
	});
}

void SavedBrowserTab::SetDomainExitHandler(std::function<void(const std::string&)> handler)
{
	m_onDomainExit = std::move(handler);
}

int SavedBrowserTab::GetDbId() const
{
	return m_dbId;
}

const std::string& SavedBrowserTab::GetBaseUrl() const
{
	return m_baseUrl;
}

void SavedBrowserTab::Detach()
{
	m_detached = true;
}

bool SavedBrowserTab::IsSaved() const
{
	return !m_detached;
}

std::string SavedBrowserTab::GetSidebarLabel() const
{
	Walnut::WebViewState state = GetWebViewState();

	if (!state.Title.empty())
	{
		return state.Title;
	}

	std::string host = UrlUtils::GetHost(m_baseUrl);
	return host.empty() ? m_baseUrl : host;
}
