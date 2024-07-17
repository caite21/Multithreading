#include "../include/task_manager.h"

/* Get number of tasks */
int TaskManager::getNumTasks() {
	return numTasks;
}

/* Get the duration of start to end in milliseconds */
double TaskManager::getDuration(struct timespec &start, struct timespec &end) {
	double time = (end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec)/1E9) * 1000;
	return time; // in milliseconds
}

/* Get the elapsed time in milliseconds */
void TaskManager::setTimespec(int ms, struct timespec &delayTimespec) {
	// put times into timespec structs: convert ms into {s, ns}
	double timeSeconds = (double) (ms)/1000;
	int timeInt = (int) timeSeconds;
	delayTimespec.tv_sec = timeInt;
	delayTimespec.tv_nsec = (int) ((timeSeconds - timeInt) * 1e9);
	return;
}

/* Check if all resources needed by the task are available */
bool TaskManager::resourcesAreAvailable(Task &task) {
	for (const auto& pair : task.resourcesNeededDict) {
		string resource = pair.first;
		int amountNeeded = pair.second;
		if (resourcesAvailableDict[resource] < amountNeeded) {
			return false;
		}
	}
	return true;
}

/* Take and hold every resource needed by the task */
void TaskManager::grabResources(Task &task) {
	for (const auto& pair : task.resourcesNeededDict) {
		string resource = pair.first;
		int amount = pair.second;
		resourcesAvailableDict[resource] -= amount;
		task.holdingDict[resource] += amount;
	}
}

/* Release every resource needed by the task */
void TaskManager::releaseResources(Task &task) {
	for (const auto& pair : task.resourcesNeededDict) {
		string resource = pair.first;
		int amount = pair.second;
		resourcesAvailableDict[resource] += amount;
		task.holdingDict[resource] -= amount;
	}
}

/* Prints all tasks and their status */
void TaskManager::printMonitor() {
	cout << "\nmonitor: [WAIT] ";
	for (Task t : tasks) {
		if (t.status == "WAIT") {
			cout << t.name << " ";
		}
	}
	cout << "\n\t [RUN]  ";
	for (Task t : tasks) {
		if (t.status == "RUN") {
			cout << t.name << " ";
		}
	}
	cout << "\n\t [IDLE] ";
	for (Task t : tasks) {
		if (t.status == "IDLE") {
			cout << t.name << " ";
		}
	}
	cout << endl << endl;

}

/* Prints details of every task */
void TaskManager::printTasks() {
    cout << "All Tasks:" << endl;
	int i = 0;
    for (Task t : tasks) {
        cout << "[" << i << "] " << t.name << " (" << t.status << ", runTime= " << t.busyTime << " ms, idleTime= " << t.idleTime << " ms):" << endl;
        cout << "\t(tid= 0x" << hex << t.tid << dec << ")" << endl;
		for (const auto& pair : t.resourcesNeededDict) {
			string resource = pair.first;
			int amount = pair.second;
			cout << "\t" << resource << ":\t(need= " << amount << ", holding= " << t.holdingDict[resource] << ")" << endl;
		}
        cout << "\t(Ran: " << t.iter << " times, Waited: " << t.timeSpentWaiting << " ms)" << endl << endl;
		i++;
	}
    cout << endl;
}

/* Prints details of every resource */
void TaskManager::printResources() {
    cout << "\nAll Resources:" << endl;
	for (const auto& pair : resourcesAvailableDict) {
		string resource = pair.first;
		int amount = pair.second;
		cout << "\t" << resource << ":\t(maxAvail= " << maxResourcesDict[resource] << ", held= " << (maxResourcesDict[resource] - amount) << ")" << endl;
    }
    cout << endl;
}

/* Read and parse the inputFile into a TaskManager instance */
int TaskManager::parseInput(const char *inputFile) {
	ifstream file(inputFile);
	if (!file.is_open()) {
		cerr << "Failed to open file: " << inputFile << endl;
		return EXIT_FAILURE;
	}

	string line;
	try {
		while(getline(file, line)) {
			// Skip comments and empty lines
			if (line.empty() || line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
				continue; 
			}
			// Remove trailing newline
			if (line.back() == '\n' || line.back() == '\r') {
				line.pop_back();
			}

			// Tokenize string
			istringstream iss(line);
			vector<string> tokens;
			string token;
			while(getline(iss, token, ' ')) {
				tokens.push_back(token);
			}
			if (tokens[0] == "resources") {
				// Add available resource to TaskManager 
				for (uint i = 1; i < tokens.size(); i++) {
					string resource = tokens[i].substr(0, tokens[i].find(':'));
					string amount = tokens[i].substr(tokens[i].find(':') + 1);
					this->resourcesAvailableDict[resource] = stoi(amount);
					this->maxResourcesDict[resource] = stoi(amount);
				}
			} 
			else if (tokens[0] == "task") {
				Task task;
				task.name = tokens[1];
				task.busyTime = stoi(tokens[2]); 
				task.idleTime = stoi(tokens[3]); 
				setTimespec(task.busyTime, task.busyTimespec);
				setTimespec(task.idleTime, task.idleTimespec);

				// Add needed resource to task
				for (uint i = 4; i < tokens.size(); i++) {
					string resource = tokens[i].substr(0, tokens[i].find(':'));
					string amount = tokens[i].substr(tokens[i].find(':') + 1);
					task.resourcesNeededDict[resource] = stoi(amount);
					task.holdingDict[resource] = 0;
				}
				this->tasks.push_back(task);
				this->numTasks++;
			} 
			else {
				cerr << "Skipping unexpected line in " << inputFile << ": " << line << endl;
			}
		}
	}
	catch(...) {
		cerr << "Unexpected line format in " << inputFile << ": " << line << endl;
		return EXIT_FAILURE;
	}
	
	file.close();
	return 0;
}
