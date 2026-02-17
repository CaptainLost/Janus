#pragma once

#include "Walnut/Layer.h"

#include "Tabs/TabManager2.h"
#include "UI/BrowserViewport.h"

#include <memory>
#include <string>
#include <vector>

class BrowserLayer : public Walnut::Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(float ts) override;
	void OnUIRender() override;

private:
	void BuildDockLayout();
	void RenderSidebar();
	void RenderMainViewport();

	TabManager2     m_tabManager;
	BrowserViewport m_viewport{"main"};

	bool m_layoutBuilt = false;
};
