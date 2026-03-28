#include "stdafx.h"
#include "Schedule.h"

Schedule::Schedule(ScheduleConfig& config, DWORD index) : active_(false),
	begin_date_stamp_(config.begin_date.GetStamp()), end_date_stamp_(config.end_date.GetStamp()),
	start_time_stamp_(config.start_time.GetStamp()), stop_time_stamp_(config.stop_time.GetStamp()), option_(config.option), days_(0),
	start_callback_(config.start_callback), stop_callback_(config.stop_callback), expire_callback_(config.expire_callback)
	, index_(index)
{
}

Schedule::~Schedule()
{
}

bool Schedule::Update(time_t date_stamp, time_t time_stamp, BYTE day)
{
	if (date_stamp < begin_date_stamp_)
		return true;

	bool expired = (date_stamp > end_date_stamp_);

	if (expired == false && (option_ & day))
	{
		if (time_stamp >= start_time_stamp_ && time_stamp < stop_time_stamp_)
		{
			if (active_ == false)
				Start();

			days_ |= day;
			return true;
		}
	}

	if (active_ == true)
		Stop(day, expired, time_stamp);

	if (expired == true)
	{
		if (expire_callback_ != nullptr)
			expire_callback_();

		return false;
	}

	return true;
}

void Schedule::Start()
{
	if (option_ & SCHEDULE_OPTION_CONTINUOUS && days_ != 0)
		return;

	active_ = true;

	if (start_callback_ != nullptr)
		start_callback_();
}

void Schedule::Stop(BYTE day, bool expired, time_t time_stamp)
{
	if (expired)
		goto STOP;

	if (option_ & SCHEDULE_OPTION_CONTINUOUS)
	{
		if ((days_ & day && (days_ != (option_ & SCHEDULE_OPTION_WEEK))))
			return;

		goto STOP;
	}

	if (time_stamp >= start_time_stamp_ && time_stamp < stop_time_stamp_)
		return;

STOP:
	days_ = 0;
	active_ = false;

	if (stop_callback_ != nullptr)
		stop_callback_();
}