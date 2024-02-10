/*
	Synchronizing threads with a mutex

	Files:
	- main.cpp
	- system.h
	- main-tests.dat
*/
#include "system.h"

// global mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// used to pass arguments to threads
typedef struct {
	int task_num, niter, mtime;
	System * sys_ptr;
	struct timespec * start_time;
} thread_args;

// function prototypes for task threads and monitor thread
void * do_task(void *arg);
void * do_monitor(void *arguments);


/*
	Description: Parses the inputFile into an instance of the System class
	and creates the threads. When the tasks are done, it cancels the 
	monitor threads and prints out the system resources and tasks.
*/
int main (int argc, char *argv[]) {
	if(argc != 4) {
		printf("Incorrect Usage: main inputFile monitorTime NITER\n");
		exit(-1);
	}

	// initialize
	char * inputFile = argv[1];
	int monitorTime = atoi(argv[2]);
	int NITER = atoi(argv[3]);
	int err;
	System sys;
	struct timespec start, end;

	printf("main: (inputFile= '%s', monitorTime= %d, NITER= %d)\n", inputFile, monitorTime, NITER);

	// read entire file and initialize system parameters
	read_system_params(inputFile, &sys);
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	pthread_t tids[sys.num_tasks], tid_monitor;
	thread_args arg_arr[sys.num_tasks], arg_monitor;

	// create arguments to pass to task threads
	for (int i=0; i<sys.num_tasks; i++) {
		arg_arr[i].sys_ptr = &sys;
		arg_arr[i].task_num = i;
		arg_arr[i].start_time = &start;
		arg_arr[i].niter = NITER;
	}
	// for monitor thread
	arg_monitor.mtime = monitorTime;
	arg_monitor.sys_ptr = &sys;

	// lock mutex until all threads are created properly
	pthread_mutex_lock(&mutex);  // (From APUE Ch 11.6.1)

	// create a thread for the monitor
	err = pthread_create(&tid_monitor, NULL, do_monitor, (void *)&arg_monitor);
	if (err != 0) printf("Can't create monitor thread \n");
	
	// create a thread for each task (Error message from APUE Fig. 11.5)
	for (int i=0; i<sys.num_tasks; i++) {
		err = pthread_create(&tids[i], NULL, do_task, (void *)&arg_arr[i]);
		if (err != 0) printf("Can't create a task thread \n");
	}
	pthread_mutex_unlock(&mutex);

	// wait for all tasks (Error message from APUE Fig. 11.5)
	for (int i=0; i<sys.num_tasks; i++) {
		err = pthread_join(tids[i], NULL);
		if (err != 0) printf("Can't join with a thread\n");
	} 

	// tasks are done, cancel monitor (mutex ensures monitor's finished printing)
	pthread_mutex_lock(&mutex);
	pthread_cancel(tid_monitor);
	pthread_mutex_unlock(&mutex);

	// print main thread output
	sys.print_system_resources();
	sys.print_system_tasks();

	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("Running time= %.0f msec\n\n", sys.get_time(&start, &end));

	return 0;
}

/*
	Description: A task repeatedly attempts to acquire all resource units needed 
	by the task, holds the resources for busyTime millisec, releases all held 
	resources, and then enters an idle period of idleTime millisec. Done NITER times.
	Every step that changes the System is protected by locking a mutex.
*/
void * do_task(void *arguments) {
	// deconstruct arguments
	thread_args * args = (thread_args *)arguments;
	int t_num = args->task_num;
	struct timespec * start = args->start_time;
	int niter = args->niter;
	System * sys = args->sys_ptr;
	Task * task = &(sys->tasks[t_num]);

	pthread_t tid = pthread_self();
	task->tid = tid;

	struct timespec end, wait_start, wait_end, delay;
	delay.tv_nsec = 1e7; // 10ms delay for trying again
	clock_gettime(CLOCK_MONOTONIC, &wait_start);
	clock_gettime(CLOCK_MONOTONIC, &wait_end);

    while (task->iter < niter) {
    	// blocks until mutex is available
        pthread_mutex_lock(&mutex);

        // check if every resource is available
        if (sys->all_needed_res_available(task)) {
			// obtain resources
			clock_gettime(CLOCK_MONOTONIC, &wait_end);
			sys->grab_resources(task);
			if (!strcmp(task->status, "WAIT")) {
				// if ending WAIT period, add time spent waiting
				task->time_spent_waiting += sys->get_time(&wait_start, &wait_end);
			}
			strcpy(task->status, "RUN");
			pthread_mutex_unlock(&mutex);
			
			// hold resources and run
			nanosleep(&(task->busy_timespec), NULL);

			// release resources
			pthread_mutex_lock(&mutex);
			sys->release_resources(task);
			
			// idle
			strcpy(task->status, "IDLE");
			pthread_mutex_unlock(&mutex);
			nanosleep(&(task->idle_timespec), NULL);

			// done iteration
			clock_gettime(CLOCK_MONOTONIC, &end);
			pthread_mutex_lock(&mutex);
			task->iter++;
			printf("task: %s (tid= 0x%lx, iter= %d, time= %.0f msec)\n", task->name, task->tid, task->iter, sys->get_time(start, &end));
			pthread_mutex_unlock(&mutex);
        }
        else {
        	// resources are unavailable, try again
        	if (strcmp(task->status, "WAIT")) {
        		// if starting WAIT period, change status and start clock
				strcpy(task->status, "WAIT");
				clock_gettime(CLOCK_MONOTONIC, &wait_start);
				nanosleep(&delay, NULL);
        	}
        	pthread_mutex_unlock(&mutex);
        } 
    }
	pthread_exit((void *) 0);
}

/*
	Description: Prints output every monitorTime millisec and uses a mutex
	so that a task thread can't change its state when the monitor is printing.
*/
void * do_monitor(void *arguments) {
	thread_args * args = (thread_args *)arguments;
	System * sys = args->sys_ptr;
	struct timespec monitor_timespec;
	double time_sec;
	int time_int;

	// put times into timespec structs: convert ms into {s, ns}
	time_sec = (double)(args->mtime)/1000;
	time_int = (int) time_sec;
	monitor_timespec.tv_sec = time_int;
	monitor_timespec.tv_nsec = (int) ((time_sec - time_int) * 1e9);

	// print task status until this thread is cancelled
	while (true) {
		pthread_mutex_lock(&mutex);
		sys->print_monitor();
		pthread_mutex_unlock(&mutex);
		// print every monitorTime interval
		nanosleep(&monitor_timespec, NULL);
	}
}
