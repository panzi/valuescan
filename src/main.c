#include "valuescan.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>
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

#ifdef _MSC_VER
#	define PRIuSZ "Iu"
#else
#	define PRIuSZ "zu"
#endif

// printfmt:
// %% -> %
// %f -> filename
// %o -> offset
// %s -> size of matched value
// %t -> format:value tuple as provided by user
// %v -> value as provided by user
// %x -> value as hex (lower case)
// %X -> value as hex (upper case)
struct vs_options {
	const char *printfmt;
	const char *filename;
	off_t start;
	off_t end;
	char  eol;
};

static bool startswith(const char *str, const char *prefix) {
	while (*prefix) {
		if (*prefix++ != *str++) {
			return false;
		}
	}
	return true;
}

static bool startswith_ignorecase(const char *str, const char *prefix) {
	while (*prefix) {
		if (tolower(*prefix++) != tolower(*str++)) {
			return false;
		}
	}
	return true;
}

static int needle_size_cmp(const void *lhs, const void *rhs) {
	const struct vs_needle *n1 = lhs;
	const struct vs_needle *n2 = rhs;
	return n2->size - n1->size;
}

static void usage(int argc, char *argv[]) {
	const char *binary = argc > 0 ? argv[0] : "valuescan";
	printf(
		"usage: %s [options] format:value [format:value...] [--] [file...]\n"
		"\n"
		"BLOB FORAMTS:\n"
		"\n"
		"\ttext .... text as given\n"
		"\thex ..... hex encoded binary\n"
		"\n"
		"NUMBER FORMATS:\n"
		"\n"
		"\tFormat | Type    | Bits |   Sign   |    Encoding\n"
		"\t-------+---------+------+----------+---------------\n"
		"\ti8     | integer |    8 | signed   |     ----\n"
		"\tu8     | integer |    8 | unsigned |     ----\n"
		"\ti16le  | integer |   16 | signed   | little endian\n"
		"\tu16le  | integer |   16 | unsigned | little endian\n"
		"\ti32le  | integer |   32 | signed   | little endian\n"
		"\tu32le  | integer |   32 | unsigned | little endian\n"
		"\ti64le  | integer |   64 | signed   | little endian\n"
		"\tu64le  | integer |   64 | unsigned | little endian\n"
		"\ti16be  | integer |   16 | signed   | big endian\n"
		"\tu16be  | integer |   16 | unsigned | big endian\n"
		"\ti32be  | integer |   32 | signed   | big endian\n"
		"\tu32be  | integer |   32 | unsigned | big endian\n"
		"\ti64be  | integer |   64 | signed   | big endian\n"
		"\tu64be  | integer |   64 | unsigned | big endian\n"
#ifdef __STDC_IEC_559__
		"\tf32le  | float   |   32 | signed   | little endian\n"
		"\tf64le  | float   |   64 | signed   | little endian\n"
		"\tf32be  | float   |   32 | signed   | big endian\n"
		"\tf64be  | float   |   64 | signed   | big endian\n"
#endif
		"\n"
		"OPTIONS:\n"
		"\t-h, --help                   print this help message\n"
		"\t-s, --start-offset=OFFSET    start scanning at OFFSET\n"
		"\t-e, --end-offset=OFFSET      end scanning at OFFSET-1\n"
		"\t-p, --print-format=FORMAT    use FORMAT for messages\n"
		"\t          %%%% ... %%\n"
		"\t          %%f ... filename\n"
		"\t          %%o ... offset\n"
		"\t          %%s ... size of matched value\n"
		"\t          %%t ... format:value tuple as provided by user\n"
		"\t          %%v ... value as provided by user\n"
		"\t          %%x ... value as hex (lower case)\n"
		"\t          %%X ... value as hex (upper case)\n"
		"\t-0, --print0                 separate lines with null bytes\n",
		binary);
}

static bool is_needle(const char *str) {
	const char *ptr = str;
	while (isalnum(*ptr)) ++ ptr;
	return (ptr - str) > 1 && ptr[0] == ':' && ptr[1] != 0;
}

