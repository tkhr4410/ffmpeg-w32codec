// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "common.h"

void debug_printf(const char *fmt, ...)
{
	char buf[4096];
	va_list argp;

	va_start(argp, fmt);
	wvsprintfA(buf, fmt, argp);
	va_end(argp);

	OutputDebugStringA(buf);
}
