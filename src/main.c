#include "valuescan.h"
#include "parse_needle.h"

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

static int needle_size_cmp(const void *lhs, const void *rhs) {
	const struct vs_needle *n1 = lhs;
	const struct vs_needle *n2 = rhs;
	return n2->size - n1->size;
}

static void usage(int argc, char *argv[]) {
	const char *binary = argc > 0 ? argv[0] : "valuescan";
	printf(
		"Usage: %s [options] format:value[,format:value...]... [--] [file...]\n"
		"\n"
		"BLOB FORAMTS:\n"
		"\n"
		"\ttext .... either simple string [-+\\._a-zA-Z0-9]* or C-like quoted string\n"
		"\thex ..... hex encoded binary\n"
		"\tfile .... read needle from given file (filename is encoded like text)\n"
		"\n"
		"NUMBER FORMATS:\n"
		"\n"
		"\tFormat | Type    | Bits |   Sign   |  Byte Order\n"
		"\t------ | ------- | ---- | -------- | --------------\n"
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
		"\t-0, --print0                 separate lines with null bytes\n"
		"\n"
		"EXAMPLES:\n"
		"\n"
		"\tFind a 1024x1024 or 2048x2048 square:\n"
		"\n"
		"\t\t%s u32le:1024,u32le:1024 u32le:2048,u32le:2048 -- file.bin\n"
		"\n"
		"Report bugs to: https://github.com/panzi/valuescan/issues\n",
		binary, binary);
}

static bool is_needle(const char *str) {
	// non-empty string that is not an option and not a path
	// (for a very loose definition of what is a path)
	return *str && *str != '-' &&
		*str != '/' && *str != '\\' && *str != '.' &&
		!(isalpha(*str) && str[1] == ':') &&
		strchr(str, ':') != NULL;
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
			struct vs_needle needle = { 0, .ctx = (void*)arg };
			if (vs_parse_needle(arg, &needle) != 0) {
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
	else if (valuescan(NULL, STDIN_FILENO, flags, start_offset, end_offset, printfmt ? printfmt : "%o: %t", eol, needles, needle_count) != 0) {
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
