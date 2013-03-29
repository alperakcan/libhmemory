# hmemory #

hmemory is a memory error detector for c/c++ programs.

1. <a href="#1-overview">overview</a>
2. <a href="#2-configuration">configuration</a>
3. <a href="#3-error-reports">error reports</a>
4. <a href="#4-test-cases">test cases</a>
5. <a href="#5-usage-example">usage example</a>
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

hthread configuration parameters can be set using <tt>make flags</tt>, please check example section for demonstration.

- HTHREAD_ENABLE_CALLSTACK
  
  default 1
  
  enable/disable reporting call trace information on error, useful but depends on <tt>libbdf</tt>, <tt>libdl</tt>, and
  <tt>backtrace function from glibc</tt>. may be disabled for toolchains which does not support backtracing.
  
- HTHREAD_REPORT_CALLSTACK

  default 1
  
  dump callstack info (function call history) for error point.
  
- HTHREAD_ASSERT_ON_ERROR

  default 1
  
  terminate the process on any pthreads api misuse and/or lock order violation.

- HMEMORY_CORRUPTION_CHECK_INTERVAL

  default 5000
  
  memory corruption checking interval in miliseconds

### 2.2. run-time options ###
  
hthread reads configuration parameters from environment via getenv function call. one can either set/change environment
variables in source code of monitored project via setenv function call, or set them globally in running shell using
export function.

please check example section for demonstration.

- hthread_report_callstack
  
  default 1
  
  dump callstack info (function call history) for error point.

- hthread_assert_on_error
    
  default 1
  
  terminate the process on any pthreads api misuse and/or lock order violation.

- hmemory_corruption_check_interval

  default 5000
  
  memory corruption checking interval in miliseconds
  
## 3. error reports ##

## 4. test cases ##

## 5. usage example ##

## 6. contact ##

if you are using the software and/or have any questions, suggestions, etc. please contact with me at
alper.akcan@gmail.com

## 7. license ##

Copyright (C) 2008-2013 Alper Akcan <alper.akcan@gmail.com>

This work is free. It comes without any warranty, to the extent permitted
by applicable law. You can redistribute it and/or modify it under the terms
of the Do What The Fuck You Want To Public License, Version 2, as published
by Sam Hocevar. See the COPYING file for more details.
