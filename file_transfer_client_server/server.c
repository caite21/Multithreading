#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAXFILES 100    // Max number of files that can be managed
#define MAXWORD 32      // Max length of an object name
#define NCLIENT 3       // Number of clients 

// Packet structure for FIFO communication
struct Packet {
    int id;
    char type[7];            // PUT, GET, DELETE, GTIME, TIME, OK, ERROR
    char message[MAXWORD+1]; // Object name
    double num;
};

// Server's file system representation
struct FileSys {
    int index;
    char files[MAXFILES][MAXWORD];
    int owners[MAXFILES];
};

// Function prototypes
int server_put(struct Packet * p_rec, struct Packet * p, struct FileSys * fs);
int server_del(struct Packet * p_rec, struct Packet * p, struct FileSys * fs);
int server_get(struct Packet * p_rec, struct Packet * p, struct FileSys * fs);
void server_print(struct FileSys * fs);



/*
    Server main function that handles packet communication with clients
    and responds to commands from stdin (list or quit). 
*/
int main (int argc, char *argv[]) {
    // Initialize variables
    struct pollfd pollfd[NCLIENT + 1];
    int in_fifos[NCLIENT + 1] = {0};
    int on_fifos[NCLIENT] = {0};
    char fifo_x_server[] = "fifo-x-0";
    char fifo_server_x[] = "fifo-0-x";
    struct FileSys fs = {0};

    // Create FIFOs for communication with clients
    for (int i = 0; i < NCLIENT; i++) {
        // Client to server FIFO
        fifo_x_server[5] = i + '1';
        if (access(fifo_x_server, F_OK) == -1) {
            if (mkfifo(fifo_x_server, 0666) == -1) {
                perror("Error making FIFO");
                return 1;
            }
        }

        // Server to client FIFO
        fifo_server_x[7] = i + '1';
        if (access(fifo_server_x, F_OK) == -1) {
            if (mkfifo(fifo_server_x, 0666) == -1) {
                perror("Error making FIFO");
                return 1;
            }
        }
    }

    // Setup polling FIFOs and stdin
    for (int i = 0; i < NCLIENT; i++) {
        // Open FIFO for read-only
        fifo_x_server[5] = i + '1'; 
        int fd = open(fifo_x_server, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            fprintf(stderr, "Error opening read FIFO %s\n", fifo_x_server);
            return 1;
        }
        in_fifos[i] = fd;

        // Initialize poll structure
        pollfd[i].fd = fd;
        pollfd[i].events = POLLIN;
        pollfd[i].revents = 0;
    }

    // Setup polling for stdin
    in_fifos[NCLIENT] = 0;  // stdin file descriptor
    pollfd[NCLIENT].fd = 0;
    pollfd[NCLIENT].events = POLLIN;
    pollfd[NCLIENT].revents = 0;

    printf("Running server\n\n");  
    clock_t start = clock();
    int done_flag = 0;

    // Main server loop
    while(!done_flag) {
        poll(pollfd, NCLIENT + 1, 1000);

        // Check for input from stdin
        if (pollfd[NCLIENT].revents & POLLIN) {
            char buffer[MAXWORD];
            if (fgets(buffer, MAXWORD, stdin) != NULL) {
                if (!strcmp(buffer, "quit\n")) {
                    printf("Quitting\n");
                    done_flag = 1;

                    // Notify clients to quit
                    struct Packet p_quit;
                    strcpy(p_quit.type, "Cquit");
                    strcpy(p_quit.message, "");
                    for (int i = 0; i < NCLIENT; i++) {
                        if (on_fifos[i]) {
                            fifo_server_x[7] = i + '1'; 
                            int temp_fd = open(fifo_server_x, O_RDONLY | O_NONBLOCK);
                            int fd = open(fifo_server_x, O_WRONLY);
                            if (fd < 0) {
                                fprintf(stderr, "Error opening write FIFO %s\n", fifo_server_x);
                                return 1;
                            }
                            write(fd, &p_quit, sizeof(p_quit));
                            close(temp_fd);
                            close(fd);
                        }
                    }
                    return 0;
                }

                if (!strcmp(buffer, "list\n")) {
                    server_print(&fs);
                }
            }
        }

        // Check for input from client FIFOs
        for (int i = 0; i < NCLIENT; i++) {
            if (pollfd[i].revents & POLLIN) {
                if (pollfd[i].fd == in_fifos[i]){
                    on_fifos[i] = 1;
                    struct Packet p_rec;
                    read(in_fifos[i], (char *) &p_rec, sizeof(p_rec));
                    
                    if (strcmp(p_rec.type, "GTIME") == 0) {
                        printf("Received (src= client:%d) %s\n", p_rec.id, p_rec.type);
                        fifo_server_x[7] = i + '1'; // get FIFO name
                        int fd = open(fifo_server_x, O_WRONLY);
                        if (fd < 0) {
                            fprintf(stderr, "Error opening write FIFO %s\n", fifo_server_x);
                            return 1;
                        }
                        struct Packet p;
                        clock_t end = clock();
                        p.num = (double)(end - start) / CLOCKS_PER_SEC;
                        p.id = i + 1;
                        strcpy(p.type, "TIME");
                        strcpy(p.message, "");
                        write(fd, &p, sizeof(p));
                        printf("Transmitted (src= server) (TIME: %.2f)\n\n", p.num);
                    } 
                    else if (strcmp(p_rec.type, "QUIT") == 0) {
                        fifo_server_x[7] = i + '1'; 
                        int fd = open(fifo_server_x, O_WRONLY);
                        if (fd < 0) {
                            fprintf(stderr, "Error opening write FIFO %s\n", fifo_server_x);
                            return 1;
                        }
                        printf("Client:%d has finished\n\n", i+1);
                    } 
                    else {
                        // Received GET, PUT, or DELETE packets
                        printf("Received (src= client:%d) (%s: %s)\n", p_rec.id, p_rec.type, p_rec.message);
                        fifo_server_x[7] = i + '1'; // get FIFO name
                        int fd = open(fifo_server_x, O_WRONLY);
                        if (fd < 0) {
                            fprintf(stderr, "Error opening write FIFO %s\n", fifo_server_x);
                            return 1;
                        }

                        struct Packet p;
                        p.id = 0;
                        int err_flag = 0;

                        if (strcmp(p_rec.type, "PUT") == 0) {
                            err_flag = server_put(&p_rec, &p, &fs);
                        } 
                        else if (strcmp(p_rec.type, "GET") == 0) {
                            err_flag = server_get(&p_rec, &p, &fs);
                        } 
                        else if (strcmp(p_rec.type, "DELETE") == 0) {
                            err_flag = server_del(&p_rec, &p, &fs);
                        }

                        if (err_flag == 0) {
                            strcpy(p.type, "OK");
                            strcpy(p.message, "");
                            write(fd, &p, sizeof(p));
                            printf("Transmitted (src= server) %s\n\n", p.type);
                        } 
                        else {
                            write(fd, &p, sizeof(p));
                            printf("Transmitted (src= server) (%s: %s)\n\n", p.type, p.message);
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}


// Server function: Stores the object name or sends an error if the object already exists
int server_put(struct Packet * p_rec, struct Packet * p, struct FileSys * fs) {
    for(int i = 0; i < fs->index; i++)  {
        if (!strcmp(fs->files[i], p_rec->message)) {
            strcpy(p->type, "ERROR");
            strcpy(p->message, "object already exists");
            return 1;
        }
    }
    strcpy(fs->files[fs->index], p_rec->message);
    fs->owners[fs->index] = p_rec->id;
    fs->index ++;
    return 0;
}

// Server function: Removes the object name or sends an error if deleting an object owned by another client
int server_del(struct Packet * p_rec, struct Packet * p, struct FileSys * fs) {
    int index_to_delete = -1;
    for(int i = 0; i < fs->index; i++)  {
        if (!strcmp(fs->files[i], p_rec->message)) {
            if (fs->owners[i] != p_rec->id) {
                strcpy(p->type, "ERROR");
                strcpy(p->message, "client not owner");
                return 1;
            } 
            else {
                index_to_delete = i;
                break;
            }
        }
    }
    if (index_to_delete == -1) {
        strcpy(p->type, "ERROR");
        strcpy(p->message, "can't delete non-existing");
        return 1;
    } 
    else {
        strcpy(fs->files[index_to_delete], "");  // leaves hole
    }
    return 0;
}

// Server function: Sends the object name or sends an error if object does not exist
int server_get(struct Packet * p_rec, struct Packet * p, struct FileSys * fs) {
    for(int i = 0; i < fs->index; i++)  {
        if (!strcmp(fs->files[i], p_rec->message)) {
            return 0;
        }
    }
    strcpy(p->type, "ERROR");
    strcpy(p->message, "object not found");
    return 1;
}

// Server function: Prints the objects and who owns them in a table 
void server_print(struct FileSys * fs) {
    printf("Object Table:\n");
    for(int i = 0; i < fs->index; i++) {
        printf("(owner: %d, name= %s)\n", fs->owners[i], fs->files[i]);
    }
    printf("\n");
}


