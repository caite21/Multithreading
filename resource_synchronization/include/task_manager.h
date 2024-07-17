#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "common.h"
#include "task.h"


class TaskManager {
	public: 
		vector<Task> tasks;
		
		int getNumTasks();
		int parseInput(const char *inputFile);
		double get_time(struct timespec * start, struct timespec * end);
		bool resourcesAreAvailable(Task &task);
		void grabResources(Task &task);
		void releaseResources(Task &task);
		void printMonitor();
		void printTasks();
		void printResources();
		
	private:
		int numTasks = 0;
		unordered_map<string, int> resourcesAvailableDict;
		unordered_map<string, int> maxResourcesDict;
};

#endif
