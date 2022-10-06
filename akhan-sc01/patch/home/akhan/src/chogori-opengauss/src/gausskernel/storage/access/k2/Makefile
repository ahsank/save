diff --git a/src/gausskernel/storage/access/k2/Makefile b/src/gausskernel/storage/access/k2/Makefile
index 172c00fb..dcdc265f 100644
--- a/src/gausskernel/storage/access/k2/Makefile
+++ b/src/gausskernel/storage/access/k2/Makefile
@@ -10,6 +10,7 @@ ifneq "$(MAKECMDGOALS)" "clean"
   endif
 endif
 
+override CXXFLAGS += -ggdb
 SUBDIRS     = catalog
 
 OBJS = k2pg-internal.o k2pg_util.o k2pg_aux.o pg_gate_thread_local.o pg_gate_api.o k2_expr.o k2_type.o k2catam.o k2cat_cmds.o \
