// 20160443 Dongmin Lee
/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int row;
    int col;
    int i;
    int j;
    int diagonal;
    int temp = 0;
    // Devide the test cases when N is 32, 64 and else
    // When N is equal to 32
    // Divdie the sector for less cache miss
    if ( N == 32 && M == 32){
        // load the data to the cache for the 8 columns at each row
        for (row = 0; row < N; row += 8){
            for (col = 0; col < N; col += 8){
                for (i = row; i < row + 8; i ++){
                    for (j = col; j < col+8; j++){
                        if (i != j){

                            B[j][i] = A[i][j];

                        }
                        else {

                            // Save the diagonal component for later
                            temp = A[i][j];
                            diagonal = i;

                        }

                    }

                    // Update the diagonal component to the transpose of A
                    // Since loading the A's diagonalcomponent on the cache and storing the B's diagonal component on the cache uses the same cache, 
                    // so there will be miss if we do the same thing on the diagonal component
                    if (row == col){

                        B[diagonal][diagonal] = temp;
                    }


                }
            }

        }

    }

    // When matrix is 64 X 64
    // Divide the sector
    else if (M == 64 && N == 64){

        for (row = 0; row < N; row += 8){

            for (col = 0; col < M; col += 8){

                // Divide the sector 
                for (i = row; i < row + 4; i ++){

                    for (j = col; j < col + 4; j ++){

                        if (i != j){

                            B[j][i] = A[i][j];

                        }
                        else {

                            temp = A[i][j];

                        }
                        // update B[j]
                        B[j][i+4] = A[i][j+4];

                    }

                    // update for the diagonalcomponent
                   if (row == col)
                      B[i][i] = A[i][i]; 

                }

                // Update the transpose of A
                i = row + 4;
                for (j = col; j < col + 4; j++){

                    int a0, a1, a2, a3;
                    a0 = B[j][i];
                    a1 = B[j][i+1];
                    a2 = B[j][i+2];
                    a3 = B[j][i+3];
                    B[j][i] = A[i][j];
                    B[j][i+1] = A[i+1][j];
                    B[j][i+2] = A[i+2][j];
                    B[j][i+3] = A[i+3][j];
                    B[j+4][row] = a0;
                    B[j+4][row+1] = a1;
                    B[j+4][row+2] = a2;
                    B[j+4][row+3] = a3;


                }

                // Do the same thing above for the row + 4 for i
                // Since the for-loop has been done for i < row + 4
                for (i = row + 4; i < row + 8; i++){

                    for (j = col + 4; j < col + 8; j++){

                        if (i != j){

                            B[j][i] = A[i][j];
                        }
                        else{
                            temp = A[i][j];
                        }

                    }
                    if (row == col){

                        B[i][i] = A[i][i];

                    }

                }
            }

        }

    }

    // Do for the test case 3
    // Divide the sector and do the same thing done in the first case
    else{
        for (row = 0; row < N; row += 18){

            for (col = 0; col < M; col += 18){

                for (i = row; i < row + 18 && i < N; i++){

                    for (j = col; j < col + 18 && j < M; j++){

                        temp = A[i][j];
                        B[j][i] = temp;

                    }
                }
            }

        }




    }

}


/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

