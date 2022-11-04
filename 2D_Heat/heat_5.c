#include <upc_relaxed.h>
#include <upc_collective.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

shared double dTmax_local[THREADS];

void initialize(shared[] double* grid, shared[] double* new_grid, int N)
{
    int i, j;
    for(i = 0; i < N + 2; i++)
    {
        for(j = 0; j < N + 2; j++)
        {
            grid[i * (N + 2) + j] = 0.0;
            new_grid[i * (N + 2) + j] = 0.0;
        }
    }

    for(j = 0; j < N + 2; j++)
    {
        grid[j] = 1.0;
        new_grid[j] = 1.0;
    }
}

int main(int argc, char* argv[])
{
    struct timeval ts_st, ts_end;
    double dTmax, dT, epsilon, time;
    int finished, i, j, k, l;
    double T;
    int nr_iter;

    if(argc != 2)
    {
          printf("Enter 2 arguments please \n");
        return -1;
    }

    int N = atoi(argv[1]);
    int BLOCK_SIZE = ((N + 2) * (N + 2) / THREADS);

    //shared[] double* dTmax = upc_all_alloc(1,THREADS);
    shared[] double* ptr = upc_all_alloc(BLOCK_SIZE, BLOCK_SIZE * THREADS);
    shared[] double* new_ptr = upc_all_alloc(BLOCK_SIZE, BLOCK_SIZE * THREADS);

    #define ptr_get(x, y) ptr[(x) * (N + 2) + (y)]

    if(MYTHREAD == 0)
    {
        initialize(ptr, new_ptr, N);
    }

    upc_barrier;

    finished = 0;
    nr_iter = 0;
    epsilon = 0.0001;

    gettimeofday(&ts_st, NULL);

    do
    {
        dTmax = 0.0;
        //i = (N + 2) * MYTHREAD / THREADS;

        int iMax = (N + 2) * (MYTHREAD + 1) / THREADS;

        if(iMax > N + 1)
            iMax = N + 1;

        upc_forall(i = 1; i < iMax; i++; i)
        {
             for(j = 1; j <= N; j++)
            {
                T = 0.25 * (ptr_get(i + 1, j)
                + ptr_get(i - 1, j) + ptr_get(i, j - 1)
                + ptr_get(i, j + 1));

                dT = T - ptr_get(i, j);

                new_ptr[i * (N + 2) + j] = T;

                if(dTmax < fabs(dT))
                    dTmax = fabs(dT);
            }
        }

        dTmax_local[MYTHREAD] = dTmax;

        upc_barrier;

        dTmax = dTmax_local[0];

        for(i = 1; i < THREADS; i++)
            if(dTmax < dTmax_local[i]) dTmax = dTmax_local[i];

        upc_barrier;

        //upc_all_reduceD(&dTmax_g, dTmax, UPC_MAX, THREADS, 1, NULL, UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC);

        if(dTmax < epsilon)
        {
            finished = 1;
        }
        else
        {
            shared[] double* temp;
            temp = ptr;
            ptr = new_ptr;
            new_ptr = temp;
        }

        nr_iter++;

    }while(finished == 0);

    gettimeofday(&ts_end, NULL);

    if(MYTHREAD == 0)
    {
        time = ts_end.tv_sec + (ts_end.tv_usec / 1000000.0);
        time -= ts_st.tv_sec + (ts_st.tv_usec / 1000000.0);

        printf("%d iterations in %.4lf sec\n", nr_iter, time);
        printf("Time per iteration : %.4lf \n", time * 1000. / nr_iter);
    }

    return 0;
}