/*
 *  Copyright (c) 2008-2013 Alper Akcan <alper.akcan@gmail.com>
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#if defined(__DARWIN__) && (__DARWIN__ == 1)
#include <mach/mach_time.h>
#endif

#define HMEMORY_INTERNAL			1
#define HMEMORY_CALLSTACK_MAX			128

#define HMEMORY_HASH_UTHASH			0
#define HMEMORY_HASH_KHASH			1

#include "hmemory.h"
#include "khash.h"
#include "uthash.h"

#if defined(HMEMORY_ENABLE_CALLSTACK) && (HMEMORY_ENABLE_CALLSTACK == 1)
#include <bfd.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

static pthread_mutex_t debugf_mutex = PTHREAD_MUTEX_INITIALIZER;

#define hdebug_lock() pthread_mutex_lock(&debugf_mutex);
#define hdebug_unlock() pthread_mutex_unlock(&debugf_mutex);

#if 0
#define hdebugf(a...) { \
	hdebug_lock(); \
	fprintf(stderr, "hmemory::debug: "); \
	fprintf(stderr, a); \
	fprintf(stderr, " (%s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	hdebug_unlock(); \
}
#else
#define hdebugf(a...)
#endif

#define hinfof(a...) { \
	fprintf(stderr, "(hmemory:%d) ", getpid()); \
	fprintf(stderr, a); \
	fprintf(stderr, "\n"); \
}

#define herrorf(a...) { \
	hdebug_lock(); \
	fprintf(stderr, "hmemory::error: "); \
	fprintf(stderr, a); \
	fprintf(stderr, " (%s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	hdebug_unlock(); \
}

#define hassert(a) { \
	unsigned int v; \
	v = hmemory_getenv_int(HMEMORY_ASSERT_ON_ERROR_NAME); \
	if (v == (unsigned int) -1) { \
		v = HMEMORY_ASSERT_ON_ERROR; \
	} \
	if (v) { \
		assert(a); \
	} else { \
		herrorf(# a); \
	} \
}

#define hassertf(a...) { \
	hdebug_lock(); \
	fprintf(stderr, "hmemory::assert: "); \
	fprintf(stderr, a); \
	fprintf(stderr, " (%s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	assert(0); \
	hdebug_unlock(); \
}

#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)

#define hmemory_lock()			pthread_mutex_lock(&hmemory_mutex)
#define hmemory_unlock()		pthread_mutex_unlock(&hmemory_mutex)
#define hmemory_self_pthread()		pthread_self()

static pthread_cond_t hmemory_cond	= PTHREAD_COND_INITIALIZER;
static pthread_mutex_t hmemory_mutex	= PTHREAD_MUTEX_INITIALIZER;

static intptr_t  hmemory_signature	= 0xdeadbeef;
static intptr_t  hmemory_signature_size = sizeof(hmemory_signature);

static inline int hmemory_getenv_int (const char *name);
static inline int debug_dump_callstack (const char *prefix);
static inline int debug_memory_add (const char *name, void *address, size_t size, const char *command, const char *func, const char *file, const int line);
static inline int debug_memory_check (void *address, const char *command, const char *func, const char *file, const int line);
static inline int debug_memory_overlap (void *s1, const void *s2, size_t len, const char *command, const char *func, const char *file, const int line);
static inline int debug_memory_del (void *address, const char *command, const char *func, const char *file, const int line);

#else

static unsigned int hmemory_signature_size = 0;

#define debug_memory_unused() \
	(void) func; \
	(void) file; \
	(void) line;
#define debug_memory_add(a...)		(void) name; (void) command; debug_memory_unused()
#define debug_memory_del(a...)		debug_memory_unused()
#define debug_memory_check(a...)	debug_memory_unused()
#define debug_memory_overlap(a...)      debug_memory_unused()

#endif

static inline void * malloc_actual (const char *command, const char *func, const char *file, const int line, const char *name, size_t size)
{
	void *rc;
	size += hmemory_signature_size * 2;
	rc = malloc(size);
	if (rc == NULL) {
		herrorf("malloc failed");
		return NULL;
	}
	debug_memory_add(name, rc, size, command, func, file, line);
	debug_memory_check(rc, command, func, file, line);
	return rc + hmemory_signature_size;
}

static inline void free_actual (const char *command, const char *func, const char *file, const int line, void *address)
{
	void *addr;
	(void) command;
	if (address == NULL) {
		return;
	}
	addr = address - hmemory_signature_size;
	debug_memory_check(addr, command, func, file, line);
	debug_memory_del(addr, command, func, file, line);
	free(addr);
}

void * HMEMORY_FUNCTION_NAME(memcpy_actual) (const char *func, const char *file, const int line, void *s1, const void *s2, size_t len)
{
	debug_memory_overlap(s1, s2, len, "memcpy", func, file, line);
	return memcpy(s1, s2, len);
}

int HMEMORY_FUNCTION_NAME(getline_actual) (const char *func, const char *file, const int line, const char *name, char **strp, size_t *n, FILE *stream)
{
	int rc;
	if (*strp != NULL) {
		free_actual("getline", func, file, line, *strp);
		*strp = NULL;
		*n = 0;
	}
	rc = getline(strp, n, stream);
	if (rc < 0 && errno == EINVAL) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hdebug_lock();
		hinfof("getline failed");
		hinfof("    at: %s %s:%d", func, file, line);
		debug_dump_callstack("       ");
		hdebug_unlock();
		hassert((rc >= 0) && "getline failed");
#endif
	}
	if (*strp != NULL) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		void *tmp;
		tmp = malloc_actual("getline", func, file, line, name, strlen(*strp) + 1);
		memcpy(tmp, *strp, strlen(*strp) + 1);
		free(*strp);
		*strp = tmp;
#else
		(void) name;
		(void) func;
		(void) file;
		(void) line;
#endif
	}
	return rc;
}


int HMEMORY_FUNCTION_NAME(asprintf_actual) (const char *func, const char *file, const int line, const char *name, char **strp, const char *fmt, ...)
{
	int rc;
	va_list ap;
	va_start(ap, fmt);
	rc = vasprintf(strp, fmt, ap);
	if (rc < 0) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hdebug_lock();
		hinfof("asprintf failed");
		hinfof("    at: %s %s:%d", func, file, line);
		debug_dump_callstack("       ");
		hdebug_unlock();
		hassert((rc >= 0) && "asprintf failed");
#endif
	} else {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		void *tmp;
		tmp = malloc_actual("asprintf", func, file, line, name, strlen(*strp) + 1);
		memcpy(tmp, *strp, strlen(*strp) + 1);
		free(*strp);
		*strp = tmp;
#else
		(void) name;
		(void) func;
		(void) file;
		(void) line;
#endif
	}
	va_end(ap);
	return rc;
}

int HMEMORY_FUNCTION_NAME(vasprintf_actual) (const char *func, const char *file, const int line, const char *name, char **strp, const char *fmt, va_list ap)
{
	int rc;
	rc = vasprintf(strp, fmt, ap);
	if (rc < 0) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hdebug_lock();
		hinfof("vasprintf failed");
		hinfof("    at: %s %s:%d", func, file, line);
		debug_dump_callstack("       ");
		hdebug_unlock();
		hassert((rc >= 0) && "vasprintf failed");
#endif
	} else {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		void *tmp;
		tmp = malloc_actual("vasprintf", func, file, line, name, strlen(*strp) + 1);
		memcpy(tmp, *strp, strlen(*strp) + 1);
		free(*strp);
		*strp = tmp;
#else
		(void) name;
		(void) func;
		(void) file;
		(void) line;
#endif
	}
	return rc;
}

char * HMEMORY_FUNCTION_NAME(strdup_actual) (const char *func, const char *file, const int line, const char *name, const char *string)
{
	void *rc;
	if (string == NULL) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hdebug_lock();
		hinfof("strdup with invalid argument '%p'", string);
		hinfof("    at: %s %s:%d", func, file, line);
		debug_dump_callstack("       ");
		hdebug_unlock();
		hassert((string != NULL) && "invalid strdup parameter");
#endif
		return NULL;
	}
	rc = strdup(string);
	if (rc == NULL) {
		herrorf("strdup failed");
	}
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
	{
		void *tmp;
		tmp = malloc_actual("strdup", func, file, line, name, strlen(rc) + 1);
		memcpy(tmp, rc, strlen(rc) + 1);
		free(rc);
		rc = tmp;
	}
#else
	(void) name;
	(void) func;
	(void) file;
	(void) line;
#endif
	return rc;

}

char * HMEMORY_FUNCTION_NAME(strndup_actual) (const char *func, const char *file, const int line, const char *name, const char *string, size_t size)
{
	void *rc;
	if (string == NULL) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hdebug_lock();
		hinfof("strdup with invalid argument '%p'", string);
		hinfof("    at: %s %s:%d", func, file, line);
		debug_dump_callstack("       ");
		hdebug_unlock();
		hassert((string != NULL) && "invalid strndup parameter");
#endif
		return NULL;
	}
	rc = strndup(string, size);
	if (rc == NULL) {
		herrorf("strndup failed");
	}
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
	{
		void *tmp;
		tmp = malloc_actual("strndup", func, file, line, name, strlen(rc) + 1);
		memcpy(tmp, rc, strlen(rc) + 1);
		free(rc);
		rc = tmp;
	}
#else
	(void) name;
	(void) func;
	(void) file;
	(void) line;
#endif
	return rc;
}

void * HMEMORY_FUNCTION_NAME(malloc_actual) (const char *func, const char *file, const int line, const char *name, size_t size)
{
	void *rc;
	rc = malloc_actual("malloc", func, file, line, name, size);
	if (rc == NULL) {
		herrorf("malloc_actual failed");
		return NULL;
	}
	return rc;
}

void * HMEMORY_FUNCTION_NAME(calloc_actual) (const char *func, const char *file, const int line, const char *name, size_t nmemb, size_t size)
{
	void *rc;
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
	rc = malloc_actual("calloc", func, file, line, name, nmemb * size);
	if (rc == NULL) {
		herrorf("malloc actual failed");
		return NULL;
	}
	rc = memset(rc, 0, nmemb * size);
	if (rc == NULL) {
		herrorf("memset actual failed");
		return NULL;
	}
#else
	(void) name;
	(void) func;
	(void) file;
	(void) line;
	rc = calloc(nmemb, size);
	if (rc == NULL) {
		herrorf("calloc failed");
		return NULL;
	}
#endif
	return rc;
}

void * HMEMORY_FUNCTION_NAME(realloc_actual) (const char *func, const char *file, const int line, const char *name, void *address, size_t size)
{
	void *rc;
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
	void *addr;
	if (address == NULL) {
		rc = malloc_actual("realloc", func, file, line, name, size);
		if (rc == NULL) {
			herrorf("malloc_actual failed");
			return NULL;
		}
		return rc;
	}
	size += hmemory_signature_size * 2;
	addr = address - hmemory_signature_size;
	debug_memory_check(addr, "realloc", func, file, line);
	debug_memory_del(addr, "realloc", func, file, line);
	rc = realloc(addr, size);
	if (rc == NULL) {
		debug_memory_add(name, addr, size, "realloc", func, file, line);
		herrorf("realloc failed");
		return NULL;
	}
	debug_memory_add(name, rc, size, "realloc", func, file, line);
	debug_memory_check(rc, "realloc", func, file, line);
	return rc + hmemory_signature_size;
#else
	(void) name;
	(void) func;
	(void) file;
	(void) line;
	rc = realloc(address, size);
	if (rc == NULL) {
		herrorf("realloc failed");
		return NULL;
	}
#endif
	return rc;
}

void HMEMORY_FUNCTION_NAME(free_actual) (const char *func, const char *file, const int line, void *address)
{
	free_actual("free", func, file, line, address);
}

#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)

static inline int hmemory_getenv_int (const char *name)
{
	int r;
	const char *e;
	if (name == NULL) {
		return -1;
	}
	e = getenv(name);
	if (e == NULL) {
		return -1;
	}
	r = atoi(e);
	return r;
}

static inline unsigned long long debug_getclock (void)
{
	struct timespec ts;
	unsigned long long tsec;
	unsigned long long tusec;
	unsigned long long _clock;
#if defined(__DARWIN__) && (__DARWIN__ == 1)
	(void) ts;
	(void) tsec;
	(void) tusec;
	_clock = mach_absolute_time();
	_clock /= 1000 * 1000;
#elif defined(__LINUX__) && (__LINUX__ == 1)
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		return 0;
	}
	tsec = ((unsigned long long) ts.tv_sec) * 1000;
	tusec = ((unsigned long long) ts.tv_nsec) / 1000 / 1000;
	_clock = tsec + tusec;
#else
	#error "unknown os"
#endif
	return _clock;
}

struct stackinfo {
	const char *file;
	const char *func;
	unsigned int line;
	void *func_addr;
};

static inline int debug_dump_callstack (const char *prefix)
{
#if defined(HMEMORY_ENABLE_CALLSTACK) && (HMEMORY_ENABLE_CALLSTACK == 1)
	#ifndef bfd_get_section_flags
		#define bfd_get_section_flags(bfd, ptr) ((void) bfd, (ptr)->flags)
		#define bfd_get_section_size(ptr) ((ptr)->size)
		#define bfd_get_section_vma(bfd, ptr) ((void) bfd, (ptr)->vma)
	#endif

	int i;
	int rc;
	int size;
	int frames;
	char **strs;
	bfd *bfd;
	bfd_vma ofs;
	bfd_vma start;
	Dl_info dlinfo;
	asymbol **syms;
	asection *secp;
	unsigned int v;
	const char *fname;
	const char *func;
	unsigned int line;
	struct stackinfo stackinfo;
	void *callstack[HMEMORY_CALLSTACK_MAX];
	v = hmemory_getenv_int(HMEMORY_REPORT_CALLSTACK_NAME);
	if (v == (unsigned int) -1) {
		v = HMEMORY_REPORT_CALLSTACK;
	}
	if (v == 0) {
		return 0;
	}
	frames = backtrace(callstack, HMEMORY_CALLSTACK_MAX);
	strs = backtrace_symbols(callstack, frames);
	for (i = 1; i < frames; i++) {
		if (dladdr(callstack[i], &dlinfo)) {
			memset(&stackinfo, 0, sizeof(struct stackinfo));
			stackinfo.func = dlinfo.dli_sname;
			stackinfo.func_addr = dlinfo.dli_saddr;
			bfd = bfd_openr(dlinfo.dli_fname, NULL);
			if (bfd == NULL) {
				continue;
			}
			rc = bfd_check_format(bfd, bfd_object);
			if (rc == 0) {
				bfd_close(bfd);
				continue;
			}
			size = bfd_get_symtab_upper_bound(bfd);
			if (size <= 0) {
				bfd_close(bfd);
				continue;
			}
			syms = malloc(size);
			if (syms == NULL) {
				bfd_close(bfd);
				continue;
			}
			rc = bfd_canonicalize_symtab(bfd, syms);
			if (rc <= 0) {
				free(syms);
				bfd_close(bfd);
				continue;
			}
			#define ELF_DYNAMIC	0x40
			if (bfd->flags & ELF_DYNAMIC) {
				ofs = callstack[i] - dlinfo.dli_fbase;
			} else {
				ofs = callstack[i] - (void *) 0;
			}
			for (secp = bfd->sections; secp != NULL; secp = secp->next) {
				if (!(bfd_get_section_flags(bfd, secp) & SEC_ALLOC)) {
					continue;
				}
				start = bfd_get_section_vma(bfd, secp);
				if (ofs < start) {
					continue;
				}
				size = bfd_get_section_size(secp);
				if (ofs >= start + size) {
					continue;
				}
				if (bfd_find_nearest_line(bfd, secp, syms, ofs - start, &fname, &func, &line)) {
					stackinfo.file = fname;
					if (func != NULL) {
						stackinfo.func = func;
					}
					stackinfo.line = line;
					if (!stackinfo.func_addr && stackinfo.func) {
						asymbol **asymp;
						for (asymp = syms; *asymp; asymp++) {
							if (strcmp(bfd_asymbol_name(*asymp), stackinfo.func) == 0) {
								stackinfo.func_addr = bfd_asymbol_value (*asymp) + (void *) 0;
								break;
							}
						}
					}
				}

				break;
			}
			hinfof("%s%p: %s (%s:%d)", prefix, callstack[i], (stackinfo.file == NULL) ? "(null)" : ((strrchr(stackinfo.file, '/') == NULL) ? stackinfo.file : (strrchr(stackinfo.file, '/') + 1)), stackinfo.func, stackinfo.line);
			free(syms);
			bfd_close(bfd);
		}
	}
#if 0
	for (i = 0; i < frames; i++) {
		hinfof("%s%s", prefix, strs[i]);
	}
#endif
	free(strs);
#else
	(void) prefix;
#endif
	return 0;
}

#if !defined(MAX)
#define MAX(a, b)				(((a) > (b)) ? (a) : (b))
#endif

#if defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
#define hmemory_int64_hash_func(key) (khint32_t)(((uint64_t) key)>>33^((uint64_t) key)^((uint64_t) key)<<11)
KHASH_INIT(memory, void *, struct hmemory_memory *, 1, hmemory_int64_hash_func, kh_int64_hash_equal);
#endif

struct hmemory_memory {
	void *address;
	const char *func;
	const char *file;
	int line;
	size_t size;
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	UT_hash_handle hh;
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
#endif
	char name[0];
};

static pthread_t hmemory_thread;
static int hmemory_worker_started		= 0;
static int hmemory_worker_running		= 0;

#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
static struct hmemory_memory *debug_memory	= NULL;
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
static khash_t(memory) *debug_memory		= NULL;
#endif
static unsigned long long memory_peak		= 0;
static unsigned long long memory_current	= 0;
static unsigned long long memory_total		= 0;

static int debug_memory_add (const char *name, void *address, size_t size, const char *command, const char *func, const char *file, const int line)
{
	unsigned int s;
	struct hmemory_memory *m;
#if defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	int rc;
	khiter_t k;
#endif
	if (address == NULL) {
		return 0;
	}
	hmemory_lock();
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	HASH_FIND_PTR(debug_memory, &address, m);
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	k = kh_get(memory, debug_memory, address);
	if (k == kh_end(debug_memory)) {
		m = NULL;
	} else {
		m = kh_val(debug_memory, k);
	}
#endif
	if (m != NULL) {
		hdebug_lock();
		hinfof("%s with invalid memory (%p)", command, address);
		hinfof("    at: %s (%s:%d)", func, file, line);
		hinfof("  ");
		hinfof("  if it is certain that program is memory bug free, then hmemory");
		hinfof("  may have a serious bug that needs to be fixed urgent. please ");
		hinfof("  inform author");
		hinfof("    at: alper.akcan@gmail.com");
		hdebug_unlock();
		hassert((m == NULL) && "invalid memory");
		hmemory_unlock();
		return -1;
	}
	s = sizeof(struct hmemory_memory) + strlen(name) + 1;
	m = malloc(s);
	memset(m, 0, s);
	memcpy(m->name, name, strlen(name) + 1);
	m->address = address;
	m->size = size;
	m->func = func;
	m->file = file;
	m->line = line;
	memcpy(m->address, &hmemory_signature, hmemory_signature_size);
	memcpy(m->address + m->size - hmemory_signature_size, &hmemory_signature, hmemory_signature_size);
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	HASH_ADD_PTR(debug_memory, address, m);
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	k = kh_put(memory, debug_memory, address, &rc);
	if (rc == -1) {
		hdebug_lock();
		hinfof("%s with invalid memory (%p)", command, address);
		hinfof("    at: %s (%s:%d)", func, file, line);
		hinfof("  ");
		hinfof("  if it is certain that program is memory bug free, then hmemory");
		hinfof("  may have a serious bug that needs to be fixed urgent. please ");
		hinfof("  inform author");
		hinfof("    at: alper.akcan@gmail.com");
		hdebug_unlock();
		hassert((rc == -1) && "invalid memory key");
		hmemory_unlock();
		return -1;
	}
	kh_value(debug_memory, k) = m;
#endif
	hdebugf("%s added memory: %s, address: %p, size: %zd, func: %s, file: %s, line: %d", command, m->name, m->address, m->size, m->func, m->file, m->line);
	memory_total += (size - (hmemory_signature_size * 2));
	memory_current += (size - (hmemory_signature_size * 2));
	memory_peak = MAX(memory_peak, memory_current);
	hmemory_unlock();
	return 0;
}

static int debug_memory_check_actual (void *address, const char *command, const char *func, const char *file, const int line)
{
	int rcu;
	int rco;
	struct hmemory_memory *m;
#if defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	khiter_t k;
#endif
	if (address == NULL) {
		return 0;
	}
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	HASH_FIND_PTR(debug_memory, &address, m);
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	k = kh_get(memory, debug_memory, address);
	if (k == kh_end(debug_memory)) {
		m = NULL;
	} else {
		m = kh_val(debug_memory, k);
	}
#endif
	if (m != NULL) {
		goto found_m;
	}
	hdebug_lock();
	hinfof("%s with invalid address (%p)", command, address);
	hinfof("    at: %s (%s:%d)", func, file, line);
	debug_dump_callstack("       ");
	hinfof("  ");
	hinfof("  if it is certain that program is memory bug free, then hmemory");
	hinfof("  may have a serious bug that needs to be fixed urgent. please ");
	hinfof("  inform author");
	hinfof("    at: alper.akcan@gmail.com");
	hdebug_unlock();
	hassert((m != NULL) && "invalid address");
	return -1;
found_m:
	hdebug_lock();
	rcu = memcmp(m->address, &hmemory_signature, hmemory_signature_size);
	if (rcu != 0) {
		hinfof("%s with corrupted address (%p), underflow", command, address);
		hinfof("    at: %s (%s:%d)", func, file, line);
		debug_dump_callstack("       ");
	}
	rco = memcmp(m->address + m->size - hmemory_signature_size, &hmemory_signature, hmemory_signature_size);
	if (rco != 0) {
		hinfof("%s with corrupted address (%p), overflow", command, address);
		hinfof("    at: %s (%s:%d)", func, file, line);
		debug_dump_callstack("       ");
	}
	hdebug_unlock();
	hassert(((rcu == 0) && (rco == 0)) && "memory corruption");
	return 0;
}

static int debug_memory_check (void *address, const char *command, const char *func, const char *file, const int line)
{
	hmemory_lock();
	debug_memory_check_actual(address, command, func, file, line);
	hmemory_unlock();
	return 0;
}

static int debug_memory_overlap (void *s1, const void *s2, size_t len, const char *command, const char *func, const char *file, const int line)
{
	void *e1;
	e1 = s1 + len;
	if (s2 >= s1 && s2 <= e1) {
		hdebug_lock();
		hinfof("%s with overlapping memory", command);
		hinfof("    at: %s (%s:%d)", func, file, line);
		debug_dump_callstack("       ");
		hdebug_unlock();
		hassert((e1 <= s2) && "memory overlap");
	}
	return 0;
}

static int debug_memory_del (void *address, const char *command, const char *func, const char *file, const int line)
{
#if defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	khiter_t k;
#endif
	struct hmemory_memory *m;
	if (address == NULL) {
		return 0;
	}
	hmemory_lock();
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	HASH_FIND_PTR(debug_memory, &address, m);
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	k = kh_get(memory, debug_memory, address);
	if (k == kh_end(debug_memory)) {
		m = NULL;
	} else {
		m = kh_val(debug_memory, k);
	}
#endif
	if (m != NULL) {
		goto found_m;
	}
	hdebug_lock();
	hinfof("%s with invalid address (%p)", command, address);
	hinfof("    at: %s (%s:%d)", func, file, line);
	debug_dump_callstack("       ");
	hinfof("  ");
	hinfof("  if it is certain that program is memory bug free, then hmemory");
	hinfof("  may have a serious bug that needs to be fixed urgent. please ");
	hinfof("  inform author");
	hinfof("    at: alper.akcan@gmail.com");
	hdebug_unlock();
	hassert((m != NULL) && "invalid address");
	hmemory_unlock();
	return -1;
found_m:
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	HASH_DEL(debug_memory, m);
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	kh_del(memory, debug_memory, k);
#endif
	hdebugf("%s deleted memory: %s, address: %p, size: %zd, func: %s, file: %s, line: %d", command, m->name, m->address, m->size, m->func, m->file, m->line);
	memory_current -= (m->size - (hmemory_signature_size * 2));
	free(m);
	hmemory_unlock();
	return 0;
}

static void * hmemory_worker (void *arg)
{
	int check;
	unsigned int v;
	struct timeval tval;
	struct timespec tspec;
	struct hmemory_memory *m;
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	struct hmemory_memory *nm;
#endif
	(void) arg;
	while (1) {
		check = 1;
		v = hmemory_getenv_int(HMEMORY_CHECK_INTERVAL_NAME);
		if (v == (unsigned int) -1) {
			check = 0;
			v = HMEMORY_CHECK_INTERVAL;
		}
		gettimeofday(&tval, NULL);
		tspec.tv_sec = tval.tv_sec + (v / 1000);
		tspec.tv_nsec = (tval.tv_usec + ((v % 1000) * 1000)) * 1000;
		if (tspec.tv_nsec >= 1000000000) {
			tspec.tv_sec += 1;
			tspec.tv_nsec -= 1000000000;
		}
		hmemory_lock();
		if (hmemory_worker_running == 1) {
			pthread_cond_timedwait(&hmemory_cond, &hmemory_mutex, &tspec);
		}
		if (hmemory_worker_running == 0) {
			hmemory_unlock();
			break;
		}
		if (check == 0) {
			hmemory_unlock();
			continue;
		}
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
		HASH_ITER(hh, debug_memory, m, nm) {
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
		kh_foreach_value(debug_memory, m,
#endif
			debug_memory_check_actual(m->address, "worker check", __FUNCTION__, __FILE__, __LINE__);
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
		}
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
		)
#endif
		hinfof("memory information:")
		hinfof("    current: %llu bytes (%.02f mb)", memory_current, ((double) memory_current) / (1024.00 * 1024.00));
		hinfof("    peak   : %llu bytes (%.02f mb)", memory_peak, ((double) memory_peak) / (1024.00 * 1024.00));
		hinfof("    total  : %llu bytes (%.02f mb)", memory_total, ((double) memory_total) / (1024.00 * 1024.00));
		hmemory_unlock();
	}
	return NULL;
}

static void __attribute__ ((constructor)) hmemory_init (void)
{
	int rc;
	hmemory_lock();
#if defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	debug_memory = kh_init(memory);
#endif
	hmemory_worker_started = 1;
	hmemory_worker_running = 1;
	rc = pthread_create(&hmemory_thread, NULL, hmemory_worker, NULL);
	if (rc != 0) {
		herrorf("failed to create worker");
		hmemory_worker_started = 0;
		hmemory_worker_running = 0;
	}
	hmemory_unlock();
}

static void __attribute__ ((destructor)) hmemory_fini (void)
{
	int show_reachable;
	struct hmemory_memory *m;
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	struct hmemory_memory *nm;
#endif
	hmemory_lock();
	if (hmemory_worker_running == 1) {
		hmemory_worker_running = 0;
	}
	pthread_cond_signal(&hmemory_cond);
	hmemory_unlock();
	pthread_join(hmemory_thread, NULL);
	hmemory_lock();
	hdebug_lock();
	hinfof("memory information:")
	hinfof("    current: %llu bytes (%.02f mb)", memory_current, ((double) memory_current) / (1024.00 * 1024.00));
	hinfof("    peak   : %llu bytes (%.02f mb)", memory_peak, ((double) memory_peak) / (1024.00 * 1024.00));
	hinfof("    total  : %llu bytes (%.02f mb)", memory_total, ((double) memory_total) / (1024.00 * 1024.00));
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	hinfof("    leaks  : %d items", HASH_COUNT(debug_memory));
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	hinfof("    leaks  : %d items", kh_size(debug_memory));
#endif
	show_reachable = hmemory_getenv_int(HMEMORY_SHOW_REACHABLE_NAME);
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
	if (HASH_COUNT(debug_memory) > 0 && show_reachable == 1) {
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	if (kh_size(debug_memory) > 0 && show_reachable == 1) {
#endif
		hinfof("  memory leaks:");
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
		HASH_ITER(hh, debug_memory, m, nm) {
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
		kh_foreach_value(debug_memory, m,
#endif
			hinfof("    - %zd bytes at: %p %s (%s:%u)", m->size, m->address + hmemory_signature_size, m->func, m->file, m->line);
#if defined(HMEMORY_HASH_UTHASH) && (HMEMORY_HASH_UTHASH == 1)
			HASH_DEL(debug_memory, m);
			free(m->address);
			free(m);
		}
#elif defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
			free(m->address);
			free(m);
		)
#endif
		hassert(0 && "memory leak");
	}
#if defined(HMEMORY_HASH_KHASH) && (HMEMORY_HASH_KHASH == 1)
	kh_destroy(memory, debug_memory);
#endif
	hdebug_unlock();
	hmemory_unlock();
}

#endif
