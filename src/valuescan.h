#ifndef VALUESCAN_H
#define VALUESCAN_H
#pragma once

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*vs_callback)(void *ctx, off_t offset);

int vs_search_i8( int fd, int8_t   value, void *ctx, vs_callback callback);
int vs_search_u8( int fd, uint8_t  value, void *ctx, vs_callback callback);

int vs_search_i16le(int fd, int16_t  value, void *ctx, vs_callback callback);
int vs_search_u16le(int fd, uint16_t value, void *ctx, vs_callback callback);
int vs_search_i32le(int fd, int32_t  value, void *ctx, vs_callback callback);
int vs_search_u32le(int fd, uint32_t value, void *ctx, vs_callback callback);
int vs_search_i64le(int fd, int64_t  value, void *ctx, vs_callback callback);
int vs_search_u64le(int fd, uint64_t value, void *ctx, vs_callback callback);

int vs_search_i16be(int fd, int16_t  value, void *ctx, vs_callback callback);
int vs_search_u16be(int fd, uint16_t value, void *ctx, vs_callback callback);
int vs_search_i32be(int fd, int32_t  value, void *ctx, vs_callback callback);
int vs_search_u32be(int fd, uint32_t value, void *ctx, vs_callback callback);
int vs_search_i64be(int fd, int64_t  value, void *ctx, vs_callback callback);
int vs_search_u64be(int fd, uint64_t value, void *ctx, vs_callback callback);

#ifdef __STDC_IEC_559__
int vs_search_f32( int fd, float       value, void *ctx, vs_callback callback);
int vs_search_f64( int fd, double      value, void *ctx, vs_callback callback);
#if !defined(_WIN32) && !defined(_WIN64)
int vs_search_f128(int fd, long double value, void *ctx, vs_callback callback);
#endif
#endif

int vs_search(int fd, const uint8_t data[], size_t size, void *ctx, vs_callback callback);

#ifdef __cplusplus
}
#endif

#endif
