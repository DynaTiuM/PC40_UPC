#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <upc_relaxed.h>

#define N 598
#define SIZE ((N+2) * (N+2) / THREADS)
#define N_PRIV ((N+2)/THREADS)

shared[SIZE] double grid[N+2][N+2];
shared[SIZE] double new_grid[N+2][N+2];
shared[SIZE] double *ptr[N+2];
shared[SIZE] double *new_ptr[N+2];
shared[SIZE] double *temp;

double *ptr_priv[N_PRIV];
double *new_ptr_priv[N_PRIV];
double *temp_priv;

shared double dTmax_local[THREADS];

void initialize(void)
{
    int j, k, l;

    /* Heat one side of the solid */
     if(MYTHREAD == 0)
    {
        for(j = 1; j < N + 2; j++)
        {
            grid[0][j] = 1.0;
            new_grid[0][j] = 1.0;
        }
    }
}

int main(void)
{
    struct timeval ts_st, ts_end;
    double dTmax, dT, epsilon, time;
    int finished, i, j, k, l;
    double T;
    int nr_iter;

    initialize();

    upc_barrier;

    /* Set the precision wanted */
    epsilon  = 0.0001;
    finished = 0;
    nr_iter = 0;

     /* Initilisation of ptr and new_ptr */
    for(i = 0; i < N+2; i++)
    {
        ptr[i] = &grid[i][0];
        new_ptr[i] = &new_grid[i][0];
    }

    /* Initialisation of private pointers */

    for(i = 0; i < N_PRIV; i++)
    {
        ptr_priv[i] = (double *) &grid[i + (MYTHREAD * N_PRIV)][0];
        new_ptr_priv[i] = (double *) &new_grid[i + (MYTHREAD * N_PRIV)][0];
    }

    upc_barrier;

     /* start the timed section */
    gettimeofday( &ts_st, NULL );

    do
    {
        dTmax = 0.0;
        i = 0;
        //dTmax_local[MYTHREAD] = 0.0;

         if(i + N_PRIV*MYTHREAD > 0) {
            for( j=1; j < N; j++) {
                T = 0.25 * (ptr_priv[1][j] + ptr[MYTHREAD*N_PRIV + i - 1][j] + ptr_priv[0][j - 1] + ptr_priv[0][j + 1])
;

                dT = T - ptr_priv[0][j];

                new_ptr_priv[0][j] = T;

                if( dTmax < fabs(dT) )
                    dTmax = fabs(dT);
            }
        }

        for(i += 1; i < N_PRIV - 1; i++) {
            for(j = 1; j < N; j++) {
                T = 0.25 * (ptr_priv[i + 1][j] + ptr_priv[i - 1][j] + ptr_priv[i][j - 1] + ptr_priv[i][j + 1]);

                dT = T - ptr_priv[i][j];

                new_ptr_priv[i][j] = T;

                if( dTmax < fabs(dT) )
                    dTmax = fabs(dT);
            }
             }

        // Last row
        if(i + N_PRIV*MYTHREAD < N + 1) {
            for(j = 1; j < N; j++)
            {
                T = 0.25 * (ptr[N_PRIV * MYTHREAD + i + 1][j] + ptr_priv[i - 1][j] + ptr_priv[i][j - 1] + ptr_priv[i][j
 + 1]);

                dT = T - ptr_priv[i][j];

                new_ptr_priv[i][j] = T;

                if( dTmax < fabs(dT) )
                    dTmax = fabs(dT);
            }

        }

        dTmax_local[MYTHREAD] = dTmax;
        upc_barrier;

        dTmax = dTmax_local[0];
        for(i = 1; i < THREADS ; i++){
            if(dTmax < dTmax_local[i]) dTmax = dTmax_local[i];
        }
        
        if( dTmax < epsilon ) /* is the precision reached good enough ? */
            finished = 1;
        else
        {
            for(k = 0; k < N + 2; k++){      /* not yet ... Need to prepare */
                temp = ptr[k];
                ptr[k] = new_ptr[k];
                new_ptr[k] = temp;
            }

            for(k = 0; k < N_PRIV; k++){
                temp_priv = ptr_priv[k];
                ptr_priv[k] = new_ptr_priv[k];
                new_ptr_priv[k] = temp_priv;
            }

        }
        nr_iter++;

    } while( finished == 0 );

    gettimeofday( &ts_end, NULL ); /* end the timed section */

    if(MYTHREAD == 0)
    {
        
        /* compute the execution time */
        time = ts_end.tv_sec + (ts_end.tv_usec / 1000000.0);
        time -= ts_st.tv_sec + (ts_st.tv_usec / 1000000.0);


        printf("%d iterations in %.4lf sec\n", nr_iter, time);
    }

    return 0;
}
