/*
 *  Copyright (c) 2009-2013 Alper Akcan <alper.akcan@gmail.com>
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

#include "hmemory.h"
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

static pthread_mutex_t hmemory_mutex	= PTHREAD_MUTEX_INITIALIZER;

static inline int debug_dump_callstack (const char *prefix);
static inline int debug_memory_add (const char *name, void *address, size_t size, const char *command, const char *func, const char *file, const int line);
static inline int debug_memory_del (void *address, const char *command, const char *func, const char *file, const int line);

#else

#define debug_thread_unused() \
	(void) func; \
	(void) file; \
	(void) line;
#define debug_memory_add(a...)		(void) name; debug_thread_unused()
#define debug_memory_del(a...)		debug_thread_unused()

#endif

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

void * HMEMORY_FUNCTION_NAME(memset_actual) (void *destination, int c, size_t len, const char *func, const char *file, const int line)
{
	void *rc;
	(void) func;
	(void) file;
	(void) line;
	rc = memset(destination, c, len);
	return rc;
}

void * HMEMORY_FUNCTION_NAME(memcpy_actual) (void *destination, void *source, size_t len, const char *func, const char *file, const int line)
{
	void *rc;
	(void) func;
	(void) file;
	(void) line;
	rc = memcpy(destination, source, len);
	return rc;
}

char * HMEMORY_FUNCTION_NAME(strdup_actual) (const char *name, const char *string, const char *func, const char *file, const int line)
{
	void *rc;
	if (string == NULL) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hmemory_lock();
		hdebug_lock();
		hinfof("strdup with invalid argument '%p'", string);
		hinfof("    at: %s %s:%d", func, file, line);
		hdebug_unlock();
		hassert((string != NULL) && "invalid strdup parameter");
		hmemory_unlock();
#endif
		return NULL;
	}
	rc = strdup(string);
	if (rc == NULL) {
		herrorf("strdup failed");
	}
	debug_memory_add(name, rc, strlen(rc) + 1, "strdup", func, file, line);
	return rc;
}

char * HMEMORY_FUNCTION_NAME(strndup_actual) (const char *name, const char *string, size_t size, const char *func, const char *file, const int line)
{
	void *rc;
	if (string == NULL) {
#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)
		hmemory_lock();
		hdebug_lock();
		hinfof("strdup with invalid argument '%p'", string);
		hinfof("    at: %s %s:%d", func, file, line);
		hdebug_unlock();
		hassert((string != NULL) && "invalid strdup parameter");
		hmemory_unlock();
#endif
		return NULL;
	}
	rc = strndup(string, size);
	if (rc == NULL) {
		herrorf("strndup failed");
	}
	debug_memory_add(name, rc, strlen(rc) + 1, "strndup", func, file, line);
	return rc;
}

void * HMEMORY_FUNCTION_NAME(malloc_actual) (const char *name, size_t size, const char *func, const char *file, const int line)
{
	void *rc;
	rc = malloc(size);
	if (rc == NULL) {
		herrorf("malloc failed");
	}
	debug_memory_add(name, rc, size, "malloc", func, file, line);
	return rc;
}

void * HMEMORY_FUNCTION_NAME(calloc_actual) (const char *name, size_t nmemb, size_t size, const char *func, const char *file, const int line)
{
	void *rc;
	rc = calloc(nmemb, size);
	if (rc == NULL) {
		herrorf("calloc failed");
	}
	debug_memory_add(name, rc, nmemb * size, "calloc", func, file, line);
	return rc;
}

void * HMEMORY_FUNCTION_NAME(realloc_actual) (const char *name, void *address, size_t size, const char *func, const char *file, const int line)
{
	void *rc;
	rc = realloc(address, size);
	if (rc == NULL) {
		herrorf("realloc failed");
	}
	debug_memory_del(address, "realloc", func, file, line);
	debug_memory_add(name, rc, size, "realloc", func, file, line);
	return rc;
}

void HMEMORY_FUNCTION_NAME(free_actual) (void *address, const char *func, const char *file, const int line)
{
	debug_memory_del(address, "free", func, file, line);
	free(address);
}

#if defined(HMEMORY_DEBUG) && (HMEMORY_DEBUG == 1)

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

struct hmemory_memory {
	void *address;
	UT_hash_handle hh;
	const char *func;
	const char *file;
	int line;
	size_t size;
	char name[0];
};

static struct hmemory_memory *debug_memory = NULL;

static int debug_memory_add (const char *name, void *address, size_t size, const char *command, const char *func, const char *file, const int line)
{
	unsigned int s;
	struct hmemory_memory *m;
	if (address == NULL) {
		return 0;
	}
	hmemory_lock();
	HASH_FIND_PTR(debug_memory, &address, m);
	if (m != NULL) {
		hdebug_lock();
		hinfof("%s with invalid memory (%p)", command, address);
		hinfof("    at: %s (%s:%d)", func, file, line);
		hinfof("  ");
		hinfof("  it is essential for correct operation of hmemory that there");
		hinfof("  are no memory errors such as dangling pointers in process.");
		hinfof("  ");
		hinfof("  which means that it is a good idea to make sure that program");
		hinfof("  is clean before analyzing with hmemory. it is possible however");
		hinfof("  that some memory errors are caused by data races.")
		hinfof("  ");
		hinfof("  if it is certain that program is memory bug free, then hmemory");
		hinfof("  may have a serious bug that needs to be fixed urgent. please close");
		hinfof("  race condition checking for now '-DHMEMORY_ENABLE_RACE_CHECK=0',");
		hinfof("  and inform author");
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
	HASH_ADD_PTR(debug_memory, address, m);
	hdebugf("%s added memory: %s, address: %p, size: %zd, func: %s, file: %s, line: %d", command, m->name, m->address, m->size, m->func, m->file, m->line);
	hmemory_unlock();
	return 0;
}

static int debug_memory_del (void *address, const char *command, const char *func, const char *file, const int line)
{
	struct hmemory_memory *m;
	if (address == NULL) {
		return 0;
	}
	hmemory_lock();
	HASH_FIND_PTR(debug_memory, &address, m);
	if (m != NULL) {
		goto found_m;
	}
	hdebug_lock();
	hinfof("%s with invalid memory (%p)", command, address);
	hinfof("    at: %s (%s:%d)", func, file, line);
	hinfof("  ");
	hinfof("  it is essential for correct operation of hmemory that there");
	hinfof("  are no memory errors such as dangling pointers in process.");
	hinfof("  ");
	hinfof("  which means that it is a good idea to make sure that program");
	hinfof("  is clean before analyzing with hmemory. it is possible however");
	hinfof("  that some memory errors are caused by data races.")
	hinfof("  ");
	hinfof("  if it is certain that program is memory bug free, then hmemory");
	hinfof("  may have a serious bug that needs to be fixed urgent. please close");
	hinfof("  race condition checking for now '-DHMEMORY_ENABLE_RACE_CHECK=0',");
	hinfof("  and inform author");
	hinfof("    at: alper.akcan@gmail.com");
	hdebug_unlock();
	hassert((m != NULL) && "invalid memory");
	hmemory_unlock();
	return -1;
found_m:
	HASH_DEL(debug_memory, m);
	hdebugf("%s deleted memory: %s, address: %p, size: %zd, func: %s, file: %s, line: %d", command, m->name, m->address, m->size, m->func, m->file, m->line);
	free(m);
	hmemory_unlock();
	return 0;
}

#endif
