#pragma once

typedef std::function<void()> ScheduleCallback;

#define SCHEDULE_OPTION_SUNDAY WORD(0x0001)
#define SCHEDULE_OPTION_MONDAY WORD(0x0002)
#define SCHEDULE_OPTION_TUESDAY WORD(0x0004)
#define SCHEDULE_OPTION_WEDNESDAY WORD(0x0008)
#define SCHEDULE_OPTION_THURSDAY WORD(0x0010)
#define SCHEDULE_OPTION_FRIDAY	WORD(0x0020)
#define SCHEDULE_OPTION_SATURDAY WORD(0x0040)
#define SCHEDULE_OPTION_WEEK WORD(SCHEDULE_OPTION_SUNDAY | SCHEDULE_OPTION_MONDAY | SCHEDULE_OPTION_TUESDAY | SCHEDULE_OPTION_WEDNESDAY | SCHEDULE_OPTION_THURSDAY | SCHEDULE_OPTION_FRIDAY | SCHEDULE_OPTION_SATURDAY)
#define SCHEDULE_OPTION_CONTINUOUS WORD(0x0100)

struct ScheduleConfig
{
	struct Time
	{
		BYTE hour;
		BYTE minute;
		BYTE sec;

		Time();

		virtual time_t GetStamp() const;
	};

	struct Date : Time
	{
		WORD year;
		BYTE month;
		BYTE day;

		Date();

		time_t GetStamp() const override;
	};

	Date begin_date;
	Date end_date;
	Time start_time;
	Time stop_time;
	WORD option;
	ScheduleCallback start_callback;
	ScheduleCallback stop_callback;
	ScheduleCallback expire_callback;

	ScheduleConfig();

	bool Validate() const;
};