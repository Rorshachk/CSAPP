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
    if (M == 32 && N == 32) {
    //    int t[8];
        int t1, t2, t3, t4, t5, t6, t7, t8;
        for (int i = 0; i < N; i += 8)
          for (int j = 0; j < M; j += 8)
            for(int l = i; l < i + 8; l++){
                for(int k = j; k < j + 8; k++){  //不能用数组= =
                    t1 = A[k][l];
                    t2 = A[k][l + 1];
                    t3 = A[k][l + 2];
                    t4 = A[k][l + 3];
                    t5 = A[k][l + 4];
                    t6 = A[k][l + 5];
                    t7 = A[k][l + 6];
                    t8 = A[k][l + 7];
                    B[l][k] = t1;
                    B[l + 1][k] = t2;
                    B[l + 2][k] = t3;
                    B[l + 3][k] = t4;
                    B[l + 4][k] = t5;
                    B[l + 5][k] = t6;
                    B[l + 6][k] = t7;
                    B[l + 7][k] = t8;
                }
    //              t[k - j] = A[l][k];
    //            for(int k = j; k < j + 8; k++)
    //              B[k][l] = t[k - j];
            }
    }
    if(M == 64 && N == 64){
        int t1, t2, t3, t4, t5, t6, t7, t8;
        for(int i = 0; i < N; i += 8)
          for(int j = 0; j < M; j += 8){
              for(int k = i; k < i + 4; k++){
                  t1 = A[k][j];
                  t2 = A[k][j + 1];
                  t3 = A[k][j + 2];
                  t4 = A[k][j + 3];
                  t5 = A[k][j + 4];
                  t6 = A[k][j + 5];
                  t7 = A[k][j + 6];
                  t8 = A[k][j + 7];

                  B[j][k] = t1;
                  B[j + 1][k] = t2;
                  B[j + 2][k] = t3;
                  B[j + 3][k] = t4;
                  B[j][k + 4] = t5;
                  B[j + 1][k + 4] = t6;
                  B[j + 2][k + 4] = t7;
                  B[j + 3][k + 4] = t8;
              }
              for(int k = j; k < j + 4; k++){
                  t1 = A[i + 4][k];
                  t2 = A[i + 5][k];
                  t3 = A[i + 6][k];
                  t4 = A[i + 7][k];
                  t5 = B[k][i + 4];
                  t6 = B[k][i + 5];
                  t7 = B[k][i + 6];
                  t8 = B[k][i + 7];

                  
                  B[k][i + 4] = t1;
                  B[k][i + 5] = t2;
                  B[k][i + 6] = t3;
                  B[k][i + 7] = t4;

                  B[k + 4][i] = t5;
                  B[k + 4][i + 1] = t6;
                  B[k + 4][i + 2] = t7;
                  B[k + 4][i + 3] = t8;

                  t1 = A[i + 4][k + 4];
                  t2 = A[i + 5][k + 4];
                  t3 = A[i + 6][k + 4];
                  t4 = A[i + 7][k + 4];

                  B[k + 4][i + 4] = t1;
                  B[k + 4][i + 5] = t2;
                  B[k + 4][i + 6] = t3;
                  B[k + 4][i + 7] = t4;
              }
          }
    }
    if(M == 61 && N == 67){
        int siz = 23;
        for(int i = 0; i < N; i += siz)
          for(int j = 0; j < M; j += siz)
            for(int k = i; k < i + siz && k < N; k++)
              for(int t = j; t < j + siz && t < M; t++)
                B[t][k] = A[k][t];
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

