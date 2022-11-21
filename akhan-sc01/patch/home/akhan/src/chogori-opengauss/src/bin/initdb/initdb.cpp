diff --git a/src/bin/initdb/initdb.cpp b/src/bin/initdb/initdb.cpp
index a94f53fa..bda85a2b 100644
--- a/src/bin/initdb/initdb.cpp
+++ b/src/bin/initdb/initdb.cpp
@@ -4544,17 +4544,17 @@ int main(int argc, char* argv[])
     (void)fflush(stdout);
     CreatePGDefaultTempDir();
 
-    exit(0);
     /* Create the stuff we don't need to use bootstrap mode for */
 
     printf(_("Setup auth ... \n"));
     (void)fflush(stdout);
-    setup_auth();
+    //setup_auth();
     get_set_pwd();
 
     printf(_("Setup depend,load plpgsql, and system views ... \n"));
     (void)fflush(stdout);
     setup_depend();
+    exit(0);
     load_plpgsql();
     setup_sysviews();
 #ifdef ENABLE_PRIVATEGAUSS
