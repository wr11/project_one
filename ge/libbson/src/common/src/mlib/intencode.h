/*
 * Minimal implementation of mlib/intencode.h for libbson standalone build
 */

#ifndef MLIB_INTENCODE_H
#define MLIB_INTENCODE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal declarations - actual implementation may vary */
void mlib_int32_encode(uint8_t *buf, int32_t value);
int32_t mlib_int32_decode(const uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_INTENCODE_H */
