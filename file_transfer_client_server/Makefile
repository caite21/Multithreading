C_FLAGS = -Wall -g
BINS = server client thread_cmd_exec

all: server client thread_cmd_exec

server: server.c common.h
	gcc $(C_FLAGS) server.c -o server

client: client.c common.h
	gcc $(C_FLAGS) client.c -o client

thread_cmd_exec: thread_cmd_exec.cpp
	g++ $(C_FLAGS) -pthread thread_cmd_exec.cpp -o thread_cmd_exec 

clean:
	-rm -rf $(BINS)


# Commands to run executables
server0: server
	./server

client1: client
	./client 1 commands.dat

client2: client
	./client 2 commands.dat
