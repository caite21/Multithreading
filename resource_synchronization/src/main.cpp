/*
	Synchronizing threads with a mutex

	Files:
	- main.cpp
	- system.h
	- main-tests.dat
*/

#include "../include/system.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
System sys;
struct timespec start;
int monitorTime;
int nIter;

// function prototypes for task threads and monitor thread
void *do_task(void *arguments);
void *do_monitor(void *arguments);


/*
	Description: Parses the inputFile into an instance of the System class
	and creates the threads. When the tasks are done, it cancels the 
	monitor threads and prints out the system resources and tasks.
*/
int main (int argc, char *argv[]) {
	if(argc != 4) {
		cerr << "Incorrect Usage: main inputFile monitorTime NITER" << endl;
		return EXIT_FAILURE;
	}

	const char *inputFile = argv[1];
	monitorTime = atoi(argv[2]);
	nIter = atoi(argv[3]);
	cout << "main: inputFile=" << inputFile << ", monitorTime=" << monitorTime << ", nIter=" << nIter << endl;

	// Read system parameters from input file
	read_system_params(inputFile, &sys);
	clock_gettime(CLOCK_MONOTONIC, &start);	
	struct timespec end;
	pthread_t tidMonitor;
	pthread_t tids[sys.numTasks];
	int threadNums[sys.numTasks];

	for (int i = 0; i < sys.numTasks; i++) {
		threadNums[i] = i;
	}

	// Lock until all threads are created properly
	pthread_mutex_lock(&mutex); 

	// Create monitor thread
	int err = pthread_create(&tidMonitor, nullptr, do_monitor, nullptr);
	if (err != 0) {
		cerr << "Error creating monitor thread" << endl;
		exit(EXIT_FAILURE);
	}
	
	// Create task threads
	for (int i = 0; i < sys.numTasks; i++) {
		int err = pthread_create(&tids[i], nullptr, do_task, (void *)&threadNums[i]);
		if (err != 0) {
			cerr << "Error creating task thread" << endl;
			exit(EXIT_FAILURE);
		}
	}

	pthread_mutex_unlock(&mutex);

	// Join task threads
	for (int i = 0; i < sys.numTasks; i++) {
		int err = pthread_join(tids[i], nullptr);
		if (err != 0) {
			cerr << "Error joiining task thread" << endl;
			exit(EXIT_FAILURE);
		}
	} 

	// Cancel monitor when it's done printing and when tasks threads are complete
	pthread_mutex_lock(&mutex);
	pthread_cancel(tidMonitor);
	pthread_mutex_unlock(&mutex);

	// Print system
	sys.print_system_resources();
	sys.print_system_tasks();

	// Print total running time
	clock_gettime(CLOCK_MONOTONIC, &end);
	cout << "Running time= " << sys.get_time(&start, &end) << " ms" << endl;

	return 0;
}

/*
	Description: A task repeatedly attempts to acquire all resource units needed 
	by the task, holds the resources for busyTime millisec, releases all held 
	resources, and then enters an idle period of idleTime millisec. Done NITER times.
	Every step that changes the System is protected by locking a mutex.
*/
void *do_task(void *arguments) {
	// deconstruct arguments
	int t_num = *((int*) arguments);
	Task &task = sys.tasks[t_num];
	task.tid = pthread_self();

	struct timespec end, wait_start, wait_end, delay;
	delay.tv_nsec = 1e7; // 10ms delay for trying again
	clock_gettime(CLOCK_MONOTONIC, &wait_start);
	clock_gettime(CLOCK_MONOTONIC, &wait_end);

    while (task.iter < nIter) {
        pthread_mutex_lock(&mutex);

        if (sys.resourcesAvailable(task)) {
			if (task.status == "WAIT") {
				// Wait period done; add time spent waiting
				clock_gettime(CLOCK_MONOTONIC, &wait_end);
				task.time_spent_waiting += sys.get_time(&wait_start, &wait_end);
			}
			// Simulate running task; hold necessary resources for busyTime  
			sys.grab_resources(task);
			task.status = "RUN";
			pthread_mutex_unlock(&mutex);
			nanosleep(&(task.busy_timespec), NULL);

			// Simulate idle task; release resources for idleTime
			pthread_mutex_lock(&mutex);
			sys.release_resources(task);
			task.status = "IDLE";
			pthread_mutex_unlock(&mutex);
			nanosleep(&(task.idle_timespec), NULL);

			// Iteration complete
			clock_gettime(CLOCK_MONOTONIC, &end);
			pthread_mutex_lock(&mutex);
			task.iter++;
			cout << "task complete: " << task.name << " (iter= " << task.iter << ", time= " << sys.get_time(&start, &end) << " ms)" << endl;
			pthread_mutex_unlock(&mutex);
        }
        else {
        	if (task.status != "WAIT") {
        		// Start wait period
				task.status = "WAIT";
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
void *do_monitor(void *arguments) {
	struct timespec monitor_timespec;
	double time_sec;
	int time_int;

	// put times into timespec structs: convert ms into {s, ns}
	time_sec = (double)(monitorTime)/1000;
	time_int = (int) time_sec;
	monitor_timespec.tv_sec = time_int;
	monitor_timespec.tv_nsec = (int) ((time_sec - time_int) * 1e9);

	// print task status until this thread is cancelled
	while (true) {
		pthread_mutex_lock(&mutex);
		sys.print_monitor();
		pthread_mutex_unlock(&mutex);
		// print every monitorTime interval
		nanosleep(&monitor_timespec, NULL);
	}
}
