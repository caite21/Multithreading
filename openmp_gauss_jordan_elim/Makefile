# Makefile for Gauss-Jordan Elimination Program
C_FLAGS = -g -Wall -std=c99


make: main.c
	gcc $(C_FLAGS) -lpthread -lm -fopenmp main.c MatrixIO.c -o main

clean:
	-rm -rf main
