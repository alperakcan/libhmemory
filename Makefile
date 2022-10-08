
HMEMORY_BUILD_TEST	?= y

prefix ?= /usr/local

subdir-y = \
    src

subdir-${HMEMORY_BUILD_TEST} += \
    test

test_depends-y = \
    src

include Makefile.lib

tests: test
	${Q}( \
	  export hmemory_corruption_check_interval=250; \
	  export hmemory_show_reachable=1; \
	  sc=0; \
	  ss=0; \
	  sf=0; \
	  for t in `ls -1 test/success-*-debug`; do \
	    echo "testing $$t ..."; \
	    $$t; \
	    if [ "$$?" != "0" ]; then \
	      sf=$$((sf + 1)); \
	    else \
	      ss=$$((ss + 1)); \
	    fi; \
	    sc=$$((sc + 1)); \
	  done; \
	  export hmemory_corruption_check_interval=250; \
	  fc=0; \
	  fs=0; \
	  ff=0; \
	  for t in `ls -1 test/fail-*-debug`; do \
	    echo "testing $$t ..."; \
	    $$t; \
	    if [ "$$?" = "0" ]; then \
	      ff=$$((ff + 1)); \
	    else \
	      fs=$$((fs + 1)); \
	    fi; \
	    fc=$$((fc +1)); \
	  done; \
	  echo "success tests total: $$sc, success: $$ss, fail: $$sf"; \
	  echo "fail tests    total: $$fc, success: $$fs, fail: $$ff"; \
	)

install: src test
	install -d ${DESTDIR}/${prefix}/include/hmemory
	install -m 0644 dist/include/hmemory.h ${DESTDIR}/${prefix}/include/hmemory/hmemory.h

	install -d ${DESTDIR}/${prefix}/lib
	install -m 0644 dist/lib/libhmemory.o ${DESTDIR}/${prefix}/lib/libhmemory.o
	install -m 0644 dist/lib/libhmemory.a ${DESTDIR}/${prefix}/lib/libhmemory.a
	install -m 0755 dist/lib/libhmemory.so ${DESTDIR}/${prefix}/lib/libhmemory.so
