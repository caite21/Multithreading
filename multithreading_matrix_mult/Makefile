C_FLAGS = -g -Wall

all:
	make main

main: main.c
	gcc $(C_FLAGS) -lpthread -lm main.c IO.c -o main

clean:
	-rm -rf main getmatrix

test1:
	gcc $(C_FLAGS) -o getmatrix matrix.c
	./getmatrix -s 4 -b 3 -p
	make main
	./main 1
	cat data_output

test2:
	make main
	@gcc -g -o getmatrix matrix.c
	@./getmatrix -s 100 -b 10 
	./main 100
	@tail -n 1 data_output

test3:
	make main
	@gcc -g -o getmatrix matrix.c
	./getmatrix -s 12 -b 5 
	./main 1
	@tail -n 1 data_output
	./main 4
	@tail -n 1 data_output
	./main 36
	@tail -n 1 data_output
	./main 144
	@tail -n 1 data_output
