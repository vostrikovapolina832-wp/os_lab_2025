#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include "find_min_max.h"
#include "utils.h"

static volatile sig_atomic_t timeout_reached = 0;
static void on_alarm(int signo) { (void)signo; timeout_reached = 1; }

static int positive_number(const char *s) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || s == end || *end != '\0' || v <= 0 || v > INT_MAX) return -1;
    return (int)v;
}

static int send_result(int fd, const struct MinMax *result) {
    const char *buf = (const char *)result;
    size_t done = 0;
    while (done < sizeof(*result)) {
        ssize_t n = write(fd, buf + done, sizeof(*result) - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

static int receive_result(int fd, struct MinMax *result) {
    char *buf = (char *)result;
    size_t done = 0;
    while (done < sizeof(*result)) {
        ssize_t n = read(fd, buf + done, sizeof(*result) - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

int main(int argc, char **argv) {
    int seed = -1, array_size = -1, pnum = -1, timeout = 0, delay = 0;
    bool with_files = false;
    static struct option opts[] = {
        {"seed", required_argument, NULL, 's'},
        {"array_size", required_argument, NULL, 'a'},
        {"pnum", required_argument, NULL, 'p'},
        {"by_files", no_argument, NULL, 'f'},
        {"timeout", required_argument, NULL, 't'},
        {"delay", required_argument, NULL, 'd'}, /* only for timeout demonstration */
        {0, 0, 0, 0}
    };
    int c;
    while ((c = getopt_long(argc, argv, "s:a:p:ft:d:", opts, NULL)) != -1) {
        switch (c) {
            case 's': seed = positive_number(optarg); break;
            case 'a': array_size = positive_number(optarg); break;
            case 'p': pnum = positive_number(optarg); break;
            case 'f': with_files = true; break;
            case 't': timeout = positive_number(optarg); break;
            case 'd': delay = positive_number(optarg); break;
            default: fprintf(stderr, "Invalid argument\n"); return 1;
        }
    }
    if (optind != argc || seed < 1 || array_size < 1 || pnum < 1 ||
        pnum > array_size || timeout < 0 || delay < 0) {
        fprintf(stderr, "Usage: %s --seed N --array_size N --pnum N [--by_files] [--timeout N] [--delay N]\n", argv[0]);
        return 1;
    }
    int *array = malloc((size_t)array_size * sizeof(*array));
    pid_t *pids = calloc((size_t)pnum, sizeof(*pids));
    int (*pipes)[2] = calloc((size_t)pnum, sizeof(*pipes));
    if (!array || !pids || !pipes) { perror("allocation"); free(array); free(pids); free(pipes); return 1; }
    GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);

    struct sigaction sa = {0};
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) < 0) { perror("sigaction"); return 1; }
    if (timeout) alarm((unsigned int)timeout);

    struct timespec started, finished;
    clock_gettime(CLOCK_MONOTONIC, &started);
    int created = 0, remaining = 0, failed = 0;
    for (int i = 0; i < pnum; ++i) {
        if (timeout_reached) break;
        if (!with_files && pipe(pipes[i]) < 0) { perror("pipe"); failed = 1; break; }
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); if (!with_files) { close(pipes[i][0]); close(pipes[i][1]); } failed = 1; break; }
        if (pid == 0) {
            /* The child must not run the parent's alarm handler. */
            alarm(0);
            if (delay) { struct timespec pause_for = {delay, 0}; nanosleep(&pause_for, NULL); }
            unsigned int begin = (unsigned int)((long long)i * array_size / pnum);
            unsigned int end = (unsigned int)((long long)(i + 1) * array_size / pnum);
            struct MinMax local = GetMinMax(array, begin, end);
            if (with_files) {
                char name[128];
                snprintf(name, sizeof(name), "/tmp/lab4_%ld_%d.txt", (long)getppid(), i);
                FILE *f = fopen(name, "w");
                if (!f) _exit(2);
                if (fprintf(f, "%d %d\n", local.min, local.max) < 0 || fclose(f) != 0) _exit(2);
            } else {
                for (int j = 0; j < i; ++j) close(pipes[j][0]);
                close(pipes[i][0]);
                int ok = send_result(pipes[i][1], &local);
                close(pipes[i][1]);
                if (ok < 0) _exit(2);
            }
            _exit(0);
        }
        pids[i] = pid;
        ++created;
        ++remaining;
        if (!with_files) close(pipes[i][1]);
    }

    /* Poll without blocking. SIGALRM only raises a flag; kill happens here. */
    while (remaining > 0 && !failed && !timeout_reached) {
        int status = 0;
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);
        if (finished_pid > 0) {
            for (int i = 0; i < created; ++i)
                if (pids[i] == finished_pid) { pids[i] = 0; --remaining; break; }
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) failed = 1;
        } else if (finished_pid == 0) {
            struct timespec pause_for = {0, 10000000L};
            nanosleep(&pause_for, NULL);
        } else if (errno != EINTR) { perror("waitpid"); failed = 1; }
    }
    alarm(0);
    if (timeout_reached || failed) {
        for (int i = 0; i < created; ++i) if (pids[i] > 0) kill(pids[i], SIGKILL);
        while (waitpid(-1, NULL, 0) > 0 || errno == EINTR) { }
        if (timeout_reached) fprintf(stderr, "Timeout reached: unfinished children killed\n");
        else fprintf(stderr, "Child process error\n");
    } else {
        struct MinMax total = {INT_MAX, INT_MIN};
        for (int i = 0; i < created; ++i) {
            struct MinMax local;
            if (with_files) {
                char name[128];
                snprintf(name, sizeof(name), "/tmp/lab4_%ld_%d.txt", (long)getpid(), i);
                FILE *f = fopen(name, "r");
                if (!f || fscanf(f, "%d %d", &local.min, &local.max) != 2) { failed = 1; if (f) fclose(f); break; }
                fclose(f);
            } else {
                if (receive_result(pipes[i][0], &local) < 0) { failed = 1; break; }
            }
            if (local.min < total.min) total.min = local.min;
            if (local.max > total.max) total.max = local.max;
        }
        if (!failed) {
            clock_gettime(CLOCK_MONOTONIC, &finished);
            double ms = (finished.tv_sec - started.tv_sec) * 1000.0 +
                        (finished.tv_nsec - started.tv_nsec) / 1000000.0;
            printf("Min: %d\nMax: %d\nElapsed time: %.3fms\n", total.min, total.max, ms);
        }
    }
    for (int i = 0; i < created; ++i) {
        if (!with_files) close(pipes[i][0]);
        else {
            char name[128];
            snprintf(name, sizeof(name), "/tmp/lab4_%ld_%d.txt", (long)getpid(), i);
            unlink(name);
        }
    }
    free(array); free(pids); free(pipes);
    return timeout_reached ? 124 : (failed ? 1 : 0);
}
