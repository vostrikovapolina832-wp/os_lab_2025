#include "sum.h"
int64_t Sum(const struct SumArgs *args) {
    int64_t result = 0;
    for (size_t i = args->begin; i < args->end; ++i) result += args->array[i];
    return result;
}
void *ThreadSum(void *arg) {
    struct SumArgs *args = arg;
    args->partial = Sum(args);
    return NULL;
}
