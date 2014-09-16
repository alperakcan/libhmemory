
target.o-y = \
	libhmemory.o \
	libhmemory-actual.o \
	libhmemory-debug.o

target.so-y = \
	libhmemory.so

libhmemory.o_files-y = \
	libhmemory-actual.o \
	libhmemory-debug.o

libhmemory.so_files-y = \
	libhmemory.o

libhmemory-actual.o_files-y = \
	hmemory.c

libhmemory-actual.o_cflags-y = \
	-DHMEMORY_DEBUG=0
	
libhmemory-debug.o_files-y = \
	hmemory.c

libhmemory-debug.o_cflags-y = \
	-DHMEMORY_DEBUG=1

libhmemory.so_ldflags-y += \
	-lpthread

ifeq (${HMEMORY_ENABLE_CALLSTACK}, y)
libhmemory-actual.o_cflags-y += \
	-DHMEMORY_ENABLE_CALLSTACK=${HMEMORY_ENABLE_CALLSTACK}

libhmemory-debug.o_cflags-y += \
	-DHMEMORY_ENABLE_CALLSTACK=${HMEMORY_ENABLE_CALLSTACK}

libhmemory.so_ldflags-y += \
	-ldl \
	-lbfd
endif

ifeq (${HMEMORY_REPORT_CALLSTACK}, y)
libhmemory-actual.o_cflags-y += \
	-DHMEMORY_REPORT_CALLSTACK=${HMEMORY_REPORT_CALLSTACK}

libhmemory-debug.o_cflags-y += \
	-DHMEMORY_REPORT_CALLSTACK=${HMEMORY_REPORT_CALLSTACK}
endif

ifneq (${HMEMORY_ASSERT_ON_ERROR}, )
libhmemory-actual.o_cflags-y += \
	-DHMEMORY_ASSERT_ON_ERROR=${HMEMORY_ASSERT_ON_ERROR}

libhmemory-debug.o_cflags-y += \
	-DHMEMORY_ASSERT_ON_ERROR=${HMEMORY_ASSERT_ON_ERROR}
endif

distdir = ../dist

dist.lib-y = \
	libhmemory.so

dist.include-y = \
	hmemory.h

include ../Makefile.lib
