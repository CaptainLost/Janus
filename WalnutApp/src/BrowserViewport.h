#pragma once

#include "Tabs/TabManager2.h"

#include "imgui.h"

#include <functional>
#include <string>

// Forward declaration.
class BrowserViewport;

/// Global drag state shared between all BrowserViewport instances.
/// Only one tab can be dragged at a time.
struct TabDragState
{
	bool             active     = false;
	int              tabId      = -1;
	TabManager2*     srcManager = nullptr;
	BrowserViewport* srcViewport = nullptr;

	void Clear() { active = false; tabId = -1; srcManager = nullptr; srcViewport = nullptr; }
};

class BrowserViewport
{
public:
	explicit BrowserViewport(const std::string& uniqueId = "main");

	void Render(TabManager2& tabManager);
	void UpdateBrowserImage(BrowserTab2& tab);

	using DetachCallback = std::function<void(TabManager2&, int, ImVec2)>;
	void SetDetachCallback(DetachCallback cb);

	static int StringResizeCallback(ImGuiInputTextCallbackData* data);

	/// Global drag state — shared across all viewports.
	static TabDragState s_dragState;

private:
	void RenderAddressBar(TabManager2& tabManager);
	void RenderTabBar(TabManager2& tabManager);
	void RenderTabContent(BrowserTab2& tab);
	void RenderBrowserContent(BrowserTab2& tab);
	void ForwardInputToBrowser(BrowserTab2& tab, ImVec2 imagePos);
	void RenderDragOverlay();

	std::string    m_uniqueId;
	int            m_pendingSelectTabId = -1;
	bool           m_urlBarFocused      = false;
	DetachCallback m_detachCallback;
};
