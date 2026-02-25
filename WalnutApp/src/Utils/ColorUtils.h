#pragma once

#include "imgui.h"

#include <cstdint>

namespace ColorUtils
{
	inline int PackColor(ImColor color)
	{
		auto r = static_cast<uint8_t>(color.Value.x * 255.0f);
		auto g = static_cast<uint8_t>(color.Value.y * 255.0f);
		auto b = static_cast<uint8_t>(color.Value.z * 255.0f);
		auto a = static_cast<uint8_t>(color.Value.w * 255.0f);

		return (r << 24) | (g << 16) | (b << 8) | a;
	}

	inline ImColor UnpackColor(int packed)
	{
		float r = ((packed >> 24) & 0xFF) / 255.0f;
		float g = ((packed >> 16) & 0xFF) / 255.0f;
		float b = ((packed >> 8)  & 0xFF) / 255.0f;
		float a = (packed & 0xFF) / 255.0f;

		return ImColor(r, g, b, a);
	}
}
