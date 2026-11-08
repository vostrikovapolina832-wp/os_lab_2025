#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include "utils.h"
#include "sum.h"

static int positive_number(const char *s) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || s == end || *end != '\0' || v <= 0 || v > INT_MAX) return -1;
    return (int)v;
}
int main(int argc, char **argv) {
    int threads_num = -1, seed = -1, array_size = -1;
    static struct option opts[] = {
        {"threads_num", required_argument, NULL, 't'},
        {"seed", required_argument, NULL, 's'},
        {"array_size", required_argument, NULL, 'a'},
        {0,0,0,0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "t:s:a:", opts, NULL)) != -1) {
        switch (c) {
            case 't': threads_num = positive_number(optarg); break;
            case 's': seed = positive_number(optarg); break;
            case 'a': array_size = positive_number(optarg); break;
            default: return 1;
        }
    }
    if (optind != argc || threads_num < 1 || seed < 1 ||
        array_size < 1 || threads_num > array_size) {
        fprintf(stderr, "Usage: %s --threads_num N --seed N --array_size N\n", argv[0]);
        return 1;
    }
    int *array = malloc((size_t)array_size * sizeof(*array));
    pthread_t *threads = calloc((size_t)threads_num, sizeof(*threads));
    struct SumArgs *args = calloc((size_t)threads_num, sizeof(*args));
    if (!array || !threads || !args) { perror("allocation"); free(array); free(threads); free(args); return 1; }
    GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start); // generation is excluded
    int created = 0;
    for (int i = 0; i < threads_num; ++i) {
        args[i].array = array;
        args[i].begin = (size_t)i * (size_t)array_size / (size_t)threads_num;
        args[i].end = (size_t)(i + 1) * (size_t)array_size / (size_t)threads_num;
        int err = pthread_create(&threads[i], NULL, ThreadSum, &args[i]);
        if (err) { fprintf(stderr, "pthread_create failed: %d\n", err); break; }
        ++created;
    }
    int64_t total = 0;
    for (int i = 0; i < created; ++i) {
        int err = pthread_join(threads[i], NULL);
        if (err) { fprintf(stderr, "pthread_join failed: %d\n", err); return 1; }
        total += args[i].partial;
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    if (created == threads_num) {
        double ms = (finish.tv_sec - start.tv_sec) * 1000.0 +
                    (finish.tv_nsec - start.tv_nsec) / 1000000.0;
        printf("Total: %" PRId64 "\nElapsed time: %.3fms\n", total, ms);
    }
    free(array); free(threads); free(args);
    return created == threads_num ? 0 : 1;
}
