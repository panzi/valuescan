#ifndef VALUESCAN_H
#define VALUESCAN_H
#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*vs_callback)(void *ctx, size_t offset);

int vs_search_i8( const uint8_t haystack[], size_t haystack_size, int8_t   value, void *ctx, vs_callback callback);
int vs_search_u8( const uint8_t haystack[], size_t haystack_size, uint8_t  value, void *ctx, vs_callback callback);

int vs_search_i16le(const uint8_t haystack[], size_t haystack_size, int16_t  value, void *ctx, vs_callback callback);
int vs_search_u16le(const uint8_t haystack[], size_t haystack_size, uint16_t value, void *ctx, vs_callback callback);
int vs_search_i32le(const uint8_t haystack[], size_t haystack_size, int32_t  value, void *ctx, vs_callback callback);
int vs_search_u32le(const uint8_t haystack[], size_t haystack_size, uint32_t value, void *ctx, vs_callback callback);
int vs_search_i64le(const uint8_t haystack[], size_t haystack_size, int64_t  value, void *ctx, vs_callback callback);
int vs_search_u64le(const uint8_t haystack[], size_t haystack_size, uint64_t value, void *ctx, vs_callback callback);

int vs_search_i16be(const uint8_t haystack[], size_t haystack_size, int16_t  value, void *ctx, vs_callback callback);
int vs_search_u16be(const uint8_t haystack[], size_t haystack_size, uint16_t value, void *ctx, vs_callback callback);
int vs_search_i32be(const uint8_t haystack[], size_t haystack_size, int32_t  value, void *ctx, vs_callback callback);
int vs_search_u32be(const uint8_t haystack[], size_t haystack_size, uint32_t value, void *ctx, vs_callback callback);
int vs_search_i64be(const uint8_t haystack[], size_t haystack_size, int64_t  value, void *ctx, vs_callback callback);
int vs_search_u64be(const uint8_t haystack[], size_t haystack_size, uint64_t value, void *ctx, vs_callback callback);

#ifdef __STDC_IEC_559__
int vs_search_f32( const uint8_t haystack[], size_t haystack_size, float       value, void *ctx, vs_callback callback);
int vs_search_f64( const uint8_t haystack[], size_t haystack_size, double      value, void *ctx, vs_callback callback);
#if !defined(_WIN32) && !defined(_WIN64)
int vs_search_f128(const uint8_t haystack[], size_t haystack_size, long double value, void *ctx, vs_callback callback);
#endif
#endif

int vs_search(const uint8_t haystack[], size_t haystack_size, const uint8_t needle[], size_t needle_size, void *ctx, vs_callback callback);

#ifdef __cplusplus
}
#endif

#endif
