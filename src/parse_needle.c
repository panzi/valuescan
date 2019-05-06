#include "parse_needle.h"

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

enum vs_needle_type {
	VS_TEXT,
	VS_HEX,
	VS_FILE,
	VS_INT,
#ifdef __STDC_IEC_559__
	VS_FLOAT,
#endif
};

enum vs_byte_order {
	VS_LITTLE_ENDIAN,
	VS_BIG_ENDIAN,
};

enum vs_sign {
	VS_SIGNED,
	VS_UNSIGNED,
};

struct vs_needle_type_info {
	enum vs_needle_type type;
	enum vs_sign sign;
	size_t size;
	enum vs_byte_order byte_order;
};

static bool startswith_ignorecase(const char *str, const char *prefix) {
	while (*prefix) {
		if (tolower(*prefix++) != tolower(*str++)) {
			return false;
		}
	}
	return true;
}

static int parse_half_hex_byte(char ch) {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	} else if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	} else {
		errno = EINVAL;
		return -1;
	}
}

static int parse_hex_byte(const char *str) {
	int hi = parse_half_hex_byte(str[0]);
	if (hi < 0) {
		return -1;
	}
	int lo = parse_half_hex_byte(str[1]);
	if (lo < 0) {
		return -1;
	}
	return (hi << 4) | lo;
}

static const char *parse_string(const char *str, char *buf, size_t *sizeptr) {
	size_t bufsize = buf == NULL || sizeptr == NULL ? 0 : *sizeptr;
	size_t size = 0;

	if (*str == '"') {
		++ str;
		while (*str != '"') {
			if (!*str) {
				errno = EINVAL;
				return NULL;
			}
			if (*str == '\\') {
				++ str;
				switch (*str) {
					case 'x': case 'X':
						++ str;
						int value = parse_hex_byte(str);
						if (value < 0) {
							return NULL;
						}
						++ str;
						if (bufsize > size) {
							buf[size] = (uint8_t)value;
						}
						break;

					case '\\':
						if (bufsize > size) {
							buf[size] = '\\';
						}
						break;

					case '"':
						if (bufsize > size) {
							buf[size] = '"';
						}
						break;

					case 'n':
						if (bufsize > size) {
							buf[size] = '\n';
						}
						break;

					case 't':
						if (bufsize > size) {
							buf[size] = '\t';
						}
						break;

					case 'r':
						if (bufsize > size) {
							buf[size] = '\r';
						}
						break;

					case 'a':
						if (bufsize > size) {
							buf[size] = '\a';
						}
						break;

					case 'v':
						if (bufsize > size) {
							buf[size] = '\v';
						}
						break;

					case 'f':
						if (bufsize > size) {
							buf[size] = '\f';
						}
						break;

					case '0':
						if (bufsize > size) {
							buf[size] = '\0';
						}
						break;

					default:
						errno = EINVAL;
						return NULL;
				}
			} else if (bufsize > size) {
				buf[size] = *str;
			}
			++ size;
			++ str;
		}
		++ str;
	} else {
		while (isalnum(*str) || isspace(*str) || *str == '-' || *str == '+' || *str == '_' || *str == '.') {
			if (bufsize > size) {
				buf[size] = *str;
			}
			++ size;
			++ str;
		}
	}

	if (sizeptr != NULL) {
		*sizeptr = size;
	}
	return str;
}

