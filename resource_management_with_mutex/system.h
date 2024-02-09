#include <iostream>
#include <string.h> // for strcmp, strcpy
#include <pthread.h> // for threads

// assumptions:
#define NRES_TYPES 10  // max num resource types
#define NTASKS 25  // max num tasks
#define MAXWORD 32 // max chars in an object name
#define MAXLINE 100 // Max # of characters in an input file line


/*
	Description: Holds all necessary information about each task.
*/
struct Task {
	char name[MAXWORD] = {'\0'};
	pthread_t tid;
	int iter = 0;
	int time_spent_waiting = 0;
	
	int busy_time = 0;
	int idle_time = 0;
	struct timespec busy_timespec = {0, 0};
	struct timespec idle_timespec = {0, 0};

	int num_res_needed = 0;
	char res_needed_names[NRES_TYPES][MAXWORD] = {'\0'};
	int res_needed_amounts[NRES_TYPES] = {0};
	int res_holding_amounts[NRES_TYPES] = {0};

	char status[5] = {'\0'};  // WAIT, IDLE, or RUN
};

/*
	Description: Holds all information about every thread and resource.
	Has methods that handle printing information, checking resource
	availability, and holding and releasing resources.
*/
class System {
public: 
	// resource information
	int num_res = 0;
	char res_names[NRES_TYPES][MAXWORD] = {'\0'};
	int res_amounts[NRES_TYPES] = {0};
	int res_held[NRES_TYPES] = {0};
	
	// task information
	int num_tasks = 0;
	char task_names[NTASKS][MAXWORD] = {'\0'};
	Task tasks[NTASKS];
	char tasks_waiting[NTASKS][MAXWORD] = {'\0'};
	char tasks_running[NTASKS][MAXWORD] = {'\0'};
	char tasks_idling[NTASKS][MAXWORD] = {'\0'};

	/* Get the elapsed time in milliseconds */
	double get_time(struct timespec * start, struct timespec * end) {
		double time = (end->tv_sec - start->tv_sec + 
			(end->tv_nsec - start->tv_nsec)/1E9) * 1000;
		return time; // in milliseconds
	}

	/* Find and return the index of a resource in the list of resources */
	int get_resource_index(char r_name[MAXWORD]) {
		// find the index of the resource name
		for (int i=0; i<num_res; i++) {
			if (!strcmp(r_name, res_names[i])) {
				return i;
			}
		}
		return -100; // error
	}

	/* Return how many units of a resource are available */
	int how_many_res_avail(char r_name[MAXWORD]) {
		int amount;
		int held;
		// find the index of the resource name
		int i = get_resource_index(r_name);
		amount = res_amounts[i];
		held = res_held[i];
		// return how many are available
		return (amount - held);
	}

	/* Check if every resource needed by the task is available */
	bool all_needed_res_available(Task * task) {
		int got_resources = 0;

        for (int i=0; i<task->num_res_needed; i++) {
			if (how_many_res_avail(task->res_needed_names[i]) >= task->res_needed_amounts[i]) {
				got_resources++;
			}
        }
        if (got_resources == task->num_res_needed) {
        	return true;
        }
        return false;
	}

	/* Hold every resource needed by the task */
	void grab_resources(Task * task) {
		int ind;
		for (int i=0; i<task->num_res_needed; i++) {
			// increment res_held by res_needed_amount at the corresponding resource
			task->res_holding_amounts[i] += task->res_needed_amounts[i];
			// do same in sys  
			ind = get_resource_index(task->res_needed_names[i]);
			res_held[ind] += task->res_needed_amounts[i];
		}
	}

	/* Release every resource needed by the task */
	void release_resources(Task * task) {
		int ind;
		for (int i=0; i<task->num_res_needed; i++) {
			// decrement res_held by res_needed_amount at the corresponding resource
			task->res_holding_amounts[i] -= task->res_needed_amounts[i];
			// do same in sys  
			ind = get_resource_index(task->res_needed_names[i]);
			res_held[ind] -= task->res_needed_amounts[i];
		}
	}

	/* Updates task status lists */
	void collect_task_status() {
		int w_index = 0;
		int r_index = 0;
		int i_index = 0;
		memset(tasks_waiting, '\0', sizeof(tasks_waiting));
		memset(tasks_running, '\0', sizeof(tasks_running));
		memset(tasks_idling, '\0', sizeof(tasks_idling));

		for (Task t : tasks) {
			if (!strcmp(t.status, "WAIT")) {
				strcpy(tasks_waiting[w_index], t.name);
				w_index++;
			}
			else if (!strcmp(t.status, "RUN")) {
				strcpy(tasks_running[r_index], t.name);
				r_index++;
			}
			else if (!strcmp(t.status, "IDLE")) {
				strcpy(tasks_idling[i_index], t.name);
				i_index++;
			}
		}
	}

