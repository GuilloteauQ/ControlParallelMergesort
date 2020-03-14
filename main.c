#include <assert.h>
#include <omp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int block_size = 0;
double reference = 0;  // nb of threads in the system
// int cumulated_error = 0;
int num_threads = 0;
double kp = 0.0;
double ki = 0.0;
pthread_mutex_t mutex_reference;

void controller(double speedup) {
    pthread_mutex_lock(&mutex_reference);
    if (speedup / num_threads <= 2) {
        double error = reference - speedup;
        // cumulated_error += error;
        // block_size =
        //     (int)((double)block_size + kp * error);  // + ki *
        //     cumulated_error;
        block_size = (error <= 0.0) ? block_size / 2 : block_size * 2;
        block_size = (block_size <= 0) ? 1 : block_size;
        printf("%lf, %d\n", speedup, block_size);
    }
    pthread_mutex_unlock(&mutex_reference);
}

int get_block_size() {
    int local_block_size;
    pthread_mutex_lock(&mutex_reference);
    local_block_size = block_size;
    pthread_mutex_unlock(&mutex_reference);
    return local_block_size;
}

void merge(int* tab, int size) {
    int* tmp = malloc(sizeof(int) * size);

    int mid = size / 2;

    int a = 0;
    int b = mid;
    int c = 0;

    while (a < mid && b < size) {
        tmp[c++] = (tab[a] < tab[b]) ? tab[a++] : tab[b++];
    }
    if (a == mid) {
        while (b < size) {
            tmp[c++] = tab[b++];
        }
    } else {
        while (a < mid) {
            tmp[c++] = tab[a++];
        }
    }
    memcpy(tab, tmp, size * sizeof(int));
    free(tmp);
}

static int cmp(const void* x, const void* y) {
    const int* xInt = x;
    const int* yInt = y;

    return *xInt - *yInt;
}

void seq_sort(int* tab, int size) {
    qsort(tab, size, sizeof(int), cmp);
}

void mergesort(int* tab, int size, int is_in_measure) {
    int mid = size / 2;
    int reduced_size =
        size / (2 * num_threads);  // probably more a power than a *
    int local_block_size = get_block_size();
    if (is_in_measure) {
        if (size <= local_block_size) {
            seq_sort(tab, size);
        } else {
#pragma omp task
            mergesort(tab, mid, is_in_measure);
#pragma omp task
            mergesort(tab + mid, size - mid, is_in_measure);
#pragma omp taskwait
            merge(tab, size);
        }
    } else {
        if (reduced_size > local_block_size) {
            // We continue normally
#pragma omp task
            mergesort(tab, mid, is_in_measure);
#pragma omp task
            mergesort(tab + mid, size - mid, is_in_measure);
#pragma omp taskwait
            merge(tab, size);
        } else {
            is_in_measure = 1;
            // We need to do one sub tree in parallel the other in sequential
            double seq_total_time;
            double par_total_time;
#pragma omp task shared(par_total_time)
            {
                double par_start, par_end;
                par_start = omp_get_wtime();

                mergesort(tab, mid, is_in_measure);

                par_end = omp_get_wtime();
                par_total_time = par_end - par_start;
            }
#pragma omp task shared(seq_total_time)
            {
                double seq_start, seq_end;
                seq_start = omp_get_wtime();

                seq_sort(tab + mid, size - mid);

                seq_end = omp_get_wtime();
                seq_total_time = seq_end - seq_start;
            }
#pragma omp taskwait
            merge(tab, size);
            double speedup = seq_total_time / par_total_time;
            printf("kp: %f, reference: %f, size: %d, mid: %d, speedup: %f\n",
                   kp, reference, size, mid, speedup);
            controller(speedup);
        }
    }
}

int* create_tab(int size) {
    int* tab = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++) {
        tab[i] = size - i - 1;
    }
    return tab;
}

void print_tab(int* tab, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", tab[i]);
    }
    printf("\n");
}

void is_sorted(int* tab, int size) {
    for (int i = 0; i < size - 1; i++) {
        assert(tab[i] <= tab[i + 1]);
    }
}

int main(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "Bad Usage: %s [size] [block_size] [reference] [kp]\n",
                argv[0]);
        return 1;
    }
    int size = atoi(argv[1]);
    block_size = atoi(argv[2]);
    reference = atof(argv[3]);
    kp = atof(argv[4]);
    int* tab = create_tab(size);

    pthread_mutex_init(&mutex_reference, NULL);

#pragma omp parallel
    {
#pragma omp single
        {
            num_threads = omp_get_num_threads();
            mergesort(tab, size, 0);
        }
    }
    is_sorted(tab, size);
    pthread_mutex_destroy(&mutex_reference);
    return 0;
}

