# hmemory #

hmemory is a memory error detector for c/c++ programs.

1. <a href="#1-overview">overview</a>
2. <a href="#2-configuration">configuration</a>
  - <a href="#21-compile-time-options">compile-time options</a>
  - <a href="#22-run-time-options">run-time options</a>
3. <a href="#3-error-reports">error reports</a>
4. <a href="#4-test-cases">test cases</a>
5. <a href="#5-usage-example">usage example</a>
  - <a href="#51-build-hmemory">build hmemory</a>
  - <a href="#52-memory-leak">memory leak</a>
  - <a href="#53-memory-corruption">memory corruption</a>
6. <a href="#6-contact">contact</a>
7. <a href="#7-license">license</a>

## 1. overview ##

hmemory is a lightweight memory error detector for c/c++ programs, specifically designed for embedded systems.

main use case may include embedded systems where <a href="http://valgrind.org">valgrind</a> - 
<a href="http://valgrind.org/docs/manual/ac-manual.html">addrcheck</a>, or
<a href="http://valgrind.org/docs/manual/mc-manual.html">memcheck</a> support **is not** available.

and has benefits of:

* has a negligible effect run-time speed
* does not require any source code change
* operating system and architecture independent
* easy to use

can detect errors of:

- double/invalid free
- mismatched use of malloc versus free
- writing before or end of malloc'd blocks
- invalid realloc
- overlapping src and dst pointers in memcpy
- memory leaks

## 2. configuration ##

1. <a href="#21-compile-time-options">compile-time options</a>
2. <a href="#22-run-time-options">run-time options</a>

### 2.1. compile-time options ###

hmemory configuration parameters can be set using <tt>make flags</tt>, please check example section for demonstration.

- HMEMORY_ENABLE_CALLSTACK
  
  default 0
  
  enable/disable reporting call trace information on error, useful but depends on <tt>libbdf</tt>, <tt>libdl</tt>, and
  <tt>backtrace function from glibc</tt>. may be disabled for toolchains which does not support backtracing.
  
- HMEMORY_REPORT_CALLSTACK

  default 0
  
  dump callstack info (function call history) for error point.
  
- HMEMORY_ASSERT_ON_ERROR

  default 1
  
  terminate the process on any pthreads api misuse and/or lock order violation.

- HMEMORY_CORRUPTION_CHECK_INTERVAL

  default 5000
  
  memory corruption checking interval in miliseconds, use -1 to disable.

- HMEMORY_SHOW_REACHABLE

  default 0
  
  show reachable memory on exit

### 2.2. run-time options ###
  
hmemory reads configuration parameters from environment via getenv function call. one can either set/change environment
variables in source code of monitored project via setenv function call, or set them globally in running shell using
export function.

please check example section for demonstration.

- hmemory_report_callstack
  
  default 0
  
  dump callstack info (function call history) for error point.

- hmemory_assert_on_error
    
  default 1
  
  terminate the process on any pthreads api misuse and/or lock order violation.

- hmemory_corruption_check_interval

  default 5000
  
  memory corruption checking interval in miliseconds, use -1 to disable.
  
- hmemory_show_reachable

  default 0
  
  show reachable memory on exit
  
## 3. error reports ##

## 4. test cases ##

## 5. usage example ##

  1. <a href="#51-build-hmemory">build hmemory</a>
  2. <a href="#52-memory-leak">memory leak</a>
  3. <a href="#53-memory-corruption">memory corruption</a>

using hmemory is pretty simple, just clone libhmemory and build;

- add <tt>-include hmemory.h -DHMEMORY_DEBUG=1 -g -O1</tt> to target cflags
- link with <tt>-lhmemory -lpthread -lrt</tt> if HMEMORY_ENABLE_CALLSTACK is 0 or
- link with <tt>-lhmemory -lpthread -lrt -ldl -lbfd</tt> if HMEMORY_ENABLE_CALLSTACK is 1

### 5.1. build hmemory ###

compile libhmemory with callstack support

    # git clone git://github.com/anhanguera/libhmemory.git
    # cd libhmemory
    # HMEMORY_ENABLE_CALLSTACK=1 make
  
