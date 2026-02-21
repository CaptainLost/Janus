#pragma once

#include "BrowserTab.h"

class SavedBrowserTab : public BrowserTab
{
public:
	SavedBrowserTab(int id, int dbId, const std::string& baseUrl);

	void Open(const std::string& url) override;

	void SetDomainExitHandler(std::function<void(const std::string&)> handler);

	int GetDbId() const;
	const std::string& GetBaseUrl() const;
	void Detach();

	bool IsSaved() const override;

	std::string GetSidebarLabel() const override;

private:
	int m_dbId;
	std::string m_baseUrl;
	bool m_detached = false;

	std::function<void(const std::string&)> m_onDomainExit;
};