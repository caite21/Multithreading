#include <unistd.h> // for sleep
#include <string.h> // for strcmp
#include <stdio.h> // for file I/O
#include <poll.h> // for polls
#include <iostream> // string, open
#include <sys/time.h>
#include <sys/resource.h> // for setrlimit()
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

// assumptions:
#define NCLIENT 3 // Max number of clients 
#define CPU_LIMIT 45 // secs
#define POLL_TIMEOUT 0 // Max time client waits for reply

#define MAXLINE 80 // Max # of characters in an input file line or content line
#define MAXWORD 32 // Max # of characters in an object name, or serverAddress name
#define MAX_NTOKEN 3  // Max # of tokens in a string
#define NOBJECT 16 // Max num of files/objects that can be "put" by clients
#define NCONTENT 3 // Max number of content lines per object

/*
    packet stores the information for sending
*/
struct packet {
    int idNumber = 0;
    char type[7] = {'\0'}; // PUT, GET, DELETE, GTIME, TIME, OK, ERROR
    char message[MAXWORD+1] = {'\0'}; // object name or error message
    double num = 0.0;
    char file_content[NCONTENT][MAXLINE] = {'\0'}; // for put and get
};

/*
    file_sys stores the file system put by the clients
*/
struct file_sys {
    int index = 0;
    char files[NOBJECT][MAXWORD] = {'\0'};
    int owners[NOBJECT] = {'\0'};
    char file_contents[NOBJECT][NCONTENT][MAXLINE] = {'\0'};
};

// function prototypes, see bottom of file for details of each
int create_serv_socket_and_init(int portNumber, char * program_name);
void set_up_polling(struct pollfd * p, int s);
int create_cli_socket_and_init(char * serverAddress, int portNumber, char * program_name);
void server_handle_client(struct packet * pr, struct file_sys * fs, int fd);
void server_GTIME(struct packet * pr, int fd, int i, clock_t start);
int server_put(struct packet * pr, struct packet * p, struct file_sys * fs);
int server_del(struct packet * pr, struct packet * p, struct file_sys * fs);
int server_get(struct packet * pr, struct packet * p, struct file_sys * fs);
void server_print(struct file_sys * fs);
void client_hello(int s, int idNumber);
void client_send_and_recv(int fd, struct pollfd * fdarray, struct packet * p, struct packet * pr);
void client_add_file_contents(FILE * fp, int fd, struct packet * p);


