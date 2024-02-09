C_FLAGS = -Wall -g
BINS = main

all:
	make main

# required executions:
main: main.cpp system.h
	g++ $(C_FLAGS) -pthread main.cpp system.h -o main

clean:
	-rm -rf $(BINS)

# extra for ease of testing:
test: main main.cpp system.h
	./main main-tests.dat 750 3
