/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define SIZE 4

int main(){
    int i, j, tmp;
    int A[SIZE];	/* size of physical memory; with code, we'll run out of space!*/

    /* first initialize the array, in reverse sorted order */
    for( i = 0; i < SIZE; i++ ){
        A[i] = SIZE*10 - i;
    }

    /* then sort! */
    for( i = 0; i < SIZE - 1; i++ ){
        for( j = 0; j < SIZE - 1; j++ ){
            if( A[j] > A[j + 1] ){	/* out of order -> need to swap ! */
                tmp = A[j];
                A[j] = A[j + 1];
                A[j + 1] = tmp;
            }
        }
    }
    Exit( A[0] );		/* and then we're done -- should be 0! */
}
