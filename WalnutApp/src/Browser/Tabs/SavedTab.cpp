#include "SavedTab.h"

#include "../../Utils/UrlUtils.h"

SavedTab::SavedTab(int id, int dbId, const std::string& baseUrl)
	: BrowserTab(id), m_dbId(dbId), m_baseUrl(baseUrl)
{
}

void SavedTab::Open(const std::string& url)
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

void SavedTab::SetDomainExitHandler(std::function<void(const std::string&)> handler)
{
	m_onDomainExit = std::move(handler);
}

int SavedTab::GetDbId() const
{
	return m_dbId;
}

const std::string& SavedTab::GetBaseUrl() const
{
	return m_baseUrl;
}

void SavedTab::Detach()
{
	m_detached = true;
}

bool SavedTab::IsSaved() const
{
	return !m_detached;
}
