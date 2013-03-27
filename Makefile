
subdir-y = \
    src \
    test

test_depends-y = \
    src

include Makefile.lib

tests: test
	${Q}( \
	  export hthread_lock_threshold=1000; \
	  export hthread_lock_threshold_assert=0; \
	  export hthread_lock_try_threshold=1000; \
	  export hthread_lock_try_threshold_assert=0; \
	  let sc=0; \
	  let ss=0; \
	  let sf=0; \
	  for t in `ls -1 test/success-*-debug`; do \
	    echo "testing $$t ..."; \
	    $$t; \
	    if [ "$$?" != "0" ]; then \
	      let sf=$$sf+1; \
	    else \
	      let ss=$$ss+1; \
	    fi; \
	    let sc=$$sc+1; \
	  done; \
	  export hthread_lock_threshold=1000; \
	  export hthread_lock_threshold_assert=1; \
	  export hthread_lock_try_threshold=1000; \
	  export hthread_lock_try_threshold_assert=1; \
	  let fc=0; \
	  let fs=0; \
	  let ff=0; \
	  for t in `ls -1 test/fail-*-debug`; do \
	    echo "testing $$t ..."; \
	    $$t; \
	    if [ "$$?" == "0" ]; then \
	      let ff=$$ff+1; \
	    else \
	      let fs=$$fs+1; \
	    fi; \
	    let fc=$$fc+1; \
	  done; \
	  echo "success tests total: $$sc, success: $$ss, fail: $$sf"; \
	  echo "fail tests    total: $$fc, success: $$fs, fail: $$ff"; \
	)
