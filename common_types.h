#pragma once

typedef unsigned long long u64;
typedef long long s64;
typedef void (*code)();

#include "types.h"

typedef union {
	u32 integer;
	void *ptr;
} __attribute__((transparent_union)) arg;