/*
    Description: Follows the server route or the client route 
    based on how it is invoked. The client goes through the input file and 
    sends packets to the server while printing the proper things and also 
    delays if it requested. The server receives the packets, performs an 
    action based on the packet type, then replies to those packets. The 
    server does this while also receiving input from stdin and can respond 
    to the commands list or quit. This is done using sockets.
*/
int main (int argc, char *argv[]) {
    int idNumber;
    char * inputFile;
    int portNumber;
    char * serverAddress;

    // SERVER ROUTE
    if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 's') {
        idNumber = 0;
        portNumber = atoi(argv[2]);
        int N = 1, s;
        int newsock[NCLIENT + 1];
        int fd, done_flag = 0;
        struct file_sys fs;
        struct pollfd pfd[NCLIENT + 2];  // NCLIENTs + server + stdin
        struct sockaddr_in from;
        int done_clients[NCLIENT+1] = {0};
        int client_ids[NCONTENT+1] = {0};

        // set up server socket and polling
        s = create_serv_socket_and_init(portNumber, argv[0]);
        set_up_polling(&pfd[0], s);
        set_up_polling(&pfd[NCLIENT + 1], STDIN_FILENO);

        printf("a3w23: do_server\n");  
        printf("Server is accepting connections (port= %d)\n\n", portNumber);
        clock_t start = clock();

        // run a server that polls for data from the clients
        while(done_flag == 0) {
            poll(pfd, NCLIENT + 2, POLL_TIMEOUT);

            // accept any new connections
            if ((N < NCLIENT) && (pfd[0].revents & POLLIN)) {
                // accept a new connection
                socklen_t fromlength = sizeof(from);
                newsock[N] = accept(s, (struct sockaddr *) &from, &fromlength);
                set_up_polling(&pfd[N], newsock[N]);
                N++;
            }

            // check receiving from stdin
            if ((pfd[NCLIENT + 1].revents & POLLIN)) {
                char inStr[MAXLINE];
                fgets(inStr,MAXLINE,stdin);
                // list command
                if (!strcmp(inStr, "list\n")) {
                    server_print(&fs);
                }
                // quit command
                if (!strcmp(inStr, "quit\n")) {
                    printf("do_server: server closing main socket\n");
                    done_flag = 1;
                    for (int i = 1; i < NCLIENT+1; i++) {
                        printf("  done[%d]= %d\n", i, done_clients[i]);
                    }
                    // close all clients left
                    for (int i = 1; i < N; i++) {
                        close (newsock[i]);
                    } 
                    close (s);
                    return 0;
                }
            }

            // check receiving from clients
            for (int i = 1; i < N; i++) {
                if (pfd[i].revents & POLLIN) {
                    struct packet pr;
                    read(newsock[i], (char *) &pr, sizeof(pr));
                    fd = newsock[i];

                    // empty package: client has unexpectedly quit
                    if (!strcmp(pr.type, "")) {
                        printf("Received empty frame\n");
                        printf("Server lost connection with client %d\n\n\n", client_ids[i]);
                        pr.idNumber = client_ids[i];
                        strcpy(pr.type, "QUIT"); // simulate QUIT packet
                    }
                    // QUIT packet
                    if (!strcmp(pr.type, "QUIT")) {
                        close(newsock[i]);
                        done_clients[pr.idNumber] = 1;
                        N--;
                        // shift newsock array and pfd array
                        for (int j = i; j < N; j++) {
                            newsock[j] = newsock[j+1];
                            pfd[j+1].fd = newsock[j];
                        }
                        i--;
                    }
                    // GTIME packet
                    else if (!strcmp(pr.type, "GTIME")) {
                    	server_GTIME(&pr, fd, i, start);
                    } 
                    // GET, PUT, DELETE, OK, and HELLO packets
                    else {
                        // HELLO packet
                        if (!strcmp(pr.type, "HELLO")) {
                            client_ids[i] = pr.idNumber;
                            done_clients[pr.idNumber] = 0; 
                        }
                        // other packets
                    	server_handle_client(&pr, &fs, fd);
                    }
                }
            }
        }
    }

    // CLIENT ROUTE
    else if(argc == 6 && argv[1][0] == '-' && argv[1][1] == 'c') {
        idNumber = atoi(argv[2]);
        inputFile = argv[3];
        serverAddress = argv[4];
        portNumber = atoi(argv[5]);
        char inStr[MAXLINE];
        FILE * fp;
        int s;

        if (idNumber < 1 || idNumber > NCLIENT) {
            // check idNumber range
            printf("Please use an idNumber within the range [1,%d]\n", NCLIENT);
            exit(-1);
        }
        printf("main: do_client (idNumber= %d, inputFile= '%s')\n", idNumber, inputFile);
        printf("\t(server= '%s', port= %d)\n\n", serverAddress, portNumber);

        // create socket, init CPU, and open input file
        s = create_cli_socket_and_init(serverAddress, portNumber, argv[0]);
        fp = fopen(inputFile, "r");
        if(fp == NULL) {
            printf("Opening inputFile failed.\n");
            exit(-1);
        }

        // send HELLO packet
        client_hello(s, idNumber);

        // read 1 line from inputFile
        while(fgets(inStr,MAXLINE,fp) != NULL) {
            // get rid of trailing newline
            if (inStr[strlen(inStr)-1] == '\n') inStr[strlen(inStr)-1] = '\0';
            
            if (inStr[0] == '#' || inStr[0] == '\0') {/* skip line */}

            else {
                // is a command
                char WSPACE[]= "\n \t";
                char* token;
                int count=0;

                // tokenize string
                token = strtok(inStr, WSPACE);
                char **tokens = new char*[MAX_NTOKEN];
                while (token != NULL) {
                    tokens[count] = token;
                    count++;
                    token = strtok(NULL, WSPACE);
                }

                // check if idNumber is this client 
                if (atoi(tokens[0]) == idNumber) {
                    // initialize polling
                    struct pollfd fdarray[1];
                    set_up_polling(&fdarray[0], s);

                    // initialize packet for receiving
                    struct packet pr;
                    // initialize packet to send
                    struct packet p;
                    p.idNumber = idNumber;
                    strcpy(p.type, tokens[1]);
                    // uppercase(type)
                    for (long unsigned int i = 0; i < sizeof(p.type); i++) {
                        p.type[i] = toupper(p.type[i]);
                    }

                    // packet type is QUIT
                    if (!strcmp(tokens[1], "quit")) {
                        // let server know this client is no longer receiving
                        strcpy(p.type, "QUIT");
                        write(s, &p, sizeof(p));
                        printf("do_client: client closing connection.\n");
                        // clean up and quit without blocking the server
                        delete[] tokens;
                        close(s);
                        fclose(fp);
                        exit(0);
                    }
                    // packet type is PUT, GET, DELETE
                    else if (!strcmp(tokens[1], "put") || !strcmp(tokens[1], "get") || !strcmp(tokens[1], "delete")) {
                        // message is object name
                        strcpy(p.message, tokens[2]);

                        if (!strcmp(tokens[1], "put")) {
                            // if PUT, get the contents
                        	client_add_file_contents(fp, s, &p);
                        }

                        printf("Transmitted (src= %d) (%s: %s)\n", idNumber, p.type, p.message);

                        if (!strcmp(tokens[1], "put")) {
                            // if PUT, print the contents
                        	for (int i = 0; i < NCONTENT; i++) {
					        	if (strcmp(p.file_content[i], "\0")) {
					        		printf("  [%d]: '%s'\n", i, p.file_content[i]);
					        	}
					        }
                        }

                        client_send_and_recv(s, fdarray, &p, &pr);

                        if (!strcmp(pr.type, "ERROR")) {
                            // received ERROR packet
                            printf("Received (src= %d)  (%s: %s)\n\n", pr.idNumber, pr.type, pr.message);
                        }
                        else { 
                            // received OK packet
                            printf("Received (src= %d) %s\n", pr.idNumber, pr.type);
                            // print gotten file contents
                            if (!strcmp(tokens[1], "get")) {
                                for (int i = 0; i < NCONTENT; i++) {
                                    if (strcmp(pr.file_content[i], "\0")) {
                                        printf("  [%d]: '%s'\n", i, pr.file_content[i]);
                                    }
                                }
                            }
                            printf("\n");
                        }
                    }
                    // packet type is GTIME
                    else if (!strcmp(tokens[1], "gtime")) {
                        strcpy(p.message, "");
                        printf("Transmitted (src= client:%d) %s\n", idNumber, p.type);
                        client_send_and_recv(s, fdarray, &p, &pr);
                        printf("Received (src= server) (%s: %.2f) \n\n", pr.type, pr.num);
                    }
                    // command is delay
                    else if (!strcmp(tokens[1], "delay")) {
                        printf("*** Entering a delay period of %s msec\n", tokens[2]);
                        sleep(atoi(tokens[2])/1000);
                        printf("*** Exiting delay period\n\n");    
                    }
                    else {
                        printf("unexpected command\n");
                    }
                }
                delete[] tokens;
            }
        }
        close(s);
        fclose(fp);
    }
    // if incorrectly invoked  
    else {
      printf("Expecting:  main -s portNumber\n");
      printf("\tor main -c idNumber inputFile serverAddress portNumber\n\n");
      exit(1);
    }
    return 0;
}



/*
    Sets CPU limit, creates a managing socket with the portNumber, 
    binds and listens (also checks for errors). Returns socket.
*/
int create_serv_socket_and_init(int portNumber, char * program_name) {
    // -----------------------------------------------------------
    // removed
    // 
    // initialize server socket
    // set CPU limit
    // create a managing socket (TCP) 
    // bind the managing socket to a name
    // indicate how many connection requests can be queued
    // -----------------------------------------------------------	
    return s;
}

/*
	Sets CPU limit, creates a cient socket with the serverAddress
	and portNumber and connects (also checks for errors). 
    Returns socket.
*/
int create_cli_socket_and_init(char * serverAddress, int portNumber, char * program_name) {
    // the following enclosed code is from CMPUT 379 Slide deck #5
    // -----------------------------------------------------------
    // removed
    // -------------------------------------------------------
    return s;
}

/*
    Initializes polling parameters as POLLIN and 0
*/
void set_up_polling(struct pollfd * p, int s) {
	p->fd= s;
    p->events= POLLIN;
    p->revents= 0;
}

/*
    Handles GTIME client packet by calculating the time and
    sending a TIME packet back
*/
void server_GTIME(struct packet * pr, int fd, int i, clock_t start) {
	struct packet p;
	clock_t end = clock();
    // create packet
	printf("Received (src= client:%d) %s\n", pr->idNumber, pr->type);
	p.num = (double)(end - start) / CLOCKS_PER_SEC;
	p.idNumber = i + 1;
	strcpy(p.type, "TIME");
	strcpy(p.message, "");
    // send packet
	write(fd, &p, sizeof(p));
	printf("Transmitted (src= server) (TIME: %.2f)\n\n", p.num);
}

