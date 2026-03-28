#include "stdafx.h"
#include "Scheduler.h"
#include "Schedule.h"

Scheduler::Scheduler() : update_delay_skip_(false)
{
}

Scheduler::~Scheduler()
{
	for each (Schedule* schedule in schedules_)
		delete schedule;

	schedules_.clear();
}

bool Scheduler::Register(ScheduleConfig& config, DWORD index)
{
	if (config.Validate() == false)
		return false;

	schedules_.push_back(new Schedule(config, index));
	return true;
}

void Scheduler::UnRegister(DWORD index)
{
	for each (Schedule* current_schedule in schedules_)
	{
		if (current_schedule->index_ != index)
			continue;

		current_schedule->Stop(0, true, 0);
		schedules_.remove(current_schedule);
		return;
	}
}

void Scheduler::Update()
{
	if (schedules_.empty() == true)
		return;

	static time_t date_stamp = 0;
	static time_t prev_date_stamp = 0;
	static time_t update_delay_time = 0;
	
	if (update_delay_time > 0)
	{
		if (update_delay_skip_)
		{
			prev_date_stamp += update_delay_time;
			update_delay_time = 0;
		}
		else
		{
			++prev_date_stamp;
			--update_delay_time;
		}
		
		date_stamp = prev_date_stamp;
	}
	else
	{
		time(&date_stamp);

		if (prev_date_stamp > 0)
		{
			time_t delta_delay_time = date_stamp - prev_date_stamp;		
			if (delta_delay_time > 1)
			{
				update_delay_time += delta_delay_time;
				return;
			}
		}

		prev_date_stamp = date_stamp;
	}

	static tm date;
	if (localtime_s(&date, &date_stamp) != 0)
		return;

	static time_t time_stamp = 0;
	time_stamp = (date.tm_hour * 3600) + (date.tm_min * 60) + date.tm_sec;

	Schedule* schedule = nullptr;
	for (Schedules::iterator itor = schedules_.begin(); itor != schedules_.end();)
	{
		schedule = *itor;

		if (schedule->Update(date_stamp, time_stamp, BYTE(1 << date.tm_wday)))
			++itor;
		else
		{
			delete schedule;
			itor = schedules_.erase(itor);
		}
	}
}