#include "valuescan.h"

#include <endian.h>
#include <string.h>

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);

size_t vs_needle_from_i8(uint8_t needle[], size_t needle_size, int8_t value) {
	return vs_needle_from_u8(needle, needle_size, (uint8_t)value);
}

size_t vs_needle_from_u8(uint8_t needle[], size_t needle_size, uint8_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] = (uint8_t)value;
	}
	return sizeof(value);
}

size_t vs_needle_from_i16le(uint8_t needle[], size_t needle_size, int16_t value) {
	return vs_needle_from_u16le(needle, needle_size, (uint16_t)value);
}

size_t vs_needle_from_u16le(uint8_t needle[], size_t needle_size, uint16_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] =  value       & 0xFF;
		needle[1] = (value >> 8) & 0xFF;
	}
	return sizeof(value);
}

size_t vs_needle_from_i32le(uint8_t needle[], size_t needle_size, int32_t value) {
	return vs_needle_from_u32le(needle, needle_size, (uint32_t)value);
}

size_t vs_needle_from_u32le(uint8_t needle[], size_t needle_size, uint32_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] =  value        & 0xFF;
		needle[1] = (value >>  8) & 0xFF;
		needle[2] = (value >> 16) & 0xFF;
		needle[3] = (value >> 24) & 0xFF;
	}
	return sizeof(value);
}

size_t vs_needle_from_i64le(uint8_t needle[], size_t needle_size, int64_t value) {
	return vs_needle_from_u64le(needle, needle_size, (uint64_t)value);
}

size_t vs_needle_from_u64le(uint8_t needle[], size_t needle_size, uint64_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] =  value        & 0xFF;
		needle[1] = (value >>  8) & 0xFF;
		needle[2] = (value >> 16) & 0xFF;
		needle[3] = (value >> 24) & 0xFF;
		needle[4] = (value >> 32) & 0xFF;
		needle[5] = (value >> 40) & 0xFF;
		needle[6] = (value >> 48) & 0xFF;
		needle[7] = (value >> 56) & 0xFF;
	}
	return sizeof(value);
}

size_t vs_needle_from_i16be(uint8_t needle[], size_t needle_size, int16_t value) {
	return vs_needle_from_u16be(needle, needle_size, (uint16_t)value);
}

size_t vs_needle_from_u16be(uint8_t needle[], size_t needle_size, uint16_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] = (value >> 8) & 0xFF;
		needle[1] =  value       & 0xFF;
	}
	return sizeof(value);
}

size_t vs_needle_from_i32be(uint8_t needle[], size_t needle_size, int32_t value) {
	return vs_needle_from_u32be(needle, needle_size, (uint32_t)value);
}

size_t vs_needle_from_u32be(uint8_t needle[], size_t needle_size, uint32_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] = (value >> 24) & 0xFF;
		needle[1] = (value >> 16) & 0xFF;
		needle[2] = (value >>  8) & 0xFF;
		needle[3] =  value        & 0xFF;
	}
	return sizeof(value);
}

size_t vs_needle_from_i64be(uint8_t needle[], size_t needle_size, int64_t value) {
	return vs_needle_from_u64be(needle, needle_size, (uint64_t)value);
}

size_t vs_needle_from_u64be(uint8_t needle[], size_t needle_size, uint64_t value) {
	if (needle_size >= sizeof(value)) {
		needle[0] = (value >> 56) & 0xFF;
		needle[1] = (value >> 48) & 0xFF;
		needle[2] = (value >> 40) & 0xFF;
		needle[3] = (value >> 32) & 0xFF;
		needle[4] = (value >> 24) & 0xFF;
		needle[5] = (value >> 16) & 0xFF;
		needle[6] = (value >>  8) & 0xFF;
		needle[7] =  value        & 0xFF;
	}
	return sizeof(value);
}

#ifdef __STDC_IEC_559__
#	if (BYTE_ORDER != BIG_ENDIAN) && (BYTE_ORDER != LITTLE_ENDIAN)
#		error unsupported host byte order
#	endif

size_t vs_needle_from_f32le(uint8_t needle[], size_t needle_size, float value) {
	if (needle_size >= sizeof(value)) {
#if BYTE_ORDER == LITTLE_ENDIAN
		memcpy(needle, &value, sizeof(value));
#else
		const uint8_t *ptr = (const uint8_t*)&value;
		needle[0] = ptr[3];
		needle[1] = ptr[2];
		needle[2] = ptr[1];
		needle[3] = ptr[0];
#endif
	}
	return sizeof(value);
}

size_t vs_needle_from_f64le(uint8_t needle[], size_t needle_size, double value) {
	if (needle_size >= sizeof(value)) {
#if BYTE_ORDER == LITTLE_ENDIAN
		memcpy(needle, &value, sizeof(value));
#else
		const uint8_t *ptr = (const uint8_t*)&value;
		needle[0] = ptr[7];
		needle[1] = ptr[6];
		needle[2] = ptr[5];
		needle[3] = ptr[4];
		needle[4] = ptr[3];
		needle[5] = ptr[2];
		needle[6] = ptr[1];
		needle[7] = ptr[0];
#endif
	}
	return sizeof(value);
}

size_t vs_needle_from_f32be(uint8_t needle[], size_t needle_size, float value) {
	if (needle_size >= sizeof(value)) {
#if BYTE_ORDER == BIG_ENDIAN
		memcpy(needle, &value, sizeof(value));
#else
		const uint8_t *ptr = (const uint8_t*)&value;
		needle[0] = ptr[3];
		needle[1] = ptr[2];
		needle[2] = ptr[1];
		needle[3] = ptr[0];
#endif
	}
	return sizeof(value);
}

size_t vs_needle_from_f64be(uint8_t needle[], size_t needle_size, double value) {
	if (needle_size >= sizeof(value)) {
#if BYTE_ORDER == BIG_ENDIAN
		memcpy(needle, &value, sizeof(value));
#else
		const uint8_t *ptr = (const uint8_t*)&value;
		needle[0] = ptr[7];
		needle[1] = ptr[6];
		needle[2] = ptr[5];
		needle[3] = ptr[4];
		needle[4] = ptr[3];
		needle[5] = ptr[2];
		needle[6] = ptr[1];
		needle[7] = ptr[0];
#endif
	}
	return sizeof(value);
}
#endif

int vs_search(const uint8_t haystack[], size_t haystack_size, const struct vs_needle needles[], size_t needle_count, void *ctx, vs_callback callback) {
	const uint8_t *end = haystack + haystack_size;

	for (const uint8_t *ptr = haystack; ptr < end; ++ ptr) {
		const size_t rem = (size_t)(end - ptr);
		for (size_t i = 0; i < needle_count; ++ i) {
			const struct vs_needle *needle = needles + i;
			if (needle->size <= rem && memcmp(needle->data, ptr, needle->size) == 0) {
				int status = callback(ctx, needle, (size_t)(ptr - haystack));
				if (status != 0) {
					return status;
				}
				break;
			}
		}
	}

	return 0;
}