/*
	Handles client packets for GET, PUT, DELETE by calling their functions 
    and sends an OK packet or an ERROR packet
*/
void server_handle_client(struct packet * pr, struct file_sys * fs, int fd) {
    printf("Received (src= client:%d) (%s: %s)\n", pr->idNumber, pr->type, pr->message);
    struct packet p;
    int err_flag = 0;

    // perform get/put/delete, check for error
    if (!strcmp(pr->type, "PUT")) {
        err_flag = server_put(pr, &p, fs);
    }
    else if (!strcmp(pr->type, "GET"))
        err_flag = server_get(pr, &p, fs);
    else if (!strcmp(pr->type, "DELETE"))
        err_flag = server_del(pr, &p, fs);

    if (err_flag == 0) {
    	// no errors, send OK packet with any other added info from above
        strcpy(p.type, "OK");
        strcpy(p.message, "");
        write(fd, &p, sizeof(p));
        printf("Transmitted (src= server) %s\n\n", p.type);
    } 
    else {
        // send with last set error message
        write(fd, &p, sizeof(p));
        printf("Transmitted (src= server) (%s: %s)\n\n", p.type, p.message);
    }
}

/*
    stores the object name and its contents in a packet (returns 0) or 
    sends an error that the object already exists (returns 1)
*/
int server_put(struct packet * pr, struct packet * p, struct file_sys * fs) {
    for(int i = 0; i < fs->index; i++)  {
        // if found file in file_sys
        if (!strcmp(fs->files[i], pr->message)) {
            // error: already exists
            strcpy(p->type, "ERROR");
            strcpy(p->message, "object already exists");
            return 1;
        }
    }
    // add object to file_sys
    strcpy(fs->files[fs->index], pr->message);
    fs->owners[fs->index] = pr->idNumber;

    // add contents to file_sys
    for (int i = 0; i < NCONTENT; i++) {
		// put file contents at same index as the file that was just put
		strcpy(fs->file_contents[fs->index][i], pr->file_content[i]);
		if (strcmp(pr->file_content[i], "\0")) {		
			printf("  [%d]: '%s'\n", i, pr->file_content[i]);
		}
    }
    fs->index++;
    return 0;
}

