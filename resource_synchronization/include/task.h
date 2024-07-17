#ifndef TASK_H
#define TASK_H

#include "common.h"


// Represents a single task in the system
struct Task {
    string name;
    string status;
    
    pthread_t tid;
    int iter = 0;
    int busyTime = 0;
    int idleTime = 0;
    int timeSpentWaiting = 0;
    struct timespec busy_timespec = {0, 0};
    struct timespec idle_timespec = {0, 0};
    
    unordered_map<string, int> resourcesNeededDict;
    unordered_map<string, int> holdingDict;
};

#endif