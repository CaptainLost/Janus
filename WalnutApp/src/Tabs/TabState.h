#pragma once

#include <cstdint>

enum class TabState : uint8_t
{
	Blank,
	Loading,
	Ready,
	Error
};