compile libhmemory without callstack support

    # git clone git://github.com/anhanguera/libhmemory.git
    # cd libhmemory
    # HMEMORY_ENABLE_CALLSTACK=0 make

### 5.2. memory leak ###

let below is the source code - with memory leak - to be monitored:

     1 #include <stdio.h>
     2 #include <stdlib.h>
     3 #include <string.h>
     4
     5 int main (int argc, char *argv[])
     6 {
     7     void *rc;
     8     (void) argc;
     9     (void) argv;
    10     rc = malloc(1024);
    11     if (rc == NULL) {
    12         fprintf(stderr, "malloc failed\n");
    13         exit(-1);
    14     }
    15     rc = malloc(1024);
    16     if (rc == NULL) {
    17         fprintf(stderr, "malloc failed\n");
    18         exit(-1);
    19     }
    20     free(rc);
    21     return 0;
    22 }

compile and run as usual:
  
    # gcc -o app main.c
    # ./app

application will exit normal, nothing unusual excpet that we have a leaking memory which was allocated at line 10. now,
enable monitoring with hmemory:

    # gcc -include src/hmemory.h -DHMEMORY_DEBUG=1 -g -O1 -o app-debug main.c -Lsrc -lhmemory -lpthread
    # LD_LIBRARY_PATH=src ./app-debug
    $ ./app-debug
    (hmemory:19437) memory information:
    (hmemory:19437)     current: 1032 bytes (0.00 mb)
    (hmemory:19437)     peak   : 2064 bytes (0.00 mb)
    (hmemory:19437)     total  : 2064 bytes (0.00 mb)
    (hmemory:19437)     leaks  : 1 items
    (hmemory:19437)   memory leaks:
    (hmemory:19437)     - 1032 bytes at: main (main.c:10)
    Assertion failed: (0 && "memory leak"), function hmemory_fini, file hmemory.c, line 775.

hmemory will report memory information and leaking points on exit.

### 5.3. memory corruption ###

let below is the source code - with memory corruption/overflow - to be monitored:

     1 #include <stdio.h>
     2 #include <stdlib.h>
     3 #include <string.h>
     4 #include <unistd.h>
     5 
     6 int main (int argc, char *argv[])
     7 {
     8     void *rc;
     9     (void) argc;
    10     (void) argv;
    11     rc = malloc(1024);
    12     if (rc == NULL) {
    13         fprintf(stderr, "malloc failed\n");
    14         exit(-1);
    15     }
    16     memset(rc, 0, 1025);
    17     sleep(1);
    18     free(rc);
    19     return 0;
    20 }

enable monitoring with hmemory:

    # gcc -include src/hmemory.h -DHMEMORY_DEBUG=1 -g -O1 -o app-debug main.c -lhmemory -lpthread
    # ./app-debug
    (hmemory:19513) free with corrupted address (0x7fb5ab000000), overflow
    (hmemory:19513)     at: main (main.c:18)
    Assertion failed: (((rcu == 0) && (rco == 0)) && "memory corruption"), function debug_memory_check_actual, file hmemory.c, line 634.

free call after memset catchs the error. hmemory has check points in every intercepted call. there is also a corruption
checker which works with defined interval, below is the example of how to tune:

    # gcc -include src/hmemory.h -DHMEMORY_DEBUG=1 -g -O1 -o app-debug main.c -lhmemory -lpthread
    # hmemory_corruption_check_interval=250 ./app-debug
    (hmemory:19530) worker check with corrupted address (0x7f80fb800000), overflow
    (hmemory:19530)     at: hmemory_worker (hmemory.c:727)
    Assertion failed: (((rcu == 0) && (rco == 0)) && "memory corruption"), function debug_memory_check_actual, file hmemory.c, line 634.

since the interval is 250 miliseconds, corruption checker could catch before hmemory check-points.

## 6. contact ##

if you are using the software and/or have any questions, suggestions, etc. please contact with me at
alper.akcan@gmail.com

## 7. license ##

Copyright (C) 2008-2013 Alper Akcan <alper.akcan@gmail.com>

This work is free. It comes without any warranty, to the extent permitted
by applicable law. You can redistribute it and/or modify it under the terms
of the Do What The Fuck You Want To Public License, Version 2, as published
by Sam Hocevar. See the COPYING file for more details.
