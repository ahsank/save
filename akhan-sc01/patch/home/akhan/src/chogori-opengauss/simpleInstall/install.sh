diff --git a/simpleInstall/install.sh b/simpleInstall/install.sh
index 88bd2eac..f0244d8f 100644
--- a/simpleInstall/install.sh
+++ b/simpleInstall/install.sh
@@ -212,7 +212,7 @@ function set_environment() {
 
 function single_install() {
     info "[step 6]: init datanode"
-    gs_initdb -w $password -D $app/data/single_node --nodename "sgnode" --locale="en_US.UTF-8"
+    ${GDB} gs_initdb -w $password -d -D $app/data/single_node --nodename "sgnode" --locale="en_US.UTF-8"
     if [ X$port != X$default_port  ]
     then
         sed -i "/^#port =/c\port = $port" $app/data/single_node/postgresql.conf
