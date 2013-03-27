
ifeq ($(uname_S), Linux)
HMEMORY_ENABLE_CALLSTACK ?= y
else
HMEMORY_ENABLE_CALLSTACK := n
endif

tests-success-y = \
	$(subst .c, , $(wildcard success-*.c))

tests-fail-y = \
	$(subst .c, , $(filter-out fail-env.c, $(wildcard fail-*.c)))

target-y = \
	${tests-success-y} \
	$(addsuffix -debug, ${tests-success-y}) \
	${tests-fail-y} \
	$(addsuffix -debug, ${tests-fail-y})

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

define test-defaults
	$1_files-y = \
		$(addsuffix .c, $1)

	$1_includes-y = \
		../src
endif
endef

define test-debug-defaults
	$1_files-y = \
		$(addsuffix .c, $(subst -debug, , $1)) \
		../src/libhmemory.o

	$1_cflags-y = \
		-DHMEMORY_DEBUG=1 \
		-include ../src/hmemory.h
	
	$1_includes-y = \
		../src
	
	$1_ldflags-${HMEMORY_ENABLE_CALLSTACK} += \
		-rdynamic \
		-ldl \
		-lbfd
endif
endef

$(eval $(foreach T,$(target-y), $(eval $(call test-defaults,$T))))
$(eval $(foreach T,$(target-y), $(eval $(call test-debug-defaults,$(addsuffix -debug, $T)))))

include ../Makefile.lib
