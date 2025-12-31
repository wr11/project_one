/*
 * Minimal implementation of mlib/duration.h for libbson standalone build
 */

#ifndef MLIB_DURATION_H
#define MLIB_DURATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t mlib_duration_t;

#define MLIB_DURATION_INFINITE ((mlib_duration_t)-1)

#ifdef __cplusplus
}
#endif

#endif /* MLIB_DURATION_H */
