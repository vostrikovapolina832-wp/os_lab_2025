#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char etext, edata, end;
static char *cptr = "This message is output by showit()\n";
static char buffer1[25];
static void showit(const char *text) {
    char *buffer2 = malloc(strlen(text) + 1);
    printf("Local pointer buffer2: %p\n", (void *)&buffer2);
    if (!buffer2) { perror("malloc"); exit(EXIT_FAILURE); }
    printf("Allocated heap memory: %p\n", (void *)buffer2);
    strcpy(buffer2, text);
    printf("%s", buffer2);
    free(buffer2);
}

int main(void) {
    int i = 0;
    printf("etext (end of program code): %p\n", (void *)&etext);
    printf("edata (end of initialized data): %p\n", (void *)&edata);
    printf("end   (end of BSS): %p\n", (void *)&end);
    printf("main function: %p\n", (void *)(uintptr_t)main);
    printf("showit function: %p\n", (void *)(uintptr_t)showit);
    printf("global pointer cptr: %p\n", (void *)&cptr);
    printf("global buffer1: %p\n", (void *)buffer1);
    printf("local variable i: %p\n", (void *)&i);
    strcpy(buffer1, "A demonstration\n");
    printf("%s", buffer1);
    showit(cptr);
    return 0;
}
