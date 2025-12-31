/*
 * Minimal implementation of mlib/cmp.h for libbson standalone build
 */

#ifndef MLIB_CMP_H
#define MLIB_CMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*mlib_cmp_fn)(const void *a, const void *b);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_CMP_H */
