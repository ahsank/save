diff --git a/test/integration/common_defs.sh b/test/integration/common_defs.sh
index bb4067b..bb111fa 100644
--- a/test/integration/common_defs.sh
+++ b/test/integration/common_defs.sh
@@ -1,6 +1,5 @@
 set -e
 CPODIR=/tmp/___cpo_integ_test
-rm -rf ${CPODIR}
 EPS=("tcp+k2rpc://0.0.0.0:10000" "tcp+k2rpc://0.0.0.0:10001" "tcp+k2rpc://0.0.0.0:10002" "tcp+k2rpc://0.0.0.0:10003" "tcp+k2rpc://0.0.0.0:10004")
 NUMCORES=`nproc`
 # core on which to run the TSO poller thread. Pick 4 if we have that many, or the highest-available otherwise
