#ifndef VALUESCAN_H
#define VALUESCAN_H
#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vs_needle {
	size_t size;
	const uint8_t *data;
	void *ctx;
};

typedef int (*vs_callback)(void *ctx, const struct vs_needle *needle, size_t offset);

size_t vs_needle_from_i8(uint8_t needle[], size_t needle_size, int8_t  value);
size_t vs_needle_from_u8(uint8_t needle[], size_t needle_size, uint8_t value);

size_t vs_needle_from_i16le(uint8_t needle[], size_t needle_size, int16_t  value);
size_t vs_needle_from_u16le(uint8_t needle[], size_t needle_size, uint16_t value);
size_t vs_needle_from_i32le(uint8_t needle[], size_t needle_size, int32_t  value);
size_t vs_needle_from_u32le(uint8_t needle[], size_t needle_size, uint32_t value);
size_t vs_needle_from_i64le(uint8_t needle[], size_t needle_size, int64_t  value);
size_t vs_needle_from_u64le(uint8_t needle[], size_t needle_size, uint64_t value);

size_t vs_needle_from_i16be(uint8_t needle[], size_t needle_size, int16_t  value);
size_t vs_needle_from_u16be(uint8_t needle[], size_t needle_size, uint16_t value);
size_t vs_needle_from_i32be(uint8_t needle[], size_t needle_size, int32_t  value);
size_t vs_needle_from_u32be(uint8_t needle[], size_t needle_size, uint32_t value);
size_t vs_needle_from_i64be(uint8_t needle[], size_t needle_size, int64_t  value);
size_t vs_needle_from_u64be(uint8_t needle[], size_t needle_size, uint64_t value);

#ifdef __STDC_IEC_559__
size_t vs_needle_from_f32le( uint8_t needle[], size_t needle_size, float   value);
size_t vs_needle_from_f64le( uint8_t needle[], size_t needle_size, double  value);

size_t vs_needle_from_f32be( uint8_t needle[], size_t needle_size, float   value);
size_t vs_needle_from_f64be( uint8_t needle[], size_t needle_size, double  value);
#endif

int vs_search(const uint8_t haystack[], size_t haystack_size, const struct vs_needle needles[], size_t needle_count, void *ctx, vs_callback callback);

#ifdef __cplusplus
}
#endif

#endif
