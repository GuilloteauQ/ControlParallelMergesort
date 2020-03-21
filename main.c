#include <assert.h>
#include <omp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

size_t block_size = 0;
double reference = 0;  // nb of threads in the system
// int cumulated_error = 0;
uint8_t num_threads = 0;
double kp = 0.0;
double ki = 0.0;
uint8_t percentage = 0;
pthread_mutex_t mutex_reference;

void controller(double speedup) {
    pthread_mutex_lock(&mutex_reference);
    if (speedup / num_threads <= 2) {
        double error = reference - speedup;
        block_size = (size_t)((double)block_size + kp * error);
        block_size = (block_size <= 0) ? 1 : block_size;
    }
    pthread_mutex_unlock(&mutex_reference);
}

size_t get_block_size() {
    size_t local_block_size;
    pthread_mutex_lock(&mutex_reference);
    local_block_size = block_size;
    pthread_mutex_unlock(&mutex_reference);
    return local_block_size;
}

void merge(int* tab, size_t size) {
    int* tmp = malloc(sizeof(int) * size);

    size_t mid = size / 2;

    size_t a = 0;
    size_t b = mid;
    size_t c = 0;

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

void seq_sort(int* tab, size_t size) {
    assert(size > 0);
    qsort(tab, size, sizeof(int), cmp);
}

void regular_mergesort(int* tab, size_t size, size_t local_block_size) {
    size_t mid = size / 2;
    if (size <= local_block_size) {
        seq_sort(tab, size);
    } else {
#pragma omp task
        regular_mergesort(tab, mid, local_block_size);
#pragma omp task
        regular_mergesort(tab + mid, size - mid, local_block_size);
#pragma omp taskwait
        merge(tab, size);
    }
}

void mergesort(int* tab, size_t size, size_t is_in_measure) {
    size_t mid = size / 2;
    size_t reduced_size =
        size / (2 * num_threads);  // probably more a power than a *
    size_t local_block_size = block_size;

    if (size <= local_block_size) {
        seq_sort(tab, size);
    } else if (!is_in_measure && (rand() % 100 < percentage) &&
               reduced_size <= local_block_size &&
               reduced_size > local_block_size / 2) {
        // Time to start a measure
        double seq_total_time;
        double par_total_time;
#pragma omp task shared(par_total_time, size)
        {
            double par_start, par_end;
            par_start = omp_get_wtime();

            mergesort(tab, mid, 1);

            par_end = omp_get_wtime();
            par_total_time = par_end - par_start;
        }
#pragma omp task shared(seq_total_time, size)
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
        // printf(
        //     "num_threads: %d, kp: %f, reference: %f, block_size: %d, size: "
        //     "%d, "
        //     "mid: %d, speedup: %f\n",
        //     num_threads, kp, reference, local_block_size, size, mid,
        //     speedup);
        controller(speedup);
    } else {
#pragma omp task
        mergesort(tab, mid, is_in_measure);
#pragma omp task
        mergesort(tab + mid, size - mid, is_in_measure);
#pragma omp taskwait
        merge(tab, size);
    }
}

int* create_tab(size_t size) {
    int* tab = malloc(sizeof(int) * size);
    for (size_t i = 0; i < size; i++) {
        tab[i] = size - i - 1;
    }
    return tab;
}

void print_tab(int* tab, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%d ", tab[i]);
    }
    printf("\n");
}

void is_sorted(int* tab, size_t size) {
    for (size_t i = 0; i < size - 1; i++) {
        assert(tab[i] <= tab[i + 1]);
    }
}

void seq_sort_f(int* tab, size_t size, size_t _) {
    seq_sort(tab, size);
}

double measure_exec_time(size_t size,
                         void (*sort)(int*, size_t, size_t),
                         int init) {
    double start, end;
    int* tab = create_tab(size);
#pragma omp parallel
    {
#pragma omp single
        {
            num_threads = omp_get_num_threads();
            start = omp_get_wtime();
            sort(tab, size, init);
            end = omp_get_wtime();
        }
    }
    is_sorted(tab, size);
    free(tab);
    return end - start;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    if (argc != 6) {
        fprintf(stderr,
                "Bad Usage: %s [size] [block_size] [reference] [kp] "
                "[percentage]\n",
                argv[0]);
        return 1;
    }
    size_t size = atoi(argv[1]);
    block_size = atoi(argv[2]);
    reference = atof(argv[3]);
    kp = atof(argv[4]);
    percentage = atoi(argv[5]);

    pthread_mutex_init(&mutex_reference, NULL);

    // printf("Control Start\n");
    double control_time = measure_exec_time(atoi(argv[1]), mergesort, 0);
    // printf("Control Done\n");
    // printf("Paralle Start\n");
    double parallel_time =
        measure_exec_time(atoi(argv[1]), regular_mergesort, atoi(argv[2]));
    // printf("Parallel Done\n");
    // double speedup = parallel_time / control_time;

    double seq_time = measure_exec_time(atoi(argv[1]), seq_sort_f, 0);

    // pretty print
    // printf("control: %lf, parallel: %lf -> speedup: %lf\n", control_time,
    //        parallel_time, speedup);

    // csv print
    // num_threads, size, initial_block_size, reference, kp, percentage,
    // control_time, parallel_time, seq_time
    printf("%d, %ld, %d, %f, %f, %d, %lf, %lf, %lf\n", num_threads, size,
           atoi(argv[2]), atof(argv[3]), kp, percentage, control_time,
           parallel_time, seq_time);
    pthread_mutex_destroy(&mutex_reference);
    return 0;
}

