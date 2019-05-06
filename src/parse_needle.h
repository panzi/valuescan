#ifndef PARSE_NEEDLE_H
#define PARSE_NEEDLE_H
#pragma once

#include "valuescan.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t vs_parse_needle_data(const char *str, uint8_t buf[], size_t bufsize);
int    vs_parse_needle(const char *str, struct vs_needle *needle);

#ifdef __cplusplus
}
#endif

#endif