diff --git a/src/gausskernel/storage/access/k2/catalog/Makefile b/src/gausskernel/storage/access/k2/catalog/Makefile
index c858563b..bcd01304 100644
--- a/src/gausskernel/storage/access/k2/catalog/Makefile
+++ b/src/gausskernel/storage/access/k2/catalog/Makefile
@@ -11,7 +11,7 @@ include $(top_srcdir)/src/gausskernel/common.mk
 
 
 $(OBJ_DIR)/%.o: %.cpp
-	$(COMPILE.cpp) -MMD -MP -MF"$(patsubst %.o,%.d,$@)" -MT"$@" -o $@ $<
+	$(COMPILE.cpp) -ggdb -MMD -MP -MF"$(patsubst %.o,%.d,$@)" -MT"$@" -o $@ $<
 
 $(OBJS): | buildrepo
 
