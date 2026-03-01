#pragma once

#include "../Browser/Tabs/Tab.h"
#include "Walnut/WebView.h"

#include <memory>

class AdressBarPanel
{
public:
	AdressBarPanel();

	void Render(const std::shared_ptr<Tab>& tab);

private:
	void RenderNavigationButtons(const std::shared_ptr<Tab>& tab);
	void RenderUrlInput(const std::shared_ptr<Tab>& tab);
	void RenderSecurityIcon(const std::shared_ptr<Tab>& tab, const Walnut::WebViewState& state);
};