	/* Prints monitor's output */
	void print_monitor() {
		collect_task_status();

		printf("\nmonitor: [WAIT] ");
		for (int i=0; i<num_tasks; i++) {
			if (strcmp(tasks_waiting[i], "\0")) printf("%s ", tasks_waiting[i]);
		}
		printf("\n\t [RUN]  ");
		for (int i=0; i<num_tasks; i++) {
			if (strcmp(tasks_running[i], "\0")) printf("%s ", tasks_running[i]);
		}
		printf("\n\t [IDLE] ");
		for (int i=0; i<num_tasks; i++) {
			if (strcmp(tasks_idling[i], "\0")) printf("%s ", tasks_idling[i]);
		}
		printf("\n\n");
	}

	/* Prints System Tasks */
	void print_system_tasks() {
		printf("System Tasks:\n");
		for (int i=0; i<num_tasks; i++) {
			printf("[%d] %s (%s, runTime= %d msec, idleTime= %d msec):\n", i, tasks[i].name, tasks[i].status, tasks[i].busy_time, tasks[i].idle_time);
			printf("\t (tid= 0x%lx)\n", tasks[i].tid);
			for (int j=0; j<tasks[i].num_res_needed; j++) {
				printf("\t %s: (needed= %d, held= %d)\n", tasks[i].res_needed_names[j], tasks[i].res_needed_amounts[j], tasks[i].res_holding_amounts[j]);
			}
			printf("\t (RUN: %d times, WAIT %d msec)\n\n", tasks[i].iter,  tasks[i].time_spent_waiting);	
		}
		printf("\n");
	}

	/* Prints System Resources */
	void print_system_resources() {
		printf("\nSystem Resources:\n");
		for (int i=0; i<num_res; i++) {
			printf("\t %s: (maxAvail= %d, held= %d)\n", res_names[i], how_many_res_avail(res_names[i]), res_held[i]);
		}
		printf("\n");
	}
};

/* Parse a resource line into a System instance */
void parse_resource(int count, char **tokens, System * sys) {
	char * name;
	int amount;

	// add resource to sys
	for (int i=1; i<count; i+=2) {
		name = tokens[i];
		amount = atoi(tokens[i+1]);
		strcpy(sys->res_names[sys->num_res], name);
		sys->res_amounts[sys->num_res] = amount;
		sys->num_res++;
	}
}

/* Parse a task line into Task instance and store it in a System instance */
void parse_task(int count, char **tokens, System * sys) {
	Task task;
	char * name;
	int amount, time_int;
	double time_sec;

	// create task
	strcpy(task.name, tokens[1]);  // taskName
	task.busy_time = atoi(tokens[2]);  // busyTime
	task.idle_time = atoi(tokens[3]);  // idleTime

	// put times in timespec structs: convert ms into {s, ns}
	time_sec = (double)(task.busy_time)/1000;
	time_int = (int) time_sec;
	task.busy_timespec.tv_sec = time_int;
	task.busy_timespec.tv_nsec = (int) ((time_sec - time_int) * 1e9);
	// same with idleTime
	time_sec = (double)(task.idle_time)/1000;
	time_int = (int) time_sec;
	task.idle_timespec.tv_sec = time_int;
	task.idle_timespec.tv_nsec = (int) ((time_sec - time_int) * 1e9);

	// add name:value pairs to sys
	for (int i=4; i<count; i+=2) {
		name = tokens[i];
		amount = atoi(tokens[i+1]);

		strcpy(task.res_needed_names[task.num_res_needed], name);
		task.res_needed_amounts[task.num_res_needed] = amount;
		task.num_res_needed++;
	}

	// add to sys
	strcpy(sys->task_names[sys->num_tasks], tokens[1]);
	sys->tasks[sys->num_tasks] = task;
	sys->num_tasks++;
}

/* Read and parse the inputFile into a System instance */
int read_system_params(char * inputFile, System * sys) {
	char inStr[MAXLINE];
	FILE * fp;

	// open file
	fp = fopen(inputFile, "r");
	if(fp == NULL) {
		printf("Opening inputFile failed.\n");
		return -1;
	}

	// read 1 line from inputFile
	while(fgets(inStr,MAXLINE,fp) != NULL) {
		// get rid of trailing newline
		if (inStr[strlen(inStr)-1] == '\n') inStr[strlen(inStr)-1] = '\0';
		// skip comments and empty lines
		if (inStr[0] == '#' || inStr[0] == '\0') {}

		else {
			// is a command
			char WSPACECOLON[]= "\n \t:";
			char * token;
			int count=0;
			int NUM_TOKENS = NRES_TYPES*2 + 1 + 3;  // 2 tokens for each resource, tokens for type,name,idle,run

			// tokenize string
			token = strtok(inStr, WSPACECOLON);
			char **tokens = new char*[NUM_TOKENS];
			while (token != NULL) {
				tokens[count] = token;
				count++;
				token = strtok(NULL, WSPACECOLON);
			}
			// parse line into sys object
			if (!strcmp(tokens[0], "resources")) {
				parse_resource(count, tokens, sys);
			}
			else if (!strcmp(tokens[0], "task")) {
				parse_task(count, tokens, sys);
			}
			else {
				printf("Unexpected line in inputFile. Exitting.\n");
				exit(-1);
			}
			delete[] tokens;
		}
	}
	fclose(fp);
	return 0;
}

