#include "../include/system.h"


/* Get the elapsed time in milliseconds */
double System::get_time(struct timespec * start, struct timespec * end) {
	double time = (end->tv_sec - start->tv_sec + 
		(end->tv_nsec - start->tv_nsec)/1E9) * 1000;
	return time; // in milliseconds
}

/* Find and return the index of a resource in the list of resources */
int System::get_resource_index(string r_name) {
	// find the index of the resource name
	for (int i=0; i<num_res; i++) {
		if (r_name == res_names[i]) {
			return i;
		}
	}
	return -100; // error
}

/* Return how many units of a resource are available */
int System::how_many_res_avail(string r_name) {
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
bool System::resourcesAvailable(Task &task) {
	int got_resources = 0;

	for (int i=0; i<task.num_res_needed; i++) {
		if (how_many_res_avail(task.res_needed_names[i]) >= task.res_needed_amounts[i]) {
			got_resources++;
		}
	}
	if (got_resources == task.num_res_needed) {
		return true;
	}
	return false;
}

/* Hold every resource needed by the task */
void System::grab_resources(Task &task) {
	int ind;
	for (int i=0; i<task.num_res_needed; i++) {
		// increment res_held by res_needed_amount at the corresponding resource
		task.res_holding_amounts[i] += task.res_needed_amounts[i];
		// do same in sys  
		ind = get_resource_index(task.res_needed_names[i]);
		res_held[ind] += task.res_needed_amounts[i];
	}
}

/* Release every resource needed by the task */
void System::release_resources(Task &task) {
	int ind;
	for (int i=0; i<task.num_res_needed; i++) {
		// decrement res_held by res_needed_amount at the corresponding resource
		task.res_holding_amounts[i] -= task.res_needed_amounts[i];
		// do same in sys  
		ind = get_resource_index(task.res_needed_names[i]);
		res_held[ind] -= task.res_needed_amounts[i];
	}
}

/* Prints all tasks and their status */
void System::print_monitor() {
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
void System::print_system_tasks() {
    cout << "System Tasks:" << endl;
    for (int i = 0; i < numTasks; i++) {
        cout << "[" << i << "] " << tasks[i].name << " (" << tasks[i].status
             << ", runTime= " << tasks[i].busy_time << " ms, idleTime= " << tasks[i].idle_time << " ms):" << endl;
        cout << "\t (tid= 0x" << hex << tasks[i].tid << dec << ")" << endl;
        for (int j = 0; j < tasks[i].num_res_needed; j++) {
            cout << "\t " << tasks[i].res_needed_names[j] << ":\t(needed= " << tasks[i].res_needed_amounts[j]
                 << ", held= " << tasks[i].res_holding_amounts[j] << ")" << endl;
        }
        cout << "\t (Ran: " << tasks[i].iter << " times, Waited: " << tasks[i].time_spent_waiting << " ms)" << endl << endl;
    }
    cout << endl;
}

/* Prints details of every resource */
void System::print_system_resources() {
    cout << "\nSystem Resources:" << endl;
    for (int i = 0; i < num_res; i++) {
        cout << "\t " << res_names[i] << ":\t(maxAvail= " << how_many_res_avail(res_names[i])
             << ", held= " << res_held[i] << ")" << endl;
    }
    cout << endl;
}

/* Read and parse the inputFile into a System instance */
int read_system_params(const char *inputFile, System *sys) {
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
				for (uint i = 1; i < tokens.size(); i++) {
					string n = tokens[i].substr(0, tokens[i].find(':'));
					string a = tokens[i].substr(tokens[i].find(':') + 1);

					sys->res_names.push_back(n);
					sys->res_amounts[sys->num_res] = stoi(a);
					sys->num_res++;
				}
			} 
			else if (tokens[0] == "task") {
				Task task;
				task.name = tokens[1];
				task.busy_time = stoi(tokens[2]); 
				task.idle_time = stoi(tokens[3]); 

				
				int time_int;
				double time_sec;
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

				// Add name:value pairs to sys
				for (uint i = 4; i < tokens.size(); i++) {
					string n = tokens[i].substr(0, tokens[i].find(':'));
					string a = tokens[i].substr(tokens[i].find(':') + 1);

					task.res_needed_names.push_back(n);
					task.res_needed_amounts[task.num_res_needed] = stoi(a);
					task.num_res_needed++;
				}
				sys->task_names.push_back(tokens[1]);
				sys->tasks[sys->numTasks] = task;
				sys->numTasks++;

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

