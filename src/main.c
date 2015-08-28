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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define START_SET 1
#define END_SET   2

#define OFF_MAX ((off_t)~((uintmax_t)1 << (sizeof(off_t) * CHAR_BIT - 1)))
#define OFF_MIN ((off_t) ((uintmax_t)1 << (sizeof(off_t) * CHAR_BIT - 1)))

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
		const char *format, const char *svalue) {
	void *ctx = (void*)filename;
	char *endptr = NULL;
	struct stat st;

	if (!*svalue) {
		errno = EINVAL;
		return -1;
	}

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

	int status = 0;
	const size_t haystack_size = (size_t)(options.end - options.start);
	const uint8_t *haystack = mmap(NULL, haystack_size, PROT_READ, MAP_PRIVATE, fd, options.start);

	if (haystack == MAP_FAILED) {
		return -1;
	}

	errno = 0;
	if (strcasecmp(format, "i8") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT8_MIN || value > INT8_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i8(haystack, haystack_size, (int8_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u8") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value > UINT8_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u8(haystack, haystack_size, (uint8_t)value, ctx, print_offset);
	}

	// little endian values
	else if (strcasecmp(format, "i16le") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i16le(haystack, haystack_size, (int16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u16le") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u16le(haystack, haystack_size, (uint16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i32le") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i32le(haystack, haystack_size, (int32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u32le") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u32le(haystack, haystack_size, (uint32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i64le") == 0) {
		long long int value = strtoll(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i64le(haystack, haystack_size, (uint64_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u64le") == 0) {
		unsigned long long int value = strtoull(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u64le(haystack, haystack_size, (uint64_t)value, ctx, print_offset);
	}

	// big endian values
	else if (strcasecmp(format, "i16be") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i16be(haystack, haystack_size, (int16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u16be") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u16be(haystack, haystack_size, (uint16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i32be") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i32be(haystack, haystack_size, (int32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u32be") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u32be(haystack, haystack_size, (uint32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i64be") == 0) {
		long long int value = strtoll(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_i64be(haystack, haystack_size, (uint64_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u64be") == 0) {
		unsigned long long int value = strtoull(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			goto error;
		}

		status = vs_search_u64be(haystack, haystack_size, (uint64_t)value, ctx, print_offset);
	}

	// floating point values
#ifdef __STDC_IEC_559__
	else if (strcasecmp(format, "f32") == 0) {
		float value = strtof(svalue, &endptr);

		if (errno != 0) {
			goto error;
		}

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		status = vs_search_f32(haystack, haystack_size, value, ctx, print_offset);
	}
	else if (strcasecmp(format, "f64") == 0) {
		double value = strtod(svalue, &endptr);

		if (errno != 0) {
			goto error;
		}

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		status = vs_search_f64(haystack, haystack_size, value, ctx, print_offset);
	}
#if !defined(_WIN32) && !defined(_WIN64)
	else if (strcasecmp(format, "f128") == 0) {
		long double value = strtold(svalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			goto error;
		}

		status = vs_search_f128(haystack, haystack_size, value, ctx, print_offset);
	}
#endif
#endif
	else {
		errno = EINVAL;
		goto error;
	}

	goto end;

error:
	status = -1;

end:
	munmap((void*)haystack, haystack_size);

	return status;
}

int main(int argc, char *argv[]) {
	const char *format = "u32le";
	const char *value  = NULL;
	static struct option long_options[] = {
		{"format", required_argument, 0, 'f'},
		{"start",  required_argument, 0, 's'},
		{"end",    required_argument, 0, 'e'},
		{"help",   no_argument,       0, 'h'},
		{0,        0,                 0,  0 }
	};
	int flags = 0;
	off_t start_offset = 0;
	off_t end_offset   = 0;

	if (argc < 2) {
		usage(argc, argv);
		return 1;
	}

	for (;;) {
		int c = getopt_long(argc, argv, "hf:", long_options, NULL);

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

			if (valuescan(filename, fd, flags, start_offset, end_offset, format, value) != 0) {
				if (errno == EINVAL || errno == ERANGE) {
					fprintf(stderr, "%s\n", strerror(errno));
				}
				else {
					perror(filename);
				}
				status = 1;
			}

			close(fd);
		}
	}
	else if (valuescan(NULL, 0, flags, start_offset, end_offset, format, value) != 0) {
		status = 1;
	}
	return status;
}
