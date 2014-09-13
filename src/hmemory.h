/*
 *  Copyright (c) 2008-2013 Alper Akcan <alper.akcan@gmail.com>
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */

#if !defined(HMEMORY_H)
#define HMEMORY_H 1

#if !defined(HMEMORY_DISABLE_YIELD)
#define HMEMORY_DISABLE_YIELD			0
#endif

#if !defined(HMEMORY_ENABLE_CALLSTACK)
#define HMEMORY_ENABLE_CALLSTACK		1
#endif

#if defined(__DARWIN__) && (__DARWIN__ == 1)
#undef HMEMORY_ENABLE_CALLSTACK
#define HMEMORY_ENABLE_CALLSTACK		0
#endif

#if !defined(HMEMORY_REPORT_CALLSTACK)
#define HMEMORY_REPORT_CALLSTACK		1
#endif
#define HMEMORY_REPORT_CALLSTACK_NAME		"hmemory_report_callstack"

#if !defined(HMEMORY_ASSERT_ON_ERROR)
#define HMEMORY_ASSERT_ON_ERROR			1
#endif
#define HMEMORY_ASSERT_ON_ERROR_NAME		"hmemory_assert_on_error"

#if !defined(HMEMORY_CORRUPTION_CHECK_INTERVAL)
#define HMEMORY_CORRUPTION_CHECK_INTERVAL	5000
#endif
#define HMEMORY_CORRUPTION_CHECK_INTERVAL_NAME	"hmemory_corruption_check_interval"

#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)

#if !defined(HMEMORY_INTERNAL) || (HMEMORY_INTERNAL == 0)

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define HMEMORY_DEBUG_NAME_MAX			256

#undef memcpy
#define memcpy(s1, s2, n) ({ \
	void *__hmemory_r; \
	__hmemory_r = hmemory_memcpy((s1), (s2), (n)); \
	__hmemory_r; \
})

#undef asprintf
#define asprintf(strp, fmt...) ({ \
	int __hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "asprintf(%s %s:%d)", __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_asprintf(__hmemory_n, strp, fmt); \
	__hmemory_r; \
})

#undef vasprintf
#define vasprintf(strp, fmt, ap) ({ \
	int __hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "vasprintf(%s %s:%d)", __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_vasprintf(__hmemory_n, strp, fmt); \
	__hmemory_r; \
})

#undef strdup
#define strdup(string) ({ \
	void *__hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "strdup-%p(%s %s:%d)", string, __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_strdup(__hmemory_n, string); \
	__hmemory_r; \
})

#undef strndup
#define strndup(string, size) ({ \
	void *__hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "strndup-%p-%lld(%s %s:%d)", string, (long long) size, __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_strndup(__hmemory_n, string, size); \
	__hmemory_r; \
})

#undef malloc
#define malloc(size) ({ \
	void *__hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "malloc-%lld(%s %s:%d)", (long long) (size), __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_malloc((__hmemory_n), (size)); \
	__hmemory_r; \
})

#undef calloc
#define calloc(nmemb, size) ({ \
	void *__hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "calloc-%lld,%lld(%s %s:%d)", (long long) nmemb, (long long) size, __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_calloc(__hmemory_n, nmemb, size); \
	__hmemory_r; \
})

#undef realloc
#define realloc(address, size) ({ \
	void *__hmemory_r; \
	char __hmemory_n[HMEMORY_DEBUG_NAME_MAX]; \
	snprintf(__hmemory_n, HMEMORY_DEBUG_NAME_MAX, "realloc-%p,%lld(%s %s:%d)", address, (long long) size, __FUNCTION__, __FILE__, __LINE__); \
	__hmemory_r = hmemory_realloc(__hmemory_n, address, size); \
	__hmemory_r; \
})

#undef free
#define free(address) ({ \
	hmemory_free(address); \
})

#endif

#define HMEMORY_FUNCTION_NAME(function) hmemory_ ## function ## _debug

#else

#define HMEMORY_FUNCTION_NAME(function) hmemory_ ## function

#endif

#define hmemory_memcpy(a, b, c)               HMEMORY_FUNCTION_NAME(memcpy_actual)(__FUNCTION__, __FILE__, __LINE__, a, b, c)

#define hmemory_asprintf(a, b...)             HMEMORY_FUNCTION_NAME(asprintf_actual)(__FUNCTION__, __FILE__, __LINE__, a, b)
#define hmemory_vasprintf(a, b, c)            HMEMORY_FUNCTION_NAME(vasprintf_actual)(__FUNCTION__, __FILE__, __LINE__, a, b, c)

#define hmemory_strdup(a, b)                  HMEMORY_FUNCTION_NAME(strdup_actual)(__FUNCTION__, __FILE__, __LINE__, a, b)
#define hmemory_strndup(a, b, c)              HMEMORY_FUNCTION_NAME(strndup_actual)(__FUNCTION__, __FILE__, __LINE__, a, b, c)

#define hmemory_malloc(a, b)                  HMEMORY_FUNCTION_NAME(malloc_actual)(__FUNCTION__, __FILE__, __LINE__, a, b)
#define hmemory_calloc(a, b, c)               HMEMORY_FUNCTION_NAME(calloc_actual)(__FUNCTION__, __FILE__, __LINE__, a, b, c)
#define hmemory_realloc(a, b, c)              HMEMORY_FUNCTION_NAME(realloc_actual)(__FUNCTION__, __FILE__, __LINE__, a, b, c)
#define hmemory_free(a)                       HMEMORY_FUNCTION_NAME(free_actual)(__FUNCTION__, __FILE__, __LINE__, a)

#ifdef __cplusplus
extern "C" {
#endif

void * HMEMORY_FUNCTION_NAME(memcpy_actual) (const char *func, const char *file, const int line, void *destination, const void *source, size_t len);

int HMEMORY_FUNCTION_NAME(asprintf_actual) (const char *func, const char *file, const int line, const char *name, char **strp, const char *fmt, ...);
int HMEMORY_FUNCTION_NAME(vasprintf_actual) (const char *func, const char *file, const int line, const char *name, char **strp, const char *fmt, va_list ap);

char * HMEMORY_FUNCTION_NAME(strdup_actual) (const char *func, const char *file, const int line, const char *name, const char *string);
char * HMEMORY_FUNCTION_NAME(strndup_actual) (const char *func, const char *file, const int line, const char *name, const char *string, size_t size);

void * HMEMORY_FUNCTION_NAME(malloc_actual) (const char *func, const char *file, const int line, const char *name, size_t size);
void * HMEMORY_FUNCTION_NAME(calloc_actual) (const char *func, const char *file, const int line, const char *name, size_t nmemb, size_t size);
void * HMEMORY_FUNCTION_NAME(realloc_actual) (const char *func, const char *file, const int line, const char *name, void *address, size_t size);
void HMEMORY_FUNCTION_NAME(free_actual) (const char *func, const char *file, const int line, void *address);

#ifdef __cplusplus
}
#endif

#endif
