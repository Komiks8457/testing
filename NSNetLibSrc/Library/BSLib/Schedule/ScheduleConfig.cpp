#include "stdafx.h"
#include "ScheduleConfig.h"

ScheduleConfig::Time::Time() : hour(0), minute(0), sec(0)
{
}

ScheduleConfig::Date::Date() : year(0), month(0), day(1)
{
}

ScheduleConfig::ScheduleConfig() : option(0)
{
}

bool ScheduleConfig::Validate() const
{
	if (begin_date.GetStamp() >= end_date.GetStamp())
		return false;

	if (start_time.GetStamp() >= stop_time.GetStamp())
		return false;

	if (option == 0)
		return false;

	if (option & SCHEDULE_OPTION_CONTINUOUS)
	{
		BYTE days = BYTE(option ^ SCHEDULE_OPTION_CONTINUOUS);

		if (days == 0)
			return false;

		if (start_time.GetStamp() + stop_time.GetStamp() != 86399)
			return false;

		BYTE check_days = 0;

		for (BYTE count = 0; count < 7; ++count)
		{
			BYTE day = BYTE(1 << count);

			if (days & day)
			{
				BYTE prev_day = 0;
				BYTE next_day = 1;

				switch (count)
				{
				case 6:
					{
						prev_day = BYTE(1 << (count - 1));
						next_day = SCHEDULE_OPTION_SUNDAY;
					}
					break;
				case 0:
					{
						prev_day = SCHEDULE_OPTION_SATURDAY;
						next_day = BYTE(1 << (count + 1));
					}
					break;
				default:
					{
						prev_day = BYTE(1 << (count - 1));
						next_day = BYTE(1 << (count + 1));
					}
				}

				if (days & prev_day || days & next_day || days == day)
					check_days |= day;
			}
		}

		if (days != check_days)
			return false;
	}

	return true;
}

time_t ScheduleConfig::Time::GetStamp() const
{
	return (hour * 3600) + (minute * 60) + sec;
}

time_t ScheduleConfig::Date::GetStamp() const
{
	tm t;
	ZeroMemory(&t, sizeof(t));

	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = sec;

	return mktime(&t);
}