static const char *parse_needle_type(const char *str, struct vs_needle_type_info *info) {
	if (startswith_ignorecase(str, "text:")) {
		info->type = VS_TEXT;
		return str + 5;
	} else if (startswith_ignorecase(str, "hex:")) {
		info->type = VS_HEX;
		return str + 4;
	} else if (startswith_ignorecase(str, "file:")) {
		info->type = VS_FILE;
		return str + 5;
	}

	switch (*str) {
		case 'i': case 'I':
			info->type = VS_INT;
			info->sign = VS_SIGNED;
			break;

		case 'u': case 'U':
			info->type = VS_INT;
			info->sign = VS_UNSIGNED;
			break;

#ifdef __STDC_IEC_559__
		case 'f': case 'F':
			info->type = VS_FLOAT;
			info->sign = VS_SIGNED;
			break;
#endif

		default:
			errno = EINVAL;
			return NULL;
	}

	++ str;
	char *endptr = NULL;
	unsigned long int size = strtoul(str, &endptr, 10);

	if (endptr == str) {
		errno = EINVAL;
		return NULL;
	}

	switch (size) {
		case 8: case 16:
#ifdef __STDC_IEC_559__
			if (info->type == VS_FLOAT) {
				errno = EINVAL;
				return NULL;
			}
#endif
			info->size = size / 8;
			break;
		
		case 32: case 64:
			info->size = size / 8;
			break;

		default:
			errno = ERANGE;
			return NULL;
	}

	str = endptr;

	if (startswith_ignorecase(str, "le")) {
		info->byte_order = VS_LITTLE_ENDIAN;
		str += 2;
	} else if (startswith_ignorecase(str, "be")) {
		info->byte_order = VS_BIG_ENDIAN;
		str += 2;
	} else if (*str == ':') {
#if BYTE_ORDER == BIG_ENDIAN
		info->byte_order = VS_BIG_ENDIAN;
#else
		info->byte_order = VS_LITTLE_ENDIAN;
#endif
	} else {
		errno = EINVAL;
		return NULL;
	}

	if (*str != ':') {
		errno = EINVAL;
		return NULL;
	}

	return str + 1;
}

const char *parse_needle_item(const char *str, struct vs_needle_type_info *info, uint8_t buf[], size_t bufsize) {
	str = parse_needle_type(str, info);
	if (str == NULL) {
		return NULL;
	}
	char *endptr = NULL;
	switch (info->type) {
		case VS_INT:
			switch (info->sign)
			{
				case VS_SIGNED:
				{
					long long int value = strtoll(str, &endptr, 10);
					if (endptr == str) {
						errno = EINVAL;
						return NULL;
					}
					str = endptr;
					switch (info->size)
					{
						case 1:
							if (value < INT8_MIN || value > INT8_MAX) {
								errno = ERANGE;
								return NULL;
							}
							vs_needle_from_i8(buf, bufsize, (int8_t)value);
							break;
					
						case 2:
							if (value < INT16_MIN || value > INT16_MAX) {
								errno = ERANGE;
								return NULL;
							}
							if (info->byte_order == VS_LITTLE_ENDIAN) {
								vs_needle_from_i16le(buf, bufsize, (int16_t)value);
							} else {
								vs_needle_from_i16be(buf, bufsize, (int16_t)value);
							}
							break;

						case 4:
							if (value < INT32_MIN || value > INT32_MAX) {
								errno = ERANGE;
								return NULL;
							}
							if (info->byte_order == VS_LITTLE_ENDIAN) {
								vs_needle_from_i32le(buf, bufsize, (int32_t)value);
							} else {
								vs_needle_from_i32be(buf, bufsize, (int32_t)value);
							}
							break;

						case 8:
							if (value < INT64_MIN || value > INT64_MAX) {
								errno = ERANGE;
								return NULL;
							}
							if (info->byte_order == VS_LITTLE_ENDIAN) {
								vs_needle_from_i64le(buf, bufsize, (int64_t)value);
							} else {
								vs_needle_from_i64be(buf, bufsize, (int64_t)value);
							}
							break;

						default:
							assert(false);
					}
					break;
				}
				case VS_UNSIGNED:
				{
					unsigned long long int value = strtoull(str, &endptr, 10);
					if (endptr == str) {
						errno = EINVAL;
						return NULL;
					}
					str = endptr;
					switch (info->size)
					{
						case 1:
							if (value > UINT8_MAX) {
								errno = ERANGE;
								return NULL;
							}
							vs_needle_from_u8(buf, bufsize, (uint8_t)value);
							break;
					
						case 2:
							if (value > UINT16_MAX) {
								errno = ERANGE;
								return NULL;
							}
							if (info->byte_order == VS_LITTLE_ENDIAN) {
								vs_needle_from_u16le(buf, bufsize, (uint16_t)value);
							} else {
								vs_needle_from_u16be(buf, bufsize, (uint16_t)value);
							}
							break;

						case 4:
							if (value > UINT32_MAX) {
								errno = ERANGE;
								return NULL;
							}
							if (info->byte_order == VS_LITTLE_ENDIAN) {
								vs_needle_from_u32le(buf, bufsize, (uint32_t)value);
							} else {
								vs_needle_from_u32be(buf, bufsize, (uint32_t)value);
							}
							break;

						case 8:
							if (value > UINT64_MAX) {
								errno = ERANGE;
								return NULL;
							}
							if (info->byte_order == VS_LITTLE_ENDIAN) {
								vs_needle_from_u64le(buf, bufsize, (uint64_t)value);
							} else {
								vs_needle_from_u64be(buf, bufsize, (uint64_t)value);
							}
							break;

						default:
							assert(false);
					}
					break;
				}
				default:
					assert(false);
			}
			break;

#ifdef __STDC_IEC_559__
		case VS_FLOAT:
			errno = 0;
			switch (info->size)
			{
				case 4:
				{
					float value = strtof(str, &endptr);
					if (errno != 0) {
						return NULL;
					}

					if (endptr == str) {
						errno = EINVAL;
						return NULL;
					}
					str = endptr;
					if (info->byte_order == VS_LITTLE_ENDIAN) {
						vs_needle_from_f32le(buf, bufsize, value);
					} else {
						vs_needle_from_f32be(buf, bufsize, value);
					}
					break;
				}

				case 8:
				{
					double value = strtod(str, &endptr);
					if (errno != 0) {
						return NULL;
					}

					if (endptr == str) {
						errno = EINVAL;
						return NULL;
					}
					str = endptr;
					if (info->byte_order == VS_LITTLE_ENDIAN) {
						vs_needle_from_f64le(buf, bufsize, value);
					} else {
						vs_needle_from_f64be(buf, bufsize, value);
					}
					break;
				}
			}
			break;
#endif
		case VS_TEXT:
		{
			size_t size = size;
			str = parse_string(str, (char*)buf, &size);
			if (str == NULL) {
				return NULL;
			}
			info->size = size;
			break;
		}
		case VS_HEX:
			info->size = 0;
			while (isspace(*str))
				++ str;

			while (*str && *str != ',') {
				int value = parse_hex_byte(str);
				if (value < 0) {
					return NULL;
				}
				if (bufsize > info->size) {
					buf[info->size] = (uint8_t)value;
				}
				++ info->size;
				str += 2;
				while (isspace(*str))
					++ str;
			}
			break;

		case VS_FILE:
		{
			size_t size = 0;
			if (parse_string(str, NULL, &size) == NULL) {
				return NULL;
			}
			char *filename = calloc(1, size + 1);
			if (!filename) {
				return NULL;
			}
			str = parse_string(str, filename, &size);
			if (str == NULL) {
				free(filename);
				return NULL;
			}
			struct stat st;
			if (stat(filename, &st) != 0) {
				free(filename);
				return NULL;
			}
			info->size = st.st_size;
			if (info->size <= bufsize) {
				FILE *fp = fopen(filename, "rb");
				if (!fp) {
					perror(filename);
					free(filename);
					return NULL;
				}
				if (fread(buf, bufsize, 1, fp) != 1) {
					perror(filename);
					free(filename);
					return NULL;
				}
				fclose(fp);
			}
			free(filename);
			break;
		}

		default:
			assert(false);
	}

	return str;
}

