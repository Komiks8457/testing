#pragma once

#include "ScheduleConfig.h"

class Schedule;
class Scheduler
{
public:
	Scheduler();
	~Scheduler();

	bool Register(ScheduleConfig& config, DWORD index = 0);
	void UnRegister(DWORD config);
	void Update();

	void set_update_delay_skip(bool update_delay_skip) { update_delay_skip_ = update_delay_skip; }

private:
	typedef std::list<Schedule*> Schedules;

	Schedules schedules_;
	bool update_delay_skip_;
};