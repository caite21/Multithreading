#ifndef COMMON_H
#define COMMON_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ctype.h>
#include <poll.h>
#include <stdlib.h>

#define MAXWORD 32		// Max characters in an object name
#define NCLIENT 3       // Number of clients 
#define POLLTIMEOUT 10000	// Max time (ms) to wait for reply


// Packet structure for FIFO communication
struct Packet {
    int id;
    char type[7];            // PUT, GET, DELETE, GTIME, TIME, OK, ERROR
    char message[MAXWORD+1]; // Object name
    double num;
};

int send_packet(char fifo_name[9], struct Packet * p) {
    int fd = open(fifo_name, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Error opening write FIFO %s\n", fifo_name);
        return 1;
    }
    write(fd, p, sizeof(*p));
    close(fd);
    return 0;
}

int recv_packet(int fd) {
	// is blocking
    struct pollfd fdarray[1];
    fdarray[0].fd = fd;
    fdarray[0].events = POLLIN;

    while(1) {
        poll(fdarray, 1, POLLTIMEOUT);
        if (fdarray[0].revents & POLLIN) {
            struct Packet pr;
            read(fd, (char *) &pr, sizeof(pr));
            
            if (strcmp(pr.type, "ERROR") == 0) {
                printf("Received (src= server)  %s: %s\n\n", pr.type, pr.message);
            }
            else if (strcmp(pr.type, "CQUIT") == 0) {
                printf("Received (src= server) %s\n\n", pr.type);
                close(fd);
                return 1;
            }
            else if (strcmp(pr.type, "TIME") == 0) {
                printf("Received (src= server) %s: %.2f s\n\n", pr.type, pr.num);
            }
            else {
                printf("Received (src= server) %s\n\n", pr.type);
            }
            break;
        }
    }
    return 0;
}


#endif