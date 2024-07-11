/*
    Description: The client reads commands from an input file and sends 
                packets containing the commands, corresponding to the 
                client's ID, to the server for execution.
	Usage: ./client id input_file
*/

#include "common.h"
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXTOKEN 32			// Max tokens in command


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
    if (id < 1 || id > NCLIENT) {
        printf("Please use an id within the range [1,%d]\n", NCLIENT);
        return 1;
    }

    // Initialize variables
    char fifo_x_server[12], fifo_server_x[12];
	sprintf(fifo_x_server, "fifo-%c-0", argv[1][0]);
	sprintf(fifo_server_x, "fifo-0-%c", argv[1][0]);

    // Open input file and fifos
    int write_fifo = open(fifo_x_server, O_WRONLY);
    if (write_fifo < 0) {
        fprintf(stderr, "Error opening write FIFO %s\n", fifo_x_server);
        return 1;
    }
    int read_fifo = open(fifo_server_x, O_RDONLY | O_NONBLOCK);
    if (read_fifo < 0) {
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
        char *tokens[MAXTOKEN];

        token = strtok(buffer, WSPACE);
        while (token != NULL && count < MAXTOKEN) {
            tokens[count] = token;
            count++;
            token = strtok(NULL, WSPACE);
        }

        // Process command if it matches the client id
        if (atoi(tokens[0]) == id) {
            if (strcmp(tokens[1], "quit") == 0) {
                // Notify server that this client is done
                struct Packet p_close = {id, "QUIT", "", 0.0};
                write(write_fifo, &p_close, sizeof(p_close));
                printf("Client %d has finished.\n", id);

                // Quit without closing read_fifo to avoid blocking server
                close(write_fifo);
                fclose(fp);
                return 0;
            }

            struct Packet p = {id, "", "", 0.0};
            strcpy(p.type, tokens[1]);
            // Convert type to uppercase
            for (long unsigned int i = 0; i < sizeof(p.type); i++) {
                p.type[i] = toupper(p.type[i]);
            }

            if (!strcmp(tokens[1], "put") || !strcmp(tokens[1], "get") || !strcmp(tokens[1], "delete")) {
                strcpy(p.message, tokens[2]);
                write(write_fifo, &p, sizeof(p));
                printf("Transmitted (src= client:%d) %s: %s\n", id, p.type, p.message);
                if (recv_packet(read_fifo) != 0) {
                    break;
                }
            }
            else if (!strcmp(tokens[1], "gtime")) {
                write(write_fifo, &p, sizeof(p));
                printf("Transmitted (src= client:%d) %s\n", id, p.type);
                if (recv_packet(read_fifo) != 0) {
                    break;
                }
            }
            else if (strcmp(tokens[1], "delay") == 0) {
                printf("Entering delay period of %s msec\n", tokens[2]);
                usleep(atof(tokens[2]) * 1000);
                printf("Exiting delay period\n\n");    
            }
            else {
                fprintf(stderr, "Unexpected command\n");
            }
        }
    }
    close(write_fifo);
    close(read_fifo);
    fclose(fp);
    printf("Quit\n");
    return 0;
}