#include "../include/task_manager.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
TaskManager manager;
struct timespec start;
int monitorTime;
int nIter;

// Function prototypes for task threads and monitor thread
void *doTask(void *taskNum);
void *doMonitor(void *_);


/*
	Parses the inputFile into a TaskManager. Creates monitor thread and
	task threads. Prints out the TaskManager details at end.
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

	// Read resources and tasks from input file
	manager.parseInput(inputFile);
	clock_gettime(CLOCK_MONOTONIC, &start);	
	struct timespec end;
	pthread_t tidMonitor;
	pthread_t tids[manager.getNumTasks()];
	int taskNums[manager.getNumTasks()];

	for (int i = 0; i < manager.getNumTasks(); i++) {
		taskNums[i] = i;
	}

	// Lock until monitor thread and task threads are created
	pthread_mutex_lock(&mutex); 
	int err = pthread_create(&tidMonitor, nullptr, doMonitor, nullptr);
	if (err != 0) {
		cerr << "Error creating monitor thread" << endl;
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < manager.getNumTasks(); i++) {
		int err = pthread_create(&tids[i], nullptr, doTask, (void *)&taskNums[i]);
		if (err != 0) {
			cerr << "Error creating task thread" << endl;
			exit(EXIT_FAILURE);
		}
	}
	pthread_mutex_unlock(&mutex);

	// Join task threads
	for (int i = 0; i < manager.getNumTasks(); i++) {
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

	// Print TaskManager details
	manager.printResources();
	manager.printTasks();

	// Print total running time
	clock_gettime(CLOCK_MONOTONIC, &end);
	cout << "Running time= " << manager.get_time(&start, &end) << " ms" << endl;

	return 0;
}

/*
	A task thread repeatedly attempts to acquire all resource units needed 
	by the task, holds the resources for busyTime millisec, releases all held 
	resources, and then enters an idle period of idleTime millisec. Done nIter times.
	Changes to the TaskManager are protected by locking a mutex.
*/
void *doTask(void *taskNum) {
	Task &task = manager.tasks[*((int*) taskNum)];
	task.tid = pthread_self();

	struct timespec end, wait_start, wait_end, delay;
	delay.tv_nsec = 1e7; // 10ms delay for trying again
	clock_gettime(CLOCK_MONOTONIC, &wait_start);
	clock_gettime(CLOCK_MONOTONIC, &wait_end);

    while (task.iter < nIter) {
        pthread_mutex_lock(&mutex);
        if (manager.resourcesAreAvailable(task)) {
			if (task.status == "WAIT") {
				// Wait period done; add time spent waiting
				clock_gettime(CLOCK_MONOTONIC, &wait_end);
				task.timeSpentWaiting += manager.get_time(&wait_start, &wait_end);
			}
			
			// Simulate running task; hold necessary resources for busyTime  
			manager.grabResources(task);
			task.status = "RUN";
			pthread_mutex_unlock(&mutex);
			nanosleep(&(task.busy_timespec), NULL);

			// Simulate idle task; release resources for idleTime
			pthread_mutex_lock(&mutex);
			manager.releaseResources(task);
			task.status = "IDLE";
			pthread_mutex_unlock(&mutex);
			nanosleep(&(task.idle_timespec), NULL);

			// Iteration complete
			clock_gettime(CLOCK_MONOTONIC, &end);
			pthread_mutex_lock(&mutex);
			task.iter++;
			cout << "task complete: " << task.name << " (iter= " << task.iter << ", time= " << manager.get_time(&start, &end) << " ms)" << endl;
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
	Prints TaskManager details every monitorTime milliseconds so that 
	the tasks can be monitored from standard output.
*/
void *doMonitor(void *_) {
	struct timespec monitor_timespec;
	double time_sec;
	int time_int;

	// put times into timespec structs: convert ms into {s, ns}
	time_sec = (double)(monitorTime)/1000;
	time_int = (int) time_sec;
	monitor_timespec.tv_sec = time_int;
	monitor_timespec.tv_nsec = (int) ((time_sec - time_int) * 1e9);

	// Continuously print until thread is cancelled
	while (true) {
		// Lock so task threads can't change states while the monitor is printing
		pthread_mutex_lock(&mutex);
		manager.printMonitor();
		pthread_mutex_unlock(&mutex);
		// Print every monitorTime interval
		nanosleep(&monitor_timespec, NULL);
	}
}
