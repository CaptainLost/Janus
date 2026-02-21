#pragma once

#include <string>

namespace StringUtils
{
	inline std::string Truncate(const std::string& text, size_t maxLength)
	{
		if (text.size() <= maxLength)
		{
			return text;
		}

		return text.substr(0, maxLength - 3) + "...";
	}
}