static int print_offset(void *ctx, const struct vs_needle *needle, size_t offset) {
	const struct vs_options *options = (const struct vs_options *)ctx;
	const char *fmt = options->printfmt;
	const char *last = fmt;

	for (;;) {
		char ch = *fmt;

		if (!ch) {
			if (last != fmt) {
				fwrite(last, fmt - last, 1, stdout);
			}
			break;
		}
		else if (ch == '%') {
			if (last != fmt) {
				fwrite(last, fmt - last, 1, stdout);
			}
			++ fmt;
			ch = *fmt;
			switch (ch) {
			case 'f':
				if (options->filename) {
					fputs(options->filename, stdout);
				}
				++ fmt;
				break;

			case 'o':
				printf("%" PRIuSZ, options->start + offset);
				++ fmt;
				break;

			case 's':
				printf("%" PRIuSZ, needle->size);
				++ fmt;
				break;

			case 't':
				fputs((const char*)needle->ctx, stdout);
				++ fmt;
				break;

			case 'v':
				fputs(strchr((const char*)needle->ctx, ':')+1, stdout);
				++ fmt;
				break;

			case 'x':
				for (size_t i = 0; i < needle->size; ++ i) {
					printf("%02x", needle->data[i]);
				}
				++ fmt;
				break;

			case 'X':
				for (size_t i = 0; i < needle->size; ++ i) {
					printf("%02X", needle->data[i]);
				}
				++ fmt;
				break;

			default:
				fputc('%', stdout);
			}
			last = fmt;
		}
		else {
			++ fmt;
		}
	}

	fputc(options->eol, stdout);

	return 0;
}

