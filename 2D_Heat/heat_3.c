#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <upc_relaxed.h>
#include <upc_collective.h>

#define N 598
#define BLOCK_SIZE ((N+2) * (N+2) / THREADS)

shared[BLOCK_SIZE] double grid[N+2][N+2];
shared[BLOCK_SIZE] double new_grid[N+2][N+2];
shared[BLOCK_SIZE] double *ptr[N+2];
shared[BLOCK_SIZE] double *new_ptr[N+2];
shared[BLOCK_SIZE] double* temp;

shared double dTmax_local[THREADS];

void initialize()
{
    int i, j;

    if(MYTHREAD == 0)
    {

        for(i = 0; i < N + 2; i++)
        {
            grid[0][i] = 1.0;
            new_grid[0][i] = 1.0;
        }
    }


}

int main() {

    struct timeval ts_st, ts_end;

    double dTmax, dT, epsilon, time;
    int finished, i, j, k, l;
    double T;
    int nr_iter;

    initialize();

    upc_barrier;

    epsilon = 0.0001;
    finished = 0;
    nr_iter = 0;

    for(j = 0; j < N + 2; j++)
    {
        ptr[j] = &grid[j][0];
        new_ptr[j] = &new_grid[j][0];
    }

    upc_barrier;

    gettimeofday(&ts_st, NULL);

    do
    {
        dTmax = 0.0;

        upc_forall(i = 1; i < N + 1; i++; i)
        {
            for(j = 1; j < N + 1; j++)
            {
                T = 0.25 * (ptr[i + 1][j] + ptr[i-1][j] + ptr[i][j - 1] + ptr[i][j + 1]);
                dT = T - ptr[i][j];
                new_ptr[i][j] = T;
                 if(dTmax < fabs(dT) )
                    dTmax = fabs(dT);
            }
        }

        dTmax_local[MYTHREAD] = dTmax;

        upc_barrier;

        dTmax = dTmax_local[0];

        for(i = 0; i < THREADS; i++)
            if(dTmax < dTmax_local[i])
                dTmax = dTmax_local[i];

        upc_barrier;

        if( dTmax < epsilon)
            finished = 1;
        else
        {

            for(k = 0; k < N + 2; k++)
            {
                temp = ptr[k];
                ptr[k] = new_ptr[k];
                new_ptr[k] = temp;
           }
        }

        nr_iter++;

    } while(finished == 0);

    upc_barrier;

    gettimeofday(&ts_end, NULL);

    if(MYTHREAD == 0)
    {
        time = ts_end.tv_sec + (ts_end.tv_usec / 1000000.0);
        time -= ts_st.tv_sec + (ts_st.tv_usec / 1000000.0);

        printf("%d iterations in %.4lf sec\n", nr_iter, time);
        printf("time per iteration : %.4lf \n",time * 1000. / nr_iter);
    }

    return 0;
}