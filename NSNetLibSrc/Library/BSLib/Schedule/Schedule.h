#pragma once

#include "ScheduleConfig.h"

class Schedule
{
private:
	friend class Scheduler;

	Schedule(ScheduleConfig& config, DWORD index_ = 0);
	~Schedule();

	bool Update(time_t date_stamp, time_t time_stamp, BYTE day);
	void Start();
	void Stop(BYTE day, bool expired, time_t time_stamp);

	DWORD index_;
	bool active_;
	time_t begin_date_stamp_;
	time_t end_date_stamp_;
	time_t start_time_stamp_;
	time_t stop_time_stamp_;
	WORD option_;
	BYTE days_;
	ScheduleCallback start_callback_;
	ScheduleCallback stop_callback_;
	ScheduleCallback expire_callback_;
};
