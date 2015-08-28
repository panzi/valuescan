#include "valuescan.h"

#include <string.h>

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);

int vs_search_i8(const uint8_t haystack[], size_t haystack_size, int8_t value, void *ctx, vs_callback callback) {
	return vs_search_u8(haystack, haystack_size, (uint8_t)value, ctx, callback);
}

int vs_search_u8(const uint8_t haystack[], size_t haystack_size, uint8_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = { value };
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

int vs_search_i16le(const uint8_t haystack[], size_t haystack_size, int16_t value, void *ctx, vs_callback callback) {
	return vs_search_i16le(haystack, haystack_size, (uint16_t)value, ctx, callback);
}

int vs_search_u16le(const uint8_t haystack[], size_t haystack_size, uint16_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = {
		value & 0xFF,
		value >> 8
	};
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

int vs_search_i32le(const uint8_t haystack[], size_t haystack_size, int32_t value, void *ctx, vs_callback callback) {
	return vs_search_u32le(haystack, haystack_size, (uint32_t)value, ctx, callback);
}

int vs_search_u32le(const uint8_t haystack[], size_t haystack_size, uint32_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = {
		 value        & 0xFF,
		(value >>  8) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 24) & 0xFF
	};
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

int vs_search_i64le(const uint8_t haystack[], size_t haystack_size, int64_t value, void *ctx, vs_callback callback) {
	return vs_search_u64le(haystack, haystack_size, (uint64_t)value, ctx, callback);
}

int vs_search_u64le(const uint8_t haystack[], size_t haystack_size, uint64_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = {
		 value        & 0xFF,
		(value >>  8) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 24) & 0xFF,
		(value >> 32) & 0xFF,
		(value >> 40) & 0xFF,
		(value >> 48) & 0xFF,
		(value >> 56) & 0xFF
	};
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

int vs_search_i16be(const uint8_t haystack[], size_t haystack_size, int16_t value, void *ctx, vs_callback callback) {
	return vs_search_i16be(haystack, haystack_size, (uint16_t)value, ctx, callback);
}

int vs_search_u16be(const uint8_t haystack[], size_t haystack_size, uint16_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = {
		value >> 8,
		value & 0xFF
	};
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

int vs_search_i32be(const uint8_t haystack[], size_t haystack_size, int32_t value, void *ctx, vs_callback callback) {
	return vs_search_u32be(haystack, haystack_size, (uint32_t)value, ctx, callback);
}

int vs_search_u32be(const uint8_t haystack[], size_t haystack_size, uint32_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = {
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >>  8) & 0xFF,
		 value        & 0xFF
	};
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

int vs_search_i64be(const uint8_t haystack[], size_t haystack_size, int64_t value, void *ctx, vs_callback callback) {
	return vs_search_u64be(haystack, haystack_size, (uint64_t)value, ctx, callback);
}

int vs_search_u64be(const uint8_t haystack[], size_t haystack_size, uint64_t value, void *ctx, vs_callback callback) {
	const uint8_t needle[] = {
		(value >> 56) & 0xFF,
		(value >> 48) & 0xFF,
		(value >> 40) & 0xFF,
		(value >> 32) & 0xFF,
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >>  8) & 0xFF,
		 value        & 0xFF
	};
	return vs_search(haystack, haystack_size, needle, sizeof(needle), ctx, callback);
}

// XXX: this is probably not right
#ifdef __STDC_IEC_559__
int vs_search_f32( const uint8_t haystack[], size_t haystack_size, float value, void *ctx, vs_callback callback) {
	return vs_search(haystack, haystack_size, (const uint8_t*)&value, sizeof(value), ctx, callback);
}

int vs_search_f64(const uint8_t haystack[], size_t haystack_size, double value, void *ctx, vs_callback callback) {
	return vs_search(haystack, haystack_size, (const uint8_t*)&value, sizeof(value), ctx, callback);
}
#if !defined(_WIN32) && !defined(_WIN64)
int vs_search_f128(const uint8_t haystack[], size_t haystack_size, long double value, void *ctx, vs_callback callback) {
	return vs_search(haystack, haystack_size, (const uint8_t*)&value, sizeof(value), ctx, callback);
}
#endif
#endif

int vs_search(const uint8_t haystack[], size_t haystack_size, const uint8_t needle[], size_t needle_size, void *ctx, vs_callback callback) {
	int status = 0;

	size_t rem = haystack_size;
	for (const void *ptr = haystack; rem >= needle_size;) {
		uint8_t *next = memmem(ptr, rem, needle, needle_size);

		if (!next) {
			break;
		}

		if ((status = callback(ctx, (size_t)(next - haystack))) != 0) {
			break;
		}

		rem -= (size_t)(next - (const uint8_t*)ptr) + needle_size;
		ptr = next + needle_size;
	}
	
	return status;
}
