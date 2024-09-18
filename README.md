# Parallel Programming 


## Resource Synchronization <a align="right" href="https://github.com/caite21/Parallel-Programming/tree/main/resource_synchronization">📁</a>
 
Multiple "task" threads repeatedly attempt to acquire their required subset of shared resources. The program synchronizes the tasks with a mutex and monitors the tasks by printing to stdout.

**Code Structure:** The TaskManager class ([task_manager.h](https://github.com/caite21/Parallel-Programming/blob/main/resource_synchronization/include/task_manager.h)) contains multiple Task structs ([task.h](https://github.com/caite21/Parallel-Programming/blob/main/resource_synchronization/include/task.h)). There is a monitor thread and a thread for each task. 

**Usage:** 
- make test

    Or
- make
- ./main inputFile monitorTimeMilliseconds numIterations

## File Transfer Client Server <a align="right" href="https://github.com/caite21/Parallel-Programming/tree/main/file_transfer_client_server">📁</a>
The client reads commands from an input file and sends execution requests to the server. Packet communication includes handshakes to ensure reliability. 

**Usage:** Run the server and the client on the commands in commands.dat. In different terminals, type:
- make server0
- make client1
- make client2


## OpenMP Gauss-Jordan Elimination <a align="right" href="https://github.com/caite21/Parallel-Programming/tree/main/openmp_gauss_jordan_elim">📁</a>
An optimally parallelized OpenMP program that solves linear systems of equations through Gauss-Jordan Elimination with partial pivoting.

**Note:** MatrixIO.c has been made private and the program will not run without it.

**Usage:** 
- make
- ./main num_threads



## Matrix Multiplication <a align="right" href="https://github.com/caite21/Parallel-Programming/tree/main/multithreading_matrix_mult">📁</a>
Performs efficient multithreaded matrix multiplication with a specified number of threads.

**Note:** IO.c has been made private and the program will not run without it.

**Usage:** 
- make
- ./main num_threads

