/*
    Matrix Multiplication with Multithreading
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "main.h" 


// prototypes
void print_matrix(int **A, int n);
void free_matrix(int ***A, int n);
void empty_matrix(int ***A, int *n);
void matrix_multiply_serial(int ***A, int ***B, int n, int ***C);
void * matrix_multiply(void *k);

// shared by threads
int **A, **B, **C, sqrt_p, n;
float c;


/*
    Description: Matrix multiplication using multiple threads.
    Expects: main num_threads
*/
int main (int argc, char *argv[]) {
    // ensure correct usage
    if (argc != 2) {
        printf ("%s: Expects main p\n", argv[0]);
        exit(1);
    }

    // initialize
    int p, k, err;
    double start, end;
    p = atoi(argv[1]);
    pthread_t tids[p];
    load_input(&A, &B, &n);
    empty_matrix(&C, &n);

    // check p is a square number and n^2 is divisible by p
    if ((int) sqrt(p) * (int) sqrt(p) != p || n * n % p != 0) {
        printf ("number of threads is incorrect\n");
        exit(1);
    }

    get_time(start);

    // divide matrix into p blocks
    sqrt_p = sqrt(p);
    c = n / sqrt_p;
    for (k = 0; k < p; k++) {
        err = pthread_create(&tids[k], NULL, matrix_multiply, (void*) k);
        if (err != 0) printf("Can′t create thread\n");
    }

    // wait for each thread
    for (k = 0; k < p; k++) {
        err = pthread_join(tids[k], NULL);
        if (err != 0) printf("Can't join thread\n");
    } 

    get_time(end);

    save_matrix(C, &n); 
    printf("Time: %f\n", end - start);

    free_matrix(&A, n);
    free_matrix(&B, n);
    free_matrix(&C, n);
    return 0;
}


// performs matrix multiplication on a block of the matrix 
void * matrix_multiply(void * k_ptr) {
    int i, j, u, x, y, x_min, x_max, y_min, y_max;
    int k = (int) k_ptr;

    // block of matrix
    x = floor(k / sqrt_p);
    y = k % sqrt_p;
    x_min = c * x;
    x_max = c * (x + 1);
    y_min = c * y;
    y_max = c * (y + 1);

    // multiplication on martix block
    for (i = x_min; i < x_max; i++) {
        for (j = y_min; j < y_max; j++) {
            C[i][j] = 0;
            for (u = 0; u < n; u++) {
                C[i][j] += A[i][u] * B[u][j];
            }
        }
    }

    return NULL;
}


// allocates space for matrix of size n
void empty_matrix(int ***A, int *n) {
    int i;
    *A = malloc(*n * sizeof(int*));
    for (i = 0; i < *n; i++) {
      (*A)[i] = malloc(*n * sizeof(int));
    }
}


// frees matrix space
void free_matrix(int ***A, int n) {
    int i;
    for (i = 0; i < n; i++) {
        free((*A)[i]);
    }
    free(*A);
}
