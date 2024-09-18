/*
    Description: An efficiently parallelized OpenMP program that solves 
        linear systems of equations through Gauss-Jordan Elimination 
        with partial pivoting.

    Expects: main num_threads

    Date: March 2024
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "MatrixIO.h"
#include <math.h> 
#include <omp.h> 


int n; 
int p;
double **U; 

void Gaussian_Elim(void);
void Jordan_Elim(void);


/*
    Description: A parallelized program to solve linear systems 
        of equations through Gauss-Jordan Elimination with partial pivoting. 
        Input is read from a file and the result is saved in a file. 
    Expects: main num_threads
*/
int main (int argc, char *argv[]) {
    double* x;
    int i;

    // Ensure correct usage
    if (argc != 2) {
        printf ("%s: Expects main p\n", argv[0]);
        exit(1);
    }

    p = atoi(argv[1]);
    GetInput(&U, &n);

    // Gauss-Jordan Elimination computation 
    Gaussian_Elim();
    Jordan_Elim();

    // Get solution vector from reduced-row echelon form
    x = CreateVec(n);
    for (i = 0; i < n; i++) {
        x[i] = U[i][n] / U[i][i];
    }

    SaveResult(x, n);
    DeleteMatrix(U, n);
    DeleteVector(x);
    return 0;
}


/* Parallelized Gaussian Elimination on matrix U */
void Gaussian_Elim(void) {
    int k, row, col, i, j;
    int k_p;
    double temp, max;

    # pragma omp parallel default(none) private(k, row, col, temp, i, j) shared(U, n, k_p, max) num_threads(p)
    {
        for (k = 0; k < n-1; k++) {
            // Shared variables should only be set by 1 thread
            # pragma omp single
            {
                max = 0.0;
                k_p = k; 
            }

            // Find the pivot row with the maximum absolute value in column k
            # pragma omp for 
            for (row = k; row < n; row++) { 
                if (fabs(U[row][k]) > max) {
                    # pragma omp critical
                    { 
                        if (fabs(U[row][k]) > max) {
                            max = fabs(U[row][k]);
                            k_p = row; 
                        }
                    }   
                }
            }

            // Swap the current row and the pivot row
            # pragma omp for 
            for (col = 0; col < n+1; col++) {
                temp = U[k][col];
                U[k][col] = U[k_p][col];
                U[k_p][col] = temp;
            }

            // Elimination
            # pragma omp for 
            for (i = k+1; i < n; i++) {
                temp = U[i][k] / U[k][k];
                for (j = k; j < n+1; j++) {
                    U[i][j] = U[i][j] - temp * U[k][j];
                }
            }          
        }
    }
}

/* Parallelized Jordan Elimination on matrix U*/
void Jordan_Elim(void) {
    int k, i;

    # pragma omp parallel private(k, i) num_threads(p)
    for (k = n-1; k >= 1; k--) {
        # pragma omp for 
        for (i = 0; i < k; i++) {
            U[i][n] = U[i][n] - U[i][k] / U[k][k] * U[k][n];
            U[i][k] = 0;
        }
    }
}