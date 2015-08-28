#include "valuescan.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define START_SET 1
#define END_SET   2

#define OFF_MAX ((off_t)~((uintmax_t)1 << (sizeof(off_t) * CHAR_BIT - 1)))
#define OFF_MIN ((off_t) ((uintmax_t)1 << (sizeof(off_t) * CHAR_BIT - 1)))

#define XCH_TO_NUM(CH) \
	((CH) >= 'A' && (CH) <= 'F' ? 10 + (CH) - 'A' : \
	 (CH) >= 'a' && (CH) <= 'f' ? 10 + (CH) - 'a' : \
	 (CH) - '0')

struct vs_options {
	const char *filename;
	off_t start;
	off_t end;
};

static void usage(int argc, char *argv[]) {
	printf("usage: %s [-f format] [-s start-offset] [-e end-offset] value [file...]\n", argc > 0 ? argv[0] : "valuescan");
}

static int print_offset(void *ctx, size_t offset) {
	const struct vs_options *options = (const struct vs_options *)ctx;

	if (options->filename) {
		printf("%s: %" PRIiPTR "\n", options->filename, options->start + offset);
	}
	else {
		printf("%" PRIiPTR "\n", options->start + offset);
	}
	return 0;
}

static int parse_offset(const char *str, off_t *valueptr) {
	char *endptr = NULL;
	long long int value = strtoll(str, &endptr, 10);
	if (*endptr) return -1;
	if (value < OFF_MIN || value > OFF_MAX) {
		errno = ERANGE;
		return -1;
	}
	if (valueptr) *valueptr = (off_t)value;
	return 0;
}

static int valuescan(const char *filename, int fd, int flags, off_t offset_start, off_t offset_end,
		const uint8_t needle[], size_t needle_size) {
	struct stat st;

	if (fstat(fd, &st) != 0) {
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		errno = EISDIR;
		return -1;
	}

	struct vs_options options = {
		.filename = filename,
		.start    = 0,
		.end      = st.st_size
	};

	if (flags & START_SET) {
		if (offset_start < 0) {
			if (-offset_start > st.st_size) {
				errno = ERANGE;
				return -1;
			}
			options.start = st.st_size - offset_start;
		}
		else {
			options.start = offset_start;
		}
	}

	if (flags & END_SET) {
		if (offset_end < 0) {
			if (-offset_end > st.st_size) {
				errno = ERANGE;
				return -1;
			}
			options.end = st.st_size - offset_end;
		}
		else {
			options.end = offset_end;
		}
	}

	if (sizeof(off_t) > sizeof(size_t) && (options.end - options.start) > (off_t)SIZE_MAX) {
		errno = ERANGE;
		return -1;
	}

	long pagesize = sysconf(_SC_PAGE_SIZE);
	if (pagesize < 0) {
		return -1;
	}

	const size_t haystack_size = (size_t)(options.end - options.start);
	const size_t map_delta     = options.start % pagesize;
	const off_t  map_offset    = options.start - map_delta;
	const size_t map_size      = haystack_size + map_delta;
	void *map_data = mmap(NULL, map_size, PROT_READ, MAP_PRIVATE, fd, map_offset);

	if (map_data == MAP_FAILED) {
		return -1;
	}

	const uint8_t *haystack = ((const uint8_t *)map_data) + map_delta;
	int status = vs_search(haystack, haystack_size, needle, needle_size, &options, &print_offset);

	munmap(map_data, map_size);

	return status;
}