size_t vs_parse_needle_data(const char *str, uint8_t buf[], size_t bufsize) {
	size_t size = 0;
	const char *ptr = str;
	while (isspace(*ptr))
		++ ptr;
	while (*ptr) {
		struct vs_needle_type_info info;
		ptr = bufsize >= size ?
			parse_needle_item(ptr, &info, buf + size, bufsize - size) :
			parse_needle_item(ptr, &info, NULL, 0);
		if (ptr == NULL) {
			return 0;
		}
		if (size > SIZE_MAX - info.size) {
			errno = ENOMEM;
			return 0;
		}
		size += info.size;
		while (isspace(*ptr))
			++ ptr;
		if (*ptr != ',')
			break;
		++ ptr;
		while (isspace(*ptr))
			++ ptr;
	}
	if (*ptr) {
		errno = EINVAL;
		return 0;
	}
	return size;
}

int vs_parse_needle(const char *str, struct vs_needle *needle) {
	size_t size = vs_parse_needle_data(str, NULL, 0);
	if (size == 0) {
		return -1;
	}
	uint8_t *data = calloc(1, size);
	if (!data) {
		return -1;
	}
	if (vs_parse_needle_data(str, data, size) != size) {
		assert(false);
		return -1;
	}
	needle->data = data;
	needle->size = size;
	return 0;
}