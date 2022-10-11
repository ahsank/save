diff --git a/src/gausskernel/storage/access/k2/Makefile b/src/gausskernel/storage/access/k2/Makefile
index a7557222..fb70d69b 100644
--- a/src/gausskernel/storage/access/k2/Makefile
+++ b/src/gausskernel/storage/access/k2/Makefile
@@ -10,6 +10,9 @@ ifneq "$(MAKECMDGOALS)" "clean"
   endif
 endif
 
+exclude_option=-O2
+override CXXFLAGS := $(filter-out $(exclude_option),$(CXXFLAGS))
+override CXXFLAGS += -ggdb
 SUBDIRS     = catalog
 
 OBJS = k2pg-internal.o k2pg_util.o k2pg_aux.o pg_gate_thread_local.o pg_gate_api.o k2catam.o k2cat_cmds.o \
