/*
 *  Copyright (c) 2009-2013 Alper Akcan <alper.akcan@gmail.com>
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

#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)

#if !defined(HMEMORY_INTERNAL) || (HMEMORY_INTERNAL == 0)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#undef memset
#define memset(b, c, len) ({ \
	void *__r; \
	__r = hmemory_memset(b, c, len); \
	__r; \
})

#undef memcpy
#define memcpy(s1, s2, n) ({ \
	void *__r; \
	__r = hmemory_memcpy(s1, s2, n); \
	__r; \
})

#undef strdup
#define strdup(string) ({ \
	void *__r; \
	char __n[256]; \
	snprintf(__n, 256, "strdup-%p(%s %s:%d)", string, __FUNCTION__, __FILE__, __LINE__); \
	__r = hmemory_strdup((const char *) __n, string); \
	__r; \
})

#undef strndup
#define strndup(string, size) ({ \
	void *__r; \
	char __n[256]; \
	snprintf(__n, 256, "strndup-%p-%d(%s %s:%d)", string, size, __FUNCTION__, __FILE__, __LINE__); \
	__r = hmemory_strndup((const char *) __n, string, size); \
	__r; \
})

#undef malloc
#define malloc(size) ({ \
	void *__r; \
	char __n[256]; \
	snprintf(__n, 256, "malloc-%d(%s %s:%d)", size, __FUNCTION__, __FILE__, __LINE__); \
	__r = hmemory_malloc((const char *) __n, size); \
	__r; \
})

#undef calloc
#define calloc(nmemb, size) ({ \
	void *__r; \
	char __n[256]; \
	snprintf(__n, 256, "calloc-%d,%d(%s %s:%d)", nmemb, size, __FUNCTION__, __FILE__, __LINE__); \
	__r = hmemory_calloc((const char *) __n, nmemb, size); \
	__r; \
})

#undef realloc
#define realloc(address, size) ({ \
	void *__r; \
	char __n[256]; \
	snprintf(__n, 256, "realloc-%p,%d(%s %s:%d)", address, size, __FUNCTION__, __FILE__, __LINE__); \
	__r = hmemory_realloc((const char *) __n, address, size); \
	__r; \
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

#define hmemory_memset(a, b, c)               HMEMORY_FUNCTION_NAME(memset_actual)(a, b, c, __FUNCTION__, __FILE__, __LINE__)
#define hmemory_memcpy(a, b, c)               HMEMORY_FUNCTION_NAME(memcpy_actual)(a, b, c, __FUNCTION__, __FILE__, __LINE__)

#define hmemory_strdup(a, b)                  HMEMORY_FUNCTION_NAME(strdup_actual)(a, b, __FUNCTION__, __FILE__, __LINE__)
#define hmemory_strndup(a, b, c)              HMEMORY_FUNCTION_NAME(strndup_actual)(a, b, c, __FUNCTION__, __FILE__, __LINE__)
#define hmemory_malloc(a, b)                  HMEMORY_FUNCTION_NAME(malloc_actual)(a, b, __FUNCTION__, __FILE__, __LINE__)
#define hmemory_calloc(a, b, c)               HMEMORY_FUNCTION_NAME(calloc_actual)(a, b, c, __FUNCTION__, __FILE__, __LINE__)
#define hmemory_realloc(a, b, c)              HMEMORY_FUNCTION_NAME(realloc_actual)(a, b, c, __FUNCTION__, __FILE__, __LINE__)
#define hmemory_free(a)                       HMEMORY_FUNCTION_NAME(free_actual)(a, __FUNCTION__, __FILE__, __LINE__)

void * HMEMORY_FUNCTION_NAME(memset_actual) (void *destination, int c, size_t len, const char *func, const char *file, const int line);
void * HMEMORY_FUNCTION_NAME(memcpy_actual) (void *destination, void *source, size_t len, const char *func, const char *file, const int line);

char * HMEMORY_FUNCTION_NAME(strdup_actual) (const char *name, const char *string, const char *func, const char *file, const int line);
char * HMEMORY_FUNCTION_NAME(strndup_actual) (const char *name, const char *string, size_t size, const char *func, const char *file, const int line);
void * HMEMORY_FUNCTION_NAME(malloc_actual) (const char *name, size_t size, const char *func, const char *file, const int line);
void * HMEMORY_FUNCTION_NAME(calloc_actual) (const char *name, size_t nmemb, size_t size, const char *func, const char *file, const int line);
void * HMEMORY_FUNCTION_NAME(realloc_actual) (const char *name, void *address, size_t size, const char *func, const char *file, const int line);
void HMEMORY_FUNCTION_NAME(free_actual) (void *address, const char *func, const char *file, const int line);

#endif
