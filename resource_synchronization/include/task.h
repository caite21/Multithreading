
#ifndef TASK_H
#define TASK_H

#include "common.h"


// Represents a single task in the system
class Task {
public:
	string name;
	pthread_t tid;
	string status;
	int iter = 0;
    
	int busy_time = 0;
	int idle_time = 0;
	struct timespec busy_timespec = {0, 0};
	struct timespec idle_timespec = {0, 0};
	int time_spent_waiting = 0;

	int num_res_needed = 0;
    vector<string> res_needed_names;
	int res_needed_amounts[NRES_TYPES] = {0};
	int res_holding_amounts[NRES_TYPES] = {0};
};

#endif