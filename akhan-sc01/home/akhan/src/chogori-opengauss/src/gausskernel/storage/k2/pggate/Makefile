subdir = src/gausskernel/storage/k2/pggate
top_builddir = ../../../../..
include $(top_builddir)/src/Makefile.global

SUBDIRS     = catalog

OBJ_DIR = ./obj

OBJS = $(OBJ_DIR)/pg_txn_handle.o

DEPS = $(OBJ_DIR)/pg_txn_handle.d

include $(top_srcdir)/src/gausskernel/common.mk

$(OBJ_DIR)/%.o: %.cc
	$(COMPILE.cpp) -MMD -MP -MF"$(patsubst %.o,%.d,$@)" -MT"$@" -o $@ $<

$(OBJS): | buildrepo

clean:
	rm -rf $(OBJ_DIR)

buildrepo:
	@$(call make-repo)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

define make-repo
   mkdir -p $(OBJ_DIR);
endef
