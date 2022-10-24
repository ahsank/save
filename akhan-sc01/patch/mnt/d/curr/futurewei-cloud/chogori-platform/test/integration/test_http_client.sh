diff --git a/test/integration/test_http_client.sh b/test/integration/test_http_client.sh
index 3e350e5..a8135d9 100755
--- a/test/integration/test_http_client.sh
+++ b/test/integration/test_http_client.sh
@@ -22,7 +22,7 @@ tso_child_pid=$!
 
 sleep 1
 
-./build/src/k2/cmd/httpproxy/http_proxy ${COMMON_ARGS} -c1 --tcp_endpoints ${HTTP} --memory=1G --cpo ${CPO} --httpproxy_txn_timeout=100ms --httpproxy_expiry_timer_interval=50ms&
+./build/src/k2/cmd/httpproxy/http_proxy ${COMMON_ARGS} -c1 --tcp_endpoints ${HTTP} --memory=1G --cpo ${CPO} --log_level=Info k2::httpproxy=Debug --httpproxy_txn_timeout=100s --httpproxy_expiry_timer_interval=50s&
 http_child_pid=$!
 
 function finish {
@@ -53,7 +53,7 @@ function finish {
   echo ">>>> Test ${0} finished with code ${rv}"
 }
 trap finish EXIT
-sleep 1
+sleep 20m
 
 echo ">>> Starting http test ..."
 PYTHONPATH=${PYTHONPATH}:./test/integration ./test/integration/test_http.py --http http://127.0.0.1:30000
