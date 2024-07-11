/*
	Usage: client id input_file
*/


// #include <iostream>

#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXTOKEN 32			// Max tokens in command
#define MAXWORD 32			// Max characters in an object name
#define NCLIENT 3			// Max number of clients 
#define POLLTIMEOUT 10000	// Max time (ms) client waits for reply

// Packet structure for FIFO communication
struct Packet {
    int id;
    char type[7];            // PUT, GET, DELETE, GTIME, TIME, OK, ERROR
    char message[MAXWORD+1]; // Object name
    double num;
};

/*
    Client main function reads commands from the input file and sends 
    them to the server in packets.
*/
int main (int argc, char *argv[]) {
    int id;
    char *input_file;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s id input_file\n", argv[0]);
        return 1;
    }

    id = atoi(argv[1]);
    input_file = argv[2];

    // Check id range
    if (id < 1 || id > NCLIENT) {
        printf("Please use an id within the range [1,%d]\n", NCLIENT);
        return 1;
    }

    // Initialize variables
    char fifo_x_server[12], fifo_server_x[12];
	sprintf(fifo_x_server, "fifo-%c-0", argv[1][0]);
	sprintf(fifo_server_x, "fifo-0-%c", argv[1][0]);

    int fd_wr_s = open(fifo_x_server, O_WRONLY);
    if (fd_wr_s < 0) {
        fprintf(stderr, "Error opening write FIFO %s\n", fifo_x_server);
        return 1;
    }
    int fd_rd_s = open(fifo_server_x, O_RDONLY | O_NONBLOCK);
    if (fd_rd_s < 0) {
        fprintf(stderr, "Error opening read FIFO %s\n", fifo_server_x);
        return 1;
    }
    FILE *fp = fopen(input_file, "r");
    if(fp == NULL) {
        fprintf(stderr, "Error opening input file %s\n", input_file);
        return 1;
    }

    printf("Running client (id= %d, input_file= %s)\n\n", id, input_file);
    
    // Read input
    char buffer[MAXWORD];
    while(fgets(buffer, MAXWORD, fp) != NULL) {
        // Remove trailing newline
        if (buffer[strlen(buffer)-1] == '\n') buffer[strlen(buffer)-1] = '\0';
        
        // Skip comments and empty lines
        if (buffer[0] == '#' || buffer[0] == '\0') {
        	continue;
        }
        
        // Tokenize command
        char WSPACE[]= "\n \t";
        int count = 0;
        char *token;
        char **tokens = new char*[MAXTOKEN];

        token = strtok(buffer, WSPACE);
        while (token != NULL && count < MAXTOKEN) {
            tokens[count] = token;
            count++;
            token = strtok(NULL, WSPACE);
        }

        // Process command if it matches the client id
        if (atoi(tokens[0]) == id) {
            struct pollfd fdarray[1];
            fdarray[0].fd = fd_rd_s;
            fdarray[0].events = POLLIN;

            if (strcmp(tokens[1], "quit") == 0) {
                // Notify server that this client is done
                struct Packet p_close;
                p_close.id = id;
                strcpy(p_close.type, "QUIT");
                write(fd_wr_s, &p_close, sizeof(p_close));
                printf("Client %d has finished.\n", id);

                // Clean up and quit
                delete[] tokens;
                // Do not close fd_rd_s to avoid blocking server
                close(fd_wr_s);
                fclose(fp);
                return 0;
            }

            // Initialize packet
            struct Packet p;
            p.id = id;
            strcpy(p.type, tokens[1]);
            // Convert type to uppercase
            for (long unsigned int i = 0; i < sizeof(p.type); i++) {
                p.type[i] = toupper(p.type[i]);
            }

            if (!strcmp(tokens[1], "put") || !strcmp(tokens[1], "get") || !strcmp(tokens[1], "delete")) {
                // Transmit packet
                strcpy(p.message, tokens[2]);
                write(fd_wr_s, &p, sizeof(p));
                printf("Transmitted (src= client:%d) (%s: %s)\n", id, p.type, p.message);

                // Receive response
                while(1) {
                    poll(fdarray, 1, POLLTIMEOUT);
                    if (fdarray[0].revents & POLLIN) {
                        struct Packet pr;
                        read(fd_rd_s, (char *) &pr, sizeof(pr));
                        
                        if (strcmp(pr.type, "ERROR") == 0) {
                            printf("Received (src= server)  (%s: %s)\n\n", pr.type, pr.message);
                        }
                        else {
                            printf("Received (src= server) %s\n\n", pr.type);
                        }
                        break;
                        
                    }
                }
            }
            else if (!strcmp(tokens[1], "gtime")) {
                // Transmit packet
                strcpy(p.message, "");
                write(fd_wr_s, &p, sizeof(p));
                printf("Transmitted (src= client:%d) %s\n", id, p.type);
                
                // Receive response
                while(1) {
                    poll(fdarray, 1, POLLTIMEOUT);
                    if (fdarray[0].revents & POLLIN) {
                        struct Packet pr;
                        read(fd_rd_s, (char *) &pr, sizeof(pr));
                        printf("Received (src= server) (%s: %.2f) \n\n", pr.type, pr.num);
                        break;
                    }
                }
            }
            else if (strcmp(tokens[1], "delay") == 0) {
                printf("\n*** Entering a delay period of %s msec\n", tokens[2]);
                sleep(atoi(tokens[2])/1000);
                printf("*** Exiting delay period\n\n");    
            }
            else {
                fprintf(stderr, "Unexpected command\n");
            }
        }
    	delete[] tokens;
    }
    close(fd_wr_s);
    close(fd_rd_s);
    fclose(fp);
	return 0;
}