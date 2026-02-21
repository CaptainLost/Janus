#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "imgui.h"

#include "include/cef_browser.h"

#include <cstdint>

namespace CefInputBridge {

	inline uint32_t GetCefModifiers()
	{
		uint32_t modifiers = 0;
		ImGuiIO& io = ImGui::GetIO();

		if (io.KeyShift)
		{
			modifiers |= EVENTFLAG_SHIFT_DOWN;
		}
		if (io.KeyCtrl)
		{
			modifiers |= EVENTFLAG_CONTROL_DOWN;
		}
		if (io.KeyAlt)
		{
			modifiers |= EVENTFLAG_ALT_DOWN;
		}
		if (io.MouseDown[0])
		{
			modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
		}
		if (io.MouseDown[1])
		{
			modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
		}
		if (io.MouseDown[2])
		{
			modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
		}

		return modifiers;
	}

	inline cef_mouse_button_type_t ImGuiButtonToCef(int button)
	{
		switch (button)
		{
		case 0:  return MBT_LEFT;
		case 1:  return MBT_RIGHT;
		case 2:  return MBT_MIDDLE;
		default: return MBT_LEFT;
		}
	}

	inline int ImGuiKeyToVK(ImGuiKey key)
	{
		switch (key)
		{
		case ImGuiKey_Tab:        return VK_TAB;
		case ImGuiKey_LeftArrow:  return VK_LEFT;
		case ImGuiKey_RightArrow: return VK_RIGHT;
		case ImGuiKey_UpArrow:    return VK_UP;
		case ImGuiKey_DownArrow:  return VK_DOWN;
		case ImGuiKey_PageUp:     return VK_PRIOR;
		case ImGuiKey_PageDown:   return VK_NEXT;
		case ImGuiKey_Home:       return VK_HOME;
		case ImGuiKey_End:        return VK_END;
		case ImGuiKey_Insert:     return VK_INSERT;
		case ImGuiKey_Delete:     return VK_DELETE;
		case ImGuiKey_Backspace:  return VK_BACK;
		case ImGuiKey_Space:      return VK_SPACE;
		case ImGuiKey_Enter:      return VK_RETURN;
		case ImGuiKey_Escape:     return VK_ESCAPE;
		case ImGuiKey_A:          return 'A';
		case ImGuiKey_C:          return 'C';
		case ImGuiKey_V:          return 'V';
		case ImGuiKey_X:          return 'X';
		case ImGuiKey_Y:          return 'Y';
		case ImGuiKey_Z:          return 'Z';
		case ImGuiKey_F1:         return VK_F1;
		case ImGuiKey_F2:         return VK_F2;
		case ImGuiKey_F3:         return VK_F3;
		case ImGuiKey_F4:         return VK_F4;
		case ImGuiKey_F5:         return VK_F5;
		case ImGuiKey_F6:         return VK_F6;
		case ImGuiKey_F7:         return VK_F7;
		case ImGuiKey_F8:         return VK_F8;
		case ImGuiKey_F9:         return VK_F9;
		case ImGuiKey_F10:        return VK_F10;
		case ImGuiKey_F11:        return VK_F11;
		case ImGuiKey_F12:        return VK_F12;
		default:                  return 0;
		}
	}

	inline void ForwardMouseEvents(CefRefPtr<CefBrowserHost> host,
	                               int mouseX, int mouseY,
	                               bool isHovered)
	{
		if (!isHovered)
		{
			return;
		}

		const uint32_t mods = GetCefModifiers();

		{
			CefMouseEvent ev;
			ev.x = mouseX;
			ev.y = mouseY;
			ev.modifiers = mods;
			host->SendMouseMoveEvent(ev, false);
		}

		for (int i = 0; i < 3; i++)
		{
			if (ImGui::IsMouseClicked(i))
			{
				CefMouseEvent ev;
				ev.x = mouseX;
				ev.y = mouseY;
				ev.modifiers = mods;
				host->SendMouseClickEvent(ev, ImGuiButtonToCef(i), false, 1);
				host->SetFocus(true);
			}

			if (ImGui::IsMouseReleased(i))
			{
				CefMouseEvent ev;
				ev.x = mouseX;
				ev.y = mouseY;
				ev.modifiers = mods;
				host->SendMouseClickEvent(ev, ImGuiButtonToCef(i), true, 1);
			}
		}

		ImGuiIO& io = ImGui::GetIO();
		if (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f)
		{
			CefMouseEvent ev;
			ev.x = mouseX;
			ev.y = mouseY;
			ev.modifiers = mods;
			host->SendMouseWheelEvent(ev,
				static_cast<int>(io.MouseWheelH * 120),
				static_cast<int>(io.MouseWheel * 120));
		}
	}

	inline void ForwardKeyboardEvents(CefRefPtr<CefBrowserHost> host,
	                                  bool isFocused)
	{
		if (!isFocused)
		{
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		const uint32_t mods = GetCefModifiers();

		for (int i = 0; i < io.InputQueueCharacters.Size; i++)
		{
			ImWchar ch = io.InputQueueCharacters[i];
			CefKeyEvent ev;
			ev.type = KEYEVENT_CHAR;
			ev.character = static_cast<char16_t>(ch);
			ev.unmodified_character = static_cast<char16_t>(ch);
			ev.windows_key_code = static_cast<int>(ch);
			ev.native_key_code = 0;
			ev.modifiers = mods;
			ev.is_system_key = 0;
			ev.focus_on_editable_field = 1;
			host->SendKeyEvent(ev);
		}

		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
		{
			const int vk = ImGuiKeyToVK(static_cast<ImGuiKey>(key));
			if (vk == 0)
			{
				continue;
			}

			auto makeEvent = [&](cef_key_event_type_t type) {
				CefKeyEvent ev;
				ev.type = type;
				ev.windows_key_code = vk;
				ev.native_key_code = static_cast<int>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
				ev.modifiers = mods;
				ev.is_system_key = 0;
				ev.focus_on_editable_field = 1;
				ev.character = 0;
				ev.unmodified_character = 0;
				return ev;
			};

			if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false))
			{
				host->SendKeyEvent(makeEvent(KEYEVENT_KEYDOWN));
			}

			if (ImGui::IsKeyReleased(static_cast<ImGuiKey>(key)))
			{
				host->SendKeyEvent(makeEvent(KEYEVENT_KEYUP));
			}
		}
	}

} // namespace CefInputBridge
