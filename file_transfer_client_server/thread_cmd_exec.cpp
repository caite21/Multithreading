/*
command_executor_with_thread

Execute commands using popen() with a thread.
Usage: ./thread_cmd_exec delay

Example: ./thread_cmd_exec 5000
         date
         echo Hello World!!
         quit

*/

#include <stdlib.h>
#include <unistd.h> // for sleep
#include <string.h> 
#include <stdio.h> 
#include <pthread.h>

#define MAXLINE 128 // Max # of characters in an input line


/*
    Description: Thread function that repeatedly prompts user 
    to enter a command. Command is executed and the output is 
    printed to stdout. 
*/
void * input_func(void *arg) {
    const char *type = "w";
    char cmd[MAXLINE];
    printf("\nUser command: "); // includes \n from input

    while(fgets(cmd,MAXLINE,stdin) != NULL) {
        // quit means terminate entire program
        if (strcmp(cmd, "quit\n") == 0) {
            exit(0);
        }
        // execute command
        FILE * fp = popen(cmd, type);
        if (fp == NULL) printf("popen didnt work\n");
        int status = pclose(fp);
        if (status == -1) printf("pclose didnt work\n");

        printf("\nUser command: "); // includes \n from input
    }
    pthread_exit((void *) 0);
}


/*
    Description: A thread executes commands that are entered by the user
    repeatedly. 
    Expects: thread_cmd_exec delay
*/
int main (int argc, char *argv[]) {
    printf("This program allows you to enter commands that will be executed with a thread. \nType quit to stop the program.\n");

	// ensure correct usage
	if (argc != 2) {
      printf ("%s: Expecting: thread_cmd_exec delay\n", argv[0]);
      printf("Delay is the time in milliseconds to keep a thread alive.\n");
      exit(1);
    }

    // initialize
    int delay = atoi(argv[1]);
    int err;
    pthread_t tid_input;
    printf("thread_cmd_exec starts: (delay= %d ms)\n", delay);

    while(1) {
        // delays for commands to be entered
        printf("\n*** New thread created for a delay period of %d msec\n", delay);

        // create thread to get user input commands and execute them with popen
        err = pthread_create(&tid_input, NULL, input_func, NULL);
        if (err != 0) printf("can′t create input thread \n");

        // kill input thread after delay
        sleep(delay/1000);
        pthread_cancel(tid_input);
        printf("\n\n*** Thread killed\n");        
    }

    return 0;
}

