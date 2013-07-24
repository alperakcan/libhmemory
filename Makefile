
subdir-y = \
    src \
    test

test_depends-y = \
    src

include Makefile.lib

tests: test
	${Q}( \
	  export hmemory_corruption_check_interval=250; \
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
