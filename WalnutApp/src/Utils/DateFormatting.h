#pragma once

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

namespace DateFormatting
{
	constexpr const char* kDays[] = {
		"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
	};

	constexpr const char* kMonths[] = {
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"
	};

	inline std::string GetDayLabel(int64_t timestamp)
	{
		time_t entryTimestamp = static_cast<time_t>(timestamp);
		tm entryTime = {};
		localtime_s(&entryTime, &entryTimestamp);

		time_t now = time(nullptr);
		tm nowTime = {};
		localtime_s(&nowTime, &now);

		if (entryTime.tm_year == nowTime.tm_year && entryTime.tm_yday == nowTime.tm_yday)
		{
			return "Today";
		}

		tm yesterdayTime = nowTime;
		yesterdayTime.tm_mday--;
		mktime(&yesterdayTime);

		if (entryTime.tm_year == yesterdayTime.tm_year && entryTime.tm_yday == yesterdayTime.tm_yday)
		{
			return "Yesterday";
		}

		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%s, %s %d, %d",
			kDays[entryTime.tm_wday],
			kMonths[entryTime.tm_mon],
			entryTime.tm_mday,
			entryTime.tm_year + 1900);
		return buffer;
	}
}
