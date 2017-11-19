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
    int temp;
    // Devide the test cases when N is 32, 64 and else
    // When N is equal to 32
    if ( N == 32){
        for (col = 0; col < N; col += 8){
            for (row = 0; row < N; row += 8){
                for (i = row; i < row + 8; i ++){
                    for (j = col; j < col+8; j++){
                        if (i != j){

                            B[j][i] = A[i][j];

                        }
                        else {

                            temp = A[i][j];
                            diagonal = i;

                        }

                    }

                    if (row == col){

                        B[diagonal][diagonal] = temp;
                    }


                }
            }

        }

    }



/*

   else if (N == 64) {
	for (int row = 0; row < N; row += 4) {
		for (int column = 0; column < N; column += 4) {
			a6 = A[row+3][column];
			a5 = A[row+3][column+1];
			a4 = A[row+3][column+2];
			a3 = A[row+3][column+3];

			temp = A[row+2][column];
			B[column][row+3] = a6;
			a6 = A[row+2][column+1];// a6,a5,a4,a3 in use
			a2 = A[row+2][column+2];
			B[column+1][row+3] = a5;
			a5 = A[row+2][column+3];

			diagonal = A[row+1][column];
			a1 = A[row+1][column+1]; //a6,a5,a4,a3,a2,a1 in use
			int blockSize3 = A[row+1][column+2];

			int blockSize1 = A[row+1][column+3];

			B[column][row] = A[row][column];
			B[column][row+1] = diagonal;
			B[column][row+2] = temp;
			B[column+1][row] = A[row][column+1];
			B[column+1][row+1] = a1;
			B[column+1][row+2] = a6;
			B[column+2][row] = A[row][column+2];
			B[column+2][row+1] = blockSize3;
			B[column+2][row+2] = a2;
			B[column+2][row+3] = a4;
			B[column+3][row] = A[row][column+3];
			B[column+3][row+1] = blockSize1;
			B[column+3][row+2] = a5;
			B[column+3][row+3] = a3;

				}
			}
		}
*/
//if(M == 64 && N == 64) {

        for (i = 0; i < N; i += 4){
            for (j = 0; j < M; j += 4){
                /* load A[i][] A[i+1][] A[i+2][]*/
                int a = A[i][j];
                int b = A[i+1][j];
                int x = A[i+2][j];
                int y = A[i+2][j+1];
                int z = A[i+2][j+2];

                /* load B[j+3][] */
                B[j+3][i] = A[i][j+3];
                B[j+3][i+1] = A[i+1][j+3];
                B[j+3][i+2] = A[i+2][j+3];

                /* load B[j+2][], may evict A[i+2][] */
                B[j+2][i] = A[i][j+2];
                B[j+2][i+1] = A[i+1][j+2];
                B[j+2][i+2] = z;

                z = A[i+1][j+1];

                /* load B[j+1][], may evict A[i+1][] */
                B[j+1][i] = A[i][j+1];
                B[j+1][i+1] = z;
                B[j+1][i+2] = y;

                /* load B[j][], may evict A[i][] */
                B[j][i] = a;
                B[j][i+1] = b;
                B[j][i+2] = x;


                /* load A[i+3][], may evict B[j+3][] */
                B[j][i+3] = A[i+3][j];
                B[j+1][i+3] = A[i+3][j+1];
                B[j+2][i+3] = A[i+3][j+2];

                a = A[i+3][j+3];

                /* load B[j+3][], may evict A[i+3][] */
                B[j+3][i+3] = a;
            }
        }
    }

    // When N is equal to 64
    else if (N == 64){
        for (col = 0; col < N; col += 2){
            for (row = 0; row < N; row += 2){
                for (i = row; i < row + 2; i ++){
                    for (j = col; j < col + 2; j ++){
                        if (i != j){

                            B[j][i] = A[i][j];

                        }
                        else {

                            temp = A[i][j];
                            diagonal = i;

                        }

                    }
                    if (row == col){

                        B[diagonal][diagonal] = temp;

                    }

                }

            }
        }

    }

    // When N is not equal to 32 and 64




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

