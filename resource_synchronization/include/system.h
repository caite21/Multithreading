#ifndef SYSTEM_H
#define SYSTEM_H

#include "common.h"
#include "task.h"


class System {
public: 
	int num_res = 0;
    vector<string> res_names;
	int res_amounts[NRES_TYPES] = {0};
	int res_held[NRES_TYPES] = {0};
	int numTasks = 0;
	vector<string> task_names;
	Task tasks[NTASKS];
    
	double get_time(struct timespec * start, struct timespec * end);
	int get_resource_index(string r_name);
	int how_many_res_avail(string r_name);
	bool resourcesAvailable(Task &task);
	void grab_resources(Task &task);
	void release_resources(Task &task);
	void print_monitor();
	void print_system_tasks();
	void print_system_resources();
};

void parse_resource(int count, char **tokens, System * sys);
void parse_task(int count, char **tokens, System * sys);
int read_system_params(const char * inputFile, System * sys);

#endif
