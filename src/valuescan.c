#include "valuescan.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);

int vs_search_i8(int fd, int8_t value, void *ctx, vs_callback callback) {
	return vs_search_u8(fd, (uint8_t)value, ctx, callback);
}

int vs_search_u8(int fd, uint8_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = { value };
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

int vs_search_i16le(int fd, int16_t value, void *ctx, vs_callback callback) {
	return vs_search_i16le(fd, (uint16_t)value, ctx, callback);
}

int vs_search_u16le(int fd, uint16_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = {
		value & 0xFF,
		value >> 8
	};
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

int vs_search_i32le(int fd, int32_t value, void *ctx, vs_callback callback) {
	return vs_search_u32le(fd, (uint32_t)value, ctx, callback);
}

int vs_search_u32le(int fd, uint32_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = {
		 value        & 0xFF,
		(value >>  8) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 24) & 0xFF
	};
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

int vs_search_i64le(int fd, int64_t value, void *ctx, vs_callback callback) {
	return vs_search_u64le(fd, (uint64_t)value, ctx, callback);
}

int vs_search_u64le(int fd, uint64_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = {
		 value        & 0xFF,
		(value >>  8) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 24) & 0xFF,
		(value >> 32) & 0xFF,
		(value >> 40) & 0xFF,
		(value >> 48) & 0xFF,
		(value >> 56) & 0xFF
	};
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

int vs_search_i16be(int fd, int16_t value, void *ctx, vs_callback callback) {
	return vs_search_i16be(fd, (uint16_t)value, ctx, callback);
}

int vs_search_u16be(int fd, uint16_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = {
		value >> 8,
		value & 0xFF
	};
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

int vs_search_i32be(int fd, int32_t value, void *ctx, vs_callback callback) {
	return vs_search_u32be(fd, (uint32_t)value, ctx, callback);
}

int vs_search_u32be(int fd, uint32_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = {
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >>  8) & 0xFF,
		 value        & 0xFF
	};
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

int vs_search_i64be(int fd, int64_t value, void *ctx, vs_callback callback) {
	return vs_search_u64be(fd, (uint64_t)value, ctx, callback);
}

int vs_search_u64be(int fd, uint64_t value, void *ctx, vs_callback callback) {
	const uint8_t data[] = {
		(value >> 56) & 0xFF,
		(value >> 48) & 0xFF,
		(value >> 40) & 0xFF,
		(value >> 32) & 0xFF,
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >>  8) & 0xFF,
		 value        & 0xFF
	};
	return vs_search(fd, data, sizeof(data), ctx, callback);
}

// XXX: this is probably not right
#ifdef __STDC_IEC_559__
int vs_search_f32( int fd, float value, void *ctx, vs_callback callback) {
	return vs_search(fd, (const uint8_t*)&value, sizeof(value), ctx, callback);
}

int vs_search_f64(int fd, double value, void *ctx, vs_callback callback) {
	return vs_search(fd, (const uint8_t*)&value, sizeof(value), ctx, callback);
}
#if !defined(_WIN32) && !defined(_WIN64)
int vs_search_f128(int fd, long double value, void *ctx, vs_callback callback) {
	return vs_search(fd, (const uint8_t*)&value, sizeof(value), ctx, callback);
}
#endif
#endif

int vs_search(int fd, const uint8_t data[], size_t size, void *ctx, vs_callback callback) {
	int status = 0;
	struct stat st;
	uint8_t *filedata = NULL;

	if (fstat(fd, &st) != 0) {
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		errno = EISDIR;
		return -1;
	}

	if (st.st_size == 0) {
		return 0;
	}
	
	filedata = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if (filedata == MAP_FAILED) {
		return -1;
	}

	size_t rem = st.st_size;
	for (void *ptr = filedata; rem > 0;) {
		uint8_t *next = memmem(ptr, rem, data, size);

		if (!next) {
			break;
		}

		if ((status = callback(ctx, (off_t)(next - filedata))) != 0) {
			break;
		}

		rem -= (size_t)(next - (const uint8_t*)ptr);
		ptr = next + size;
	}

	munmap(filedata, st.st_size);
	
	return status;
}
