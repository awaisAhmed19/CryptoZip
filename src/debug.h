
// File: debug.h
#ifndef DEBUG_UTIL_H
#define DEBUG_UTIL_H

#ifndef DEBUG
#define ASSERT(n)
#else
#include <stdio.h>
#include <stdlib.h>
#define ASSERT(n)                         \
    if (!(n)) {                           \
        printf("%s - Failed\n", #n);      \
        printf("On %s\n", __DATE__);      \
        printf("At %s\n", __TIME__);      \
        printf("In File %s\n", __FILE__); \
        printf("At Line %d\n", __LINE__); \
        exit(1);                          \
    }
#endif

#endif  // DEBUG_UTIL_H
