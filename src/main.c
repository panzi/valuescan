#include "valuescan.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>

static void usage(int argc, char *argv[]) {
	printf("usage: %s [-f format] value [file...]\n", argc > 0 ? argv[0] : "valuescan");
}

static int print_offset(void *ctx, off_t offset) {
	if (ctx) {
		printf("%s: %" PRIiPTR "\n", (const char *)ctx, offset);
	}
	else {
		printf("%" PRIiPTR "\n", offset);
	}
	return 0;
}

static int valuescan(const char *filename, int fd, const char *format, const char *svalue) {
	void *ctx = (void*)filename;
	char *endptr = NULL;

	if (!*svalue) {
		errno = EINVAL;
		return -1;
	}

	errno = 0;
	if (strcasecmp(format, "i8") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT8_MIN || value > INT8_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i8(fd, (int8_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u8") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT8_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u8(fd, (uint8_t)value, ctx, print_offset);
	}

	// little endian values
	else if (strcasecmp(format, "i16le") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i16le(fd, (int16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u16le") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u16le(fd, (uint16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i32le") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i32le(fd, (int32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u32le") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u32le(fd, (uint32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i64le") == 0) {
		long long int value = strtoll(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i64le(fd, (uint64_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u64le") == 0) {
		unsigned long long int value = strtoull(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u64le(fd, (uint64_t)value, ctx, print_offset);
	}

	// big endian values
	else if (strcasecmp(format, "i16be") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i16be(fd, (int16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u16be") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u16be(fd, (uint16_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i32be") == 0) {
		long int value = strtol(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i32be(fd, (int32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u32be") == 0) {
		unsigned long int value = strtoul(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u32be(fd, (uint32_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "i64be") == 0) {
		long long int value = strtoll(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_i64be(fd, (uint64_t)value, ctx, print_offset);
	}
	else if (strcasecmp(format, "u64be") == 0) {
		unsigned long long int value = strtoull(svalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		return vs_search_u64be(fd, (uint64_t)value, ctx, print_offset);
	}

	// floating point values
#ifdef __STDC_IEC_559__
	else if (strcasecmp(format, "f32") == 0) {
		float value = strtof(svalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		return vs_search_f32(fd, value, ctx, print_offset);
	}
	else if (strcasecmp(format, "f64") == 0) {
		double value = strtod(svalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		return vs_search_f64(fd, value, ctx, print_offset);
	}
#if !defined(_WIN32) && !defined(_WIN64)
	else if (strcasecmp(format, "f128") == 0) {
		long double value = strtold(svalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		return vs_search_f128(fd, value, ctx, print_offset);
	}
#endif
#endif
	else {
		errno = EINVAL;
		return -1;
	}
}

int main(int argc, char *argv[]) {
	const char *format = "u32le";
	const char *value  = NULL;
	static struct option long_options[] = {
		{"format", required_argument, 0, 'f'},
		{"help",   no_argument,       0, 'h'},
		{0,        0,                 0,  0 }
	};

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

			if (valuescan(filename, fd, format, value) != 0) {
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
	else if (valuescan(NULL, 0, format, value) != 0) {
		status = 1;
	}
	return status;
}
