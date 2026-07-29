#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* preserve_ext must be 0 or 1. Paths are UTF-8. */
int32_t exchange(const char* path1, const char* path2, uint8_t preserve_ext);

/* Buffers need not be NUL-terminated and must remain readable for the call. */
int32_t exchange_n(const uint8_t* path1, size_t path1_len,
                   const uint8_t* path2, size_t path2_len,
                   uint8_t preserve_ext);

#ifdef __cplusplus
}
#endif