static int parse_offset(const char *str, off_t *valueptr) {
	if (!*str) {
		errno = EINVAL;
		return -1;
	}
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
                     const char *printfmt, char eol, const struct vs_needle *needles, size_t needle_count) {
	struct stat st;

	if (fstat(fd, &st) != 0) {
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		errno = EISDIR;
		return -1;
	}

	struct vs_options options = {
		.printfmt = printfmt,
		.filename = filename,
		.start    = 0,
		.end      = st.st_size,
		.eol      = eol
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
	int status = vs_search(haystack, haystack_size, needles, needle_count, &options, &print_offset);

	munmap(map_data, map_size);

	return status;
}

static int parse_needle(const char *str, struct vs_needle *needle) {
	char *endptr = NULL;
	const char *strvalue = strchr(str, ':') + 1;
	size_t   size = 0;
	uint8_t *data = NULL;

	if (startswith_ignorecase(str, "text:")) {
		size = strlen(strvalue);
		data = malloc(size);
		if (!data) {
			return -1;
		}
		memcpy(data, strvalue, size);
	}
	else if (startswith_ignorecase(str, "hex:")) {
		size = strlen(strvalue);
		if (size % 2 != 0) {
			errno = EINVAL;
			return -1;
		}
		size /= 2;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		
		for (size_t i = 0; i < size; ++ i) {
			char ch1 = strvalue[i * 2];
			char ch2 = strvalue[i * 2 + 1];

			if (!isxdigit(ch1) || !isxdigit(ch2)) {
				free(data);
				errno = EINVAL;
				return -1;
			}

			uint8_t hi = XCH_TO_NUM(ch1);
			uint8_t lo = XCH_TO_NUM(ch2);

			data[i] = (hi << 4) | lo;
		}
	}
	else if (startswith_ignorecase(str, "i8:")) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT8_MIN || value > INT8_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 1;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i8(data, size, (int8_t)value);
	}
	else if (startswith_ignorecase(str, "u8:")) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT8_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 1;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u8(data, size, (uint8_t)value);
	}

	// little endian values
	else if (startswith_ignorecase(str, "i16le:")) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 2;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i16le(data, size, (int16_t)value);
	}
	else if (startswith_ignorecase(str, "u16le:")) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 2;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u16le(data, size, (uint16_t)value);
	}
	else if (startswith_ignorecase(str, "i32le:")) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 4;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i32le(data, size, (int32_t)value);
	}
	else if (startswith_ignorecase(str, "u32le:")) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 4;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u32le(data, size, (uint32_t)value);
	}
	else if (startswith_ignorecase(str, "i64le:")) {
		long long int value = strtoll(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 8;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i64le(data, size, (uint64_t)value);
	}
	else if (startswith_ignorecase(str, "u64le:")) {
		unsigned long long int value = strtoull(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 8;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u64le(data, size, (uint64_t)value);
	}

	// big endian values
	else if (startswith_ignorecase(str, "i16be:")) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT16_MIN || value > INT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 2;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i16be(data, size, (int16_t)value);
	}
	else if (startswith_ignorecase(str, "u16be:")) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT16_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 2;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u16be(data, size, (uint16_t)value);
	}
	else if (startswith_ignorecase(str, "i32be:")) {
		long int value = strtol(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT32_MIN || value > INT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 4;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i32be(data, size, (int32_t)value);
	}
	else if (startswith_ignorecase(str, "u32be:")) {
		unsigned long int value = strtoul(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT32_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 4;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u32be(data, size, (uint32_t)value);
	}
	else if (startswith_ignorecase(str, "i64be:")) {
		long long int value = strtoll(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value < INT64_MIN || value > INT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 8;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_i64be(data, size, (uint64_t)value);
	}
	else if (startswith_ignorecase(str, "u64be:")) {
		unsigned long long int value = strtoull(strvalue, &endptr, 10);

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		if (value > UINT64_MAX) {
			errno = ERANGE;
			return -1;
		}

		size = 8;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_u64be(data, size, (uint64_t)value);
	}

	// floating point values
#ifdef __STDC_IEC_559__
	// little endian
	else if (startswith_ignorecase(str, "f32le:")) {
		float value = strtof(strvalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		size = 4;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_f32le(data, size, value);
	}
	else if (startswith_ignorecase(str, "f64le:")) {
		double value = strtod(strvalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		size = 8;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_f64le(data, size, value);
	}

	// big endian
	else if (startswith_ignorecase(str, "f32be:")) {
		float value = strtof(strvalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		size = 4;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_f32be(data, size, value);
	}
	else if (startswith_ignorecase(str, "f64be:")) {
		double value = strtod(strvalue, &endptr);

		if (errno != 0) {
			return -1;
		}

		if (*endptr) {
			errno = EINVAL;
			return -1;
		}

		size = 8;
		data = malloc(size);
		if (!data) {
			return -1;
		}
		vs_needle_from_f64be(data, size, value);
	}
#endif
	else {
		errno = EINVAL;
		return -1;
	}

	needle->size = size;
	needle->data = data;
	needle->ctx  = (void*)str;

	return 0;
}

int main(int argc, char *argv[]) {
	int flags = 0;
	const char *printfmt = NULL;
	off_t start_offset = 0;
	off_t end_offset   = 0;
	const char **filenames    = NULL;
	struct vs_needle *needles = NULL;
	size_t file_count   = 0;
	size_t needle_count = 0;
	size_t filenames_capacity = 0;
	size_t needles_capacity   = 0;
	int status = 0;
	char eol = '\n';

	if (argc < 2) {
		usage(argc, argv);
		goto error;
	}

	// not using getopt because of custom needle option format
	bool opts_ended = false;
	for (int argind = 1; argind < argc; ++ argind) {
		const char *arg = argv[argind];

		if (opts_ended) {
			goto filename_arg;
		}
		else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--start-offset") == 0) {
			if (++ argind == argc) {
				fprintf(stderr, "*** error: missing argument to option %s\n", arg);
				goto error;
			}
			if (parse_offset(argv[argind], &start_offset) != 0) {
				perror(argv[argind]);
				goto error;	
			}
			flags |= START_SET;
		}
		else if (startswith(arg, "--start-offset=")) {
			if (parse_offset(strchr(arg, '=')+1, &start_offset) != 0) {
				perror(arg);
				goto error;	
			}
			flags |= START_SET;
		}
		else if (strcmp(arg, "-e") == 0 || strcmp(arg, "--end-offset") == 0) {
			if (++ argind == argc) {
				fprintf(stderr, "*** error: missing argument to option %s\n", arg);
				goto error;
			}
			if (parse_offset(argv[argind], &end_offset) != 0) {
				perror(argv[argind]);
				goto error;	
			}
			flags |= END_SET;
		}
		else if (startswith(arg, "--end-offset=")) {
			if (parse_offset(strchr(arg, '=')+1, &end_offset) != 0) {
				perror(arg);
				goto error;	
			}
			flags |= END_SET;
		}
		else if (strcmp(arg, "-p") == 0 || strcmp(arg, "--print-format") == 0) {
			if (++ argind == argc) {
				fprintf(stderr, "*** error: missing argument to option %s\n", arg);
				goto error;
			}
			printfmt = argv[argind];
		}
		else if (startswith(arg, "--print-format=")) {
			printfmt = strchr(arg, '=') + 1;
		}
		else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			usage(argc, argv);
			goto end;
		}
		else if (strcmp(arg, "-0") == 0 || strcmp(arg, "--print0") == 0) {
			eol = 0;
		}
		else if (strcmp(arg, "--") == 0) {
			opts_ended = true;
			++ argind;
			break;
		}
		else if (startswith(arg, "-")) {
			fprintf(stderr, "*** error: unknown option %s\n", arg);
			goto error;
		}
		else if (is_needle(arg)) {
			struct vs_needle needle = { 0 };
			if (parse_needle(arg, &needle) != 0) {
				perror(arg);
				goto error;
			}
			if (needle_count == needles_capacity) {
				needles_capacity += 32;
				struct vs_needle *buf = realloc(needles, sizeof(struct vs_needle) * needles_capacity);
				if (!buf) {
					perror("allocating needle buffer");
					goto error;
				}
				needles = buf;
				memset(needles + needle_count, 0, (needles_capacity - needle_count) * sizeof(struct vs_needle));
			}
			needles[needle_count ++] = needle;
		}
		else {
		filename_arg:
			if (file_count == filenames_capacity) {
				filenames_capacity += 64;
				const char **buf = realloc(filenames, sizeof(char*) * filenames_capacity);
				if (!buf) {
					perror("allocating fiename buffer");
					goto error;
				}
				filenames = buf;
				memset(filenames + file_count, 0, (filenames_capacity - file_count) * sizeof(char*));
			}
			filenames[file_count ++] = arg;
		}
	}

	if (needle_count == 0) {
		fprintf(stderr, "*** error: no needles given\n");
		goto error;
	}

	// biggest match first
	qsort(needles, needle_count, sizeof(struct vs_needle), needle_size_cmp);

	if (file_count > 0) {
		if (!printfmt) {
			printfmt = "%f:%o: %t";
		}

		for (size_t i = 0; i < file_count; ++ i) {
			const char *filename = filenames[i];
			int fd = open(filename, O_RDONLY, 0644);

			if (fd == -1) {
				perror(filename);
				status = 1;
				continue;
			}

			if (valuescan(filename, fd, flags, start_offset, end_offset, printfmt, eol, needles, needle_count) != 0) {
				perror(filename);
				status = 1;
			}

			close(fd);
		}
	}
	else if (valuescan(NULL, 0, flags, start_offset, end_offset, printfmt ? printfmt : "%o: %t", eol, needles, needle_count) != 0) {
		status = 1;
	}

	goto end;

error:
	status = 1;

end:
	if (filenames) {
		free(filenames);
	}

	if (needles) {
		for (size_t i = 0; i < needle_count; ++ i) {
			free((void*)needles[i].data);
		}
		free(needles);
	}

	return status;
}