/*
    removes the object name (returns 0) and removes its contents 
    or sends an error if it is deleting an object owned by another 
    client (returns 1) 
*/
int server_del(struct packet * pr, struct packet * p, struct file_sys * fs) {
    int index_to_delete = -1;
    for(int i = 0; i < fs->index; i++)  {
        // if found file in file_sys
        if (!strcmp(fs->files[i], pr->message)) {
            if (fs->owners[i] != pr->idNumber) {
                // if owner is not the idNumber that's deleting: error
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

    // check for error
    if (index_to_delete == -1) {
        strcpy(p->type, "ERROR");
        strcpy(p->message, "can't delete non-existing");
        return 1;
    } 

    else {
        // delete object and its contents (leaves a hole)
        strcpy(fs->files[index_to_delete], "\0");
        for (int i = 0; i < NCONTENT; i++) {
            strcpy(fs->file_contents[index_to_delete][i], "\0");
        }
    }
    return 0;
}

/*
    sends the object name (returns 0) or sends an error if it is getting a 
    non-existing object (returns 1) 
*/
int server_get(struct packet * pr, struct packet * p, struct file_sys * fs) {
    for(int i = 0; i < fs->index; i++)  {
        if (!strcmp(fs->files[i], pr->message)) {
            // found object in file_sys, now get contents
		    for (int j = 0; j < NCONTENT; j++) {
				// get contents at same index as the file that was just got
				strcpy(p->file_content[j], fs->file_contents[i][j]);
		    }
            return 0;
        }
    }
    // could not find it
    strcpy(p->type, "ERROR");
    strcpy(p->message, "object not found");
    return 1;
}

/*
    writes the stored information about the objects (the idNumber of the 
    owning client and the object name in an Object Table and the contents)
*/
void server_print(struct file_sys * fs) {
    printf("------- Object Table: -------\n");
    for(int i = 0; i < fs->index; i++) {
        if (strcmp(fs->files[i], "\0")) {
            // if not deleted, print object name
            printf("(owner: %d, name= %s)\n", fs->owners[i], fs->files[i]);
            // print non-empty contents
            for (int j = 0; j < NCONTENT; j++) {
            	if (strcmp(fs->file_contents[i][j], "\0")) {
            		printf("  [%d]: '%s'\n", j, fs->file_contents[i][j]);
            	}
            }
        }
    }
    printf("-----------------------------\n\n");
}

/*
    client writes HELLO packet to fd and checks that an OK packet was 
    received or the program will exit
*/
void client_hello(int s, int idNumber) {
    // initalize HELLO packet
    struct packet p;
    p.idNumber = idNumber;
    strcpy(p.type, "HELLO");
    char hello_message[] = "idNumber= x";
    hello_message[10] = idNumber + '0';
    strcpy(p.message, hello_message);

    // send HELLO
    write(s, &p, sizeof(p));
    printf("Transmitted (src= %d) (%s: %s)\n", idNumber, p.type, p.message);

    // wait until receives response
    read(s, (char *) &p, sizeof(p));
    printf("Received (src= %d) %s\n\n", p.idNumber, p.type);
    if (strcmp(p.type, "OK")) {
        // Error, server did not respond properly
        printf("Server did not respond to client %d. Quitting.\n", idNumber);
        exit(-1);
    }
}

/*
    client writes packet p to fd and then polls fd to receive packet pr
*/
void client_send_and_recv(int fd, struct pollfd * fdarray, struct packet * p, struct packet * pr) {
    // send to server
    write(fd, p, sizeof(*p));

    // receive server response
    while(1) {
        poll(fdarray, 1 ,POLL_TIMEOUT);
        if (fdarray[0].revents == POLLIN){
            read(fd, pr, sizeof(*pr));
            break;
        }
    }
}

/*
    reads content lines from inputFile after put commands (between { and } ) 
    and store the contents in the packet member called file_content
*/
void client_add_file_contents(FILE * fp, int fd, struct packet * p) {
    char start_char = '{';
    char end_char = '}';
    char inStr[MAXLINE] = {'\0'};
    int i = 0;

    // read 1 line from inputFile
    if (fgets(inStr,MAXLINE,fp) == NULL) {
        printf("Error: could not read from inputFile in client_send_file_contents()\n");
    }
    // if the line following a put command is not '{':
    if (inStr[0] != start_char) {
        printf("Input file is formatted incorrectly\n");
        printf("Expected '{' after a put command. Qutting client.\n");
        exit(-1);
    }
    // while there is content, send to server
    while(fgets(inStr,MAXLINE,fp) != NULL) {
        if (inStr[0] == end_char) {
            break;
        }
        else {
            // get rid of trailing newline
            if (inStr[strlen(inStr)-1] == '\n') inStr[strlen(inStr)-1] = '\0';
            strcpy(p->file_content[i], inStr);
            i++;
        }
    }
}