size_t needle_from_value(uint8_t needle[], size_t needle_size, const char *format, const char *strvalue) {
	char *endptr = NULL;
	if (strcasecmp(format, "text") == 0 || strcasecmp(format, "txt") == 0) {
		size_t size = strlen(strvalue);
		if (size <= needle_size) {
			memcpy(needle, strvalue, size);
		}
		return size;
	}
	else if (strcasecmp(format, "hex") == 0) {
		size_t size = strlen(strvalue);
		if (size % 2 != 0) {
			errno = EINVAL;
			return SIZE_MAX;
		}
		size /= 2;
		if (size <= needle_size) {
			for (size_t i = 0; i < size; ++ i) {
				char ch1 = strvalue[i * 2];
				char ch2 = strvalue[i * 2 + 1];

				if (!isxdigit(ch1) || !isxdigit(ch2)) {
					errno = EINVAL;
					return SIZE_MAX;
				}

				uint8_t hi = XCH_TO_NUM(ch1);
				uint8_t lo = XCH_TO_NUM(ch2);

				needle[i] = (hi << 4) | lo;
			}
		}
		return size;
	}
	else if (strcasecmp(format, "i8") == 0) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT8_MIN || value > INT8_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i8(needle, needle_size, (int8_t)value);
	}
	else if (strcasecmp(format, "u8") == 0) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value > UINT8_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u8(needle, needle_size, (uint8_t)value);
	}

	// little endian values
	else if (strcasecmp(format, "i16le") == 0) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i16le(needle, needle_size, (int16_t)value);
	}
	else if (strcasecmp(format, "u16le") == 0) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u16le(needle, needle_size, (uint16_t)value);
	}
	else if (strcasecmp(format, "i32le") == 0) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i32le(needle, needle_size, (int32_t)value);
	}
	else if (strcasecmp(format, "u32le") == 0) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u32le(needle, needle_size, (uint32_t)value);
	}
	else if (strcasecmp(format, "i64le") == 0) {
		long long int value = strtoll(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i64le(needle, needle_size, (uint64_t)value);
	}
	else if (strcasecmp(format, "u64le") == 0) {
		unsigned long long int value = strtoull(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u64le(needle, needle_size, (uint64_t)value);
	}

	// big endian values
	else if (strcasecmp(format, "i16be") == 0) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i16be(needle, needle_size, (int16_t)value);
	}
	else if (strcasecmp(format, "u16be") == 0) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u16be(needle, needle_size, (uint16_t)value);
	}
	else if (strcasecmp(format, "i32be") == 0) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i32be(needle, needle_size, (int32_t)value);
	}
	else if (strcasecmp(format, "u32be") == 0) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u32be(needle, needle_size, (uint32_t)value);
	}
	else if (strcasecmp(format, "i64be") == 0) {
		long long int value = strtoll(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_i64be(needle, needle_size, (uint64_t)value);
	}
	else if (strcasecmp(format, "u64be") == 0) {
		unsigned long long int value = strtoull(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			return SIZE_MAX;
		}

		return vs_needle_from_u64be(needle, needle_size, (uint64_t)value);
	}

	// floating point values
#ifdef __STDC_IEC_559__
	// little endian
	else if (strcasecmp(format, "f32le") == 0) {
		float value = strtof(strvalue, &endptr);

		if (errno != 0) {
			return SIZE_MAX;
		}

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		return vs_needle_from_f32le(needle, needle_size, value);
	}
	else if (strcasecmp(format, "f64le") == 0) {
		double value = strtod(strvalue, &endptr);

		if (errno != 0) {
			return SIZE_MAX;
		}

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		return vs_needle_from_f64le(needle, needle_size, value);
	}

	// big endian
	else if (strcasecmp(format, "f32be") == 0) {
		float value = strtof(strvalue, &endptr);

		if (errno != 0) {
			return SIZE_MAX;
		}

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		return vs_needle_from_f32be(needle, needle_size, value);
	}
	else if (strcasecmp(format, "f64be") == 0) {
		double value = strtod(strvalue, &endptr);

		if (errno != 0) {
			return SIZE_MAX;
		}

		if (*endptr) {
			errno = EINVAL;
			return SIZE_MAX;
		}

		return vs_needle_from_f64be(needle, needle_size, value);
	}
#endif
	else {
		errno = EINVAL;
		return SIZE_MAX;
	}
}

int main(int argc, char *argv[]) {
	static struct option long_options[] = {
		{"format", required_argument, 0, 'f'},
		{"start",  required_argument, 0, 's'},
		{"end",    required_argument, 0, 'e'},
		{"help",   no_argument,       0, 'h'},
		{0,        0,                 0,  0 }
	};
	uint8_t needle[16] = { 0 };
	const char *format = "u32le";
	const char *value  = NULL;
	int flags = 0;
	off_t start_offset = 0;
	off_t end_offset   = 0;

	if (argc < 2) {
		usage(argc, argv);
		return 1;
	}

	for (;;) {
		int c = getopt_long(argc, argv, "hf:s:e:", long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'h':
			// TODO: more help
			usage(argc, argv);
			return 0;

		case 'f':
			format = optarg;
			break;

		case 's':
			if (parse_offset(optarg, &start_offset) != 0) {
				perror(optarg);
				return 1;	
			}
			flags |= START_SET;
			break;

		case 'e':
			if (parse_offset(optarg, &end_offset) != 0) {
				perror(optarg);
				return 1;
			}
			flags |= END_SET;
			break;

		case '?':
			fprintf(stderr, "*** error: unknown option -%c\n", c);
			usage(argc, argv);
			return 1;
		}
	}

	if (optind >= argc) {
		usage(argc, argv);
		return 1;
	}
	value = argv[optind];
	++ optind;

	errno = 0;
	const size_t needle_size = needle_from_value(needle, sizeof(needle), format, value);
	uint8_t *needle_buf = needle;

	if (needle_size > sizeof(needle)) {
		if (needle_size < SIZE_MAX) {
			needle_buf = malloc(needle_size);

			if (!needle_buf) {
				perror("allocating needle buffer");
				return 1;
			}

			if (needle_from_value(needle_buf, needle_size, format, value) != needle_size) {
				fprintf(stderr, "--format='%s' '%s': %s\n", format, value, strerror(errno));
				return 1;
			}
		}
		else {
			fprintf(stderr, "--format='%s' '%s': %s\n", format, value, strerror(errno));
			return 1;
		}
	}

	int status = 0;

	if (optind < argc) {
		for (int i = optind; i < argc; ++ i) {
			const char *filename = argv[i];
			int fd = open(filename, O_RDONLY, 0644);

			if (fd == -1) {
				perror(filename);
				status = 1;
				continue;
			}

			if (valuescan(filename, fd, flags, start_offset, end_offset, needle_buf, needle_size) != 0) {
				perror(filename);
				status = 1;
			}

			close(fd);
		}
	}
	else if (valuescan(NULL, 0, flags, start_offset, end_offset, needle_buf, needle_size) != 0) {
		status = 1;
	}

	if (needle_buf != needle) {
		free(needle_buf);
	}

	return status;
}
