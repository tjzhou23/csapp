/* 
 * trans.c - Matrix transpose B = A^T
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */

#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif	/* MAX */

int is_transpose(int M, int N, int A[N][M], int B[M][N]);


void transpose_blockwise(int br,  /* # of rows in a block */
                         int bc,  /* # of cols in a block */
                         int M, int N,
                         int A[N][M], int B[M][N])
{
    int i, j, ii, jj, li, lj;
    int diag, diag_i;
    for (i = 0; i < N; i += br) {
        for (j = 0; j < M; j += bc) {
            li = MIN(N, i + br);
            lj = MIN(M, j + bc);
            for (ii = i; ii < li; ii++) {
                diag_i = -1;
                for (jj = j; jj < lj; jj++) {
                    if (ii != jj) {
                        B[jj][ii] = A[ii][jj];
                    } else {

                        diag_i = ii;
                        diag = A[ii][ii];
                    }
                }
                if (diag_i >= 0) {
                    B[diag_i][diag_i] = diag;
                }
            }
        }
    }
}


void transpose_64x64(int A[64][64], int B[64][64]) {
    int i8, j8, p;
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

    for (i8 = 0; i8 < 64; i8 += 8) {
        for (j8 = 0; j8 < 64; j8 += 8) {

            for (p = i8; p < i8 + 4; p++) {
                tmp0 = A[p][j8];
                tmp1 = A[p][j8+1];
                tmp2 = A[p][j8+2];
                tmp3 = A[p][j8+3];

                tmp4 = A[p][j8+4];
                tmp5 = A[p][j8+5];
                tmp6 = A[p][j8+6];
                tmp7 = A[p][j8+7];

                /* Move A12' to B21 */
                B[j8+4][p] = tmp4;
                B[j8+5][p] = tmp5;
                B[j8+6][p] = tmp6;
                B[j8+7][p] = tmp7;

                /* Move A11' to B22 */
                B[j8+4][p+4] = tmp0;
                B[j8+5][p+4] = tmp1;
                B[j8+6][p+4] = tmp2;
                B[j8+7][p+4] = tmp3;
            }

            for (p = j8; p < j8 + 4; p++) {
                tmp4 = A[i8+4][p+4];
                tmp5 = A[i8+5][p+4];
                tmp6 = A[i8+6][p+4];
                tmp7 = A[i8+7][p+4];

                tmp0 = B[p+4][i8+4];
                tmp1 = B[p+4][i8+5];
                tmp2 = B[p+4][i8+6];
                tmp3 = B[p+4][i8+7];

                /* Move A22' to B22 */
                B[p+4][i8+4] = tmp4;
                B[p+4][i8+5] = tmp5;
                B[p+4][i8+6] = tmp6;
                B[p+4][i8+7] = tmp7;

                /* Move B22 to B11 */
                B[p][i8] = tmp0;
                B[p][i8+1] = tmp1;
                B[p][i8+2] = tmp2;
                B[p][i8+3] = tmp3;

                tmp4 = A[i8+4][p];
                tmp5 = A[i8+5][p];
                tmp6 = A[i8+6][p];
                tmp7 = A[i8+7][p];

                /* Move A21' to B12 */
                B[p][i8+4] = tmp4;
                B[p][i8+5] = tmp5;
                B[p][i8+6] = tmp6;
                B[p][i8+7] = tmp7;
            }

        }
    }
}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    if (M == 32 && N == 32) {
        transpose_blockwise(8, 8, M, N, A, B);
    } else if (M == 64 && N == 64) {
        transpose_64x64(A, B);
    } else if (M == 61 && N == 67) {
        transpose_blockwise(16, 8, M, N, A, B);
    } else {
        transpose_blockwise(4, 4, M, N, A, B);
    }

    ENSURES(is_transpose(M, N, A, B));
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

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
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


