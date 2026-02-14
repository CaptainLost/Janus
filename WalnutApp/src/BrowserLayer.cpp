#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "BrowserLayer.h"
#include "CefInputBridge.h"

#include "include/cef_app.h"

#include <algorithm>
#include <cstring>

void BrowserLayer::OnAttach()
{
	m_WebView = new Walnut::WebView(m_ViewWidth, m_ViewHeight);
	m_WebView->Create(m_StartURL);

	std::strncpy(m_URLBuffer, m_StartURL.c_str(), sizeof(m_URLBuffer) - 1);
	m_URLBuffer[sizeof(m_URLBuffer) - 1] = '\0';
}

void BrowserLayer::OnDetach()
{
	m_BrowserImage.reset();

	if (m_WebView)
	{
		m_WebView->Close();
		m_WebView = nullptr;
	}
}

void BrowserLayer::OnUpdate(float ts)
{
	CefDoMessageLoopWork();
	UpdateBrowserImage();
	SyncURLFromBrowser();
}

void BrowserLayer::OnUIRender()
{
	RenderAddressBar();
	RenderBrowserViewport();
}

void BrowserLayer::UpdateBrowserImage()
{
	if (!m_WebView)
		return;

	std::vector<uint8_t> buffer;
	int width = 0, height = 0;

	if (!m_WebView->GetPixelBuffer(buffer, width, height))
		return;
	if (width <= 0 || height <= 0)
		return;

	const bool needsRecreate =
		!m_BrowserImage ||
		m_BrowserImage->GetWidth()  != static_cast<uint32_t>(width) ||
		m_BrowserImage->GetHeight() != static_cast<uint32_t>(height);

	if (needsRecreate)
	{
		m_BrowserImage = std::make_shared<Walnut::Image>(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			Walnut::ImageFormat::RGBA);
	}

	m_BrowserImage->SetData(buffer.data());
}

void BrowserLayer::SyncURLFromBrowser()
{
	if (m_URLBarFocused || !m_WebView)
		return;

	const auto state = m_WebView->GetState();
	if (!state.URL.empty())
	{
		std::strncpy(m_URLBuffer, state.URL.c_str(), sizeof(m_URLBuffer) - 1);
		m_URLBuffer[sizeof(m_URLBuffer) - 1] = '\0';
	}
}

void BrowserLayer::RenderAddressBar()
{
	ImGui::Begin("Navigation", nullptr, ImGuiWindowFlags_NoCollapse);

	const auto state = m_WebView ? m_WebView->GetState() : Walnut::WebViewState{};

	// Back
	ImGui::BeginDisabled(!state.CanGoBack);
	if (ImGui::Button("<") && m_WebView) m_WebView->GoBack();
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Forward
	ImGui::BeginDisabled(!state.CanGoForward);
	if (ImGui::Button(">") && m_WebView) m_WebView->GoForward();
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Stop / Reload
	if (state.IsLoading)
	{
		if (ImGui::Button("X") && m_WebView) m_WebView->StopLoading();
	}
	else
	{
		if (ImGui::Button("O") && m_WebView) m_WebView->Reload();
	}

	ImGui::SameLine();

	// URL input
	const float goWidth = ImGui::CalcTextSize("Go").x +
	                      ImGui::GetStyle().ItemSpacing.x * 2 +
	                      ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - goWidth);

	const bool enterPressed = ImGui::InputText(
		"##url", m_URLBuffer, sizeof(m_URLBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue);
	m_URLBarFocused = ImGui::IsItemActive();

	ImGui::SameLine();

	if ((enterPressed || ImGui::Button("Go")) && m_WebView)
		m_WebView->Navigate(m_URLBuffer);

	// Title
	if (!state.Title.empty())
		ImGui::TextWrapped("Title: %s", state.Title.c_str());

	if (state.IsLoading)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), " Loading...");
	}

	ImGui::End();
}

void BrowserLayer::RenderBrowserViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Browser", nullptr,
	             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	// Resize
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const int newW = (std::max)(static_cast<int>(region.x), 64);
	const int newH = (std::max)(static_cast<int>(region.y), 64);

	if ((newW != m_ViewWidth || newH != m_ViewHeight) && m_WebView)
	{
		m_ViewWidth  = newW;
		m_ViewHeight = newH;
		m_WebView->SetViewSize(m_ViewWidth, m_ViewHeight);
	}

	// Display
	if (m_BrowserImage)
	{
		ImGui::Image(m_BrowserImage->GetDescriptorSet(),
		             ImVec2(static_cast<float>(m_BrowserImage->GetWidth()),
		                    static_cast<float>(m_BrowserImage->GetHeight())));
	}
	else
	{
		ImGui::Text("Browser starting...");
	}

	// Input forwarding
	ForwardInputToBrowser();

	ImGui::End();
	ImGui::PopStyleVar();
}

void BrowserLayer::ForwardInputToBrowser()
{
	if (!m_WebView)
		return;

	auto browser = m_WebView->GetBrowser();
	if (!browser)
		return;

	const bool isHovered = ImGui::IsWindowHovered();
	const bool isFocused = ImGui::IsWindowFocused();

	if (!isHovered && !isFocused)
		return;

	auto host = browser->GetHost();

	// Calculate mouse position relative to browser image
	const ImVec2 windowPos  = ImGui::GetWindowPos();
	const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
	const ImGuiIO& io       = ImGui::GetIO();

	const int mouseX = static_cast<int>(io.MousePos.x - windowPos.x - contentMin.x);
	const int mouseY = static_cast<int>(io.MousePos.y - windowPos.y - contentMin.y);

	CefInputBridge::ForwardMouseEvents(host, mouseX, mouseY, isHovered);
	CefInputBridge::ForwardKeyboardEvents(host, isFocused);
}
