# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Janus** is a custom desktop browser application built on the **Walnut** framework — an ImGui + Vulkan application framework that embeds **CEF** (Chromium Embedded Framework) for off-screen web rendering.

## Build System

**Premake5** generates Visual Studio 2026 project files. No CMake.

```bat
# From the repo root, generate VS2026 solution:
scripts\Setup.bat
# or directly:
vendor\bin\premake5.exe vs2026
```

Build configurations: **Debug**, **Release**, **Dist** (windowed, no console, no symbols).

Output lands in `bin/<Config>-windows-x86_64/WalnutApp/`.

**Requirements before building:**
- Vulkan SDK installed; `VULKAN_SDK` environment variable must be set
- CEF libraries present under `vendor/cef/Debug/` and `vendor/cef/Release/`
- Visual Studio 2022

## Project Structure

Two compiled targets, both C++20:

| Target | Type | Location |
|---|---|---|
| `Walnut` | Static lib | `Walnut/src/Walnut/` |
| `WalnutApp` | ConsoleApp / WindowedApp | `WalnutApp/src/` |

`WalnutExternal.lua` declares all include/library paths and builds the dependency group (ImGui, GLFW, CEF).

## Architecture

### Walnut Framework (`Walnut/`)

- **`Application`** — owns the Vulkan instance/device/swapchain, GLFW window, ImGui context, and a layer stack. Calls `OnUpdate(ts)` then `OnUIRender()` each frame. The dockspace ID is `"VulkanAppDockspace"`.
- **`Layer`** — interface with `OnAttach()`, `OnDetach()`, `OnUpdate(float ts)`, `OnUIRender()`. Subclass this to add features.
- **`WebView`** — off-screen CEF browser. Implements `CefClient` + four handlers. Renders into an internal RGBA pixel buffer (mutex-protected). Call `GetPixelBuffer()` to pull the latest frame; call `GetState()` for a consistent URL/title/loading snapshot. `Create()` spawns the browser; `Close()` tears it down.
- **`Image`** — Vulkan texture wrapper. Upload CPU data via `SetData()`; display via `GetDescriptorSet()` as an ImGui image.
- **`CustomTitlebar`** — draws a frameless title bar with drag support; requires `ApplicationSpecification::CustomTitlebar = true`.

### Application Layer (`WalnutApp/src/`)

```
BrowserLayer          (Walnut::Layer subclass — the only layer pushed)
├── TabManager2       (owns the list of BrowserTab2 instances)
│   └── BrowserTab2  (owns one WebView + one Image; manages TabState)
└── BrowserViewport   (renders address bar + ImGui DockSpace tab bar + CEF image)
    └── AddressBar    (navigation buttons + URL input text field)
```

**`BrowserLayer::OnUpdate()`** — calls `CefDoMessageLoopWork()` each frame, then calls `BrowserViewport::UpdateBrowserImage()` for every non-blank tab (pulls the pixel buffer from `WebView` and uploads to `Image`).

**`BrowserLayer::OnUIRender()`** — builds a one-time docked layout (20 % sidebar / 80 % main), renders the sidebar tab list + "New Tab" button, then renders the main viewport.

**`BrowserViewport::RenderTabBar()`** — uses an inner `ImGui::DockSpace("MainViewport")` so each tab is its own dockable `ImGui::Begin` window. Tab windows use the format `"<icon> <label>###TabWindow_<id>"`.

**`CefInputBridge`** (header-only, `WalnutApp/src/CefInputBridge.h`) — stateless helpers that translate ImGui mouse/keyboard events into CEF events. Lives in `WalnutApp` intentionally (couples ImGui types to CEF types, which the core `Walnut` lib avoids).

### Tab Lifecycle

1. `TabManager2::AddTab()` creates a `BrowserTab2` in `TabState::Blank`.
2. `BrowserTab2::Open(url)` creates a `WebView`, calls `Create(url)`, transitions to `TabState::Loading`.
3. CEF fires `OnLoadingStateChange` → state transitions to `TabState::Ready` (this wiring is in `WebView`, not fully shown — extend as needed).
4. `RemoveTab()` calls `BrowserTab2::Close()`, which calls `WebView::Close()` and resets the image.

## Fonts & Embedded Assets

Fonts are compiled into C++ byte arrays (`.embed` files) in `Walnut/src/Walnut/ImGui/`:
- `Roboto-Regular.embed` → `g_RobotoRegular` (default UI font, 18 px)
- `Kingdom.embed` → `g_Kingdom` (titlebar font, 20 px)
- `FontAwesome.embed` → `g_FontAwesome` (merged into Roboto at 16 px; range `ICON_MIN_FA`–`ICON_MAX_FA`)

To add a new font, use:
```powershell
.\scripts\Convert-FontToEmbed.ps1 -InputFile MyFont.ttf -OutputFile MyFont.embed -ArrayName g_MyFont
```

Icons come from `vendor/IconFontCppHeaders` (`IconsFontAwesome6.h`). Use macros like `ICON_FA_PLUS`, `ICON_FA_XMARK`, `ICON_FA_GLOBE`, etc.

## Key Preprocessor Defines

| Define | Set by |
|---|---|
| `WL_PLATFORM_WINDOWS` | premake filter |
| `WL_DEBUG` / `WL_RELEASE` / `WL_DIST` | build config |
| `CEF_AVAILABLE` | Walnut lib |
| `USING_CEF_SHARED` | both targets |
| `IMGUI_DEFINE_MATH_OPERATORS` | WalnutApp |
| `NOMINMAX` / `WIN32_LEAN_AND_MEAN` | both targets |

## CEF Notes

- CEF DLLs/resources are post-build-copied from `vendor/cef/Debug|Release/` into the output directory.
- `CefDoMessageLoopWork()` is the integration point — called once per render frame in `BrowserLayer::OnUpdate()`.
- `WebView::m_BufferMutex` guards the pixel buffer; `WebView::m_StateMutex` guards URL/title/loading state. Always use `GetState()` / `GetPixelBuffer()` from the UI thread.
- `io.ConfigWindowsMoveFromTitleBarOnly = true` is set in `Application::Init()` specifically to prevent dragging conflicts with the embedded browser content.
