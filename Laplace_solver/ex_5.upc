#include <upc_relaxed.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BLOCKSIZE 16
#define N BLOCKSIZE*THREADS

//==> declare the x, x_new and b arrays in the shared space with size of
//    BLOCKSIZE*THREADS and with blocking size of BLOCKSIZE
shared [BLOCKSIZE] double x[N];
shared [BLOCKSIZE] double x_new[N];
shared [BLOCKSIZE] double b[N];

void init();

int main(int argc, char **argv){
    int j;

    init();
    upc_barrier;
    //==> insert a upc_forall statement to do work sharing while
    //    respecting the affinity of the x_new array
    upc_forall( j=0; j<(N)-1; j++; j){
        x_new[j] = 0.5*( x[j-1] + x[j+1] + b[j] );
    }

     upc_barrier;

    if( MYTHREAD == 0 ){
        printf("   b   |    x   | x_new\n");
        printf("=============================\n");

        for( j=0; j<N; j++ )
            printf("%1.4f | %1.4f | %1.4f \n", b[j], x[j], x_new[j]);
    }

    return 0;
}

void init(){
    int i;

    if( MYTHREAD == 0 ){
        srand(time(NULL));

        for( i=0; i<N; i++ ){
            b[i] = (double)rand() / RAND_MAX;
            x[i] = (double)rand() / RAND_MAX;
        }
    }
}