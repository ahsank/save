diff --git a/test/integration/test_http_client.sh b/test/integration/test_http_client.sh
index c7da3f7..9a08f63 100755
--- a/test/integration/test_http_client.sh
+++ b/test/integration/test_http_client.sh
@@ -9,11 +9,11 @@ cd ${topname}/../..
 cpo_child_pid=$!
 
 # start nodepool
-./build/src/k2/cmd/nodepool/nodepool ${COMMON_ARGS} -c${#EPS[@]} --tcp_endpoints ${EPS[@]} --k23si_persistence_endpoint ${PERSISTENCE} --prometheus_port 63001 --memory=1G --partition_request_timeout=6s &
+./build/src/k2/cmd/nodepool/nodepool ${COMMON_ARGS} -c${#EPS[@]} --tcp_endpoints ${EPS[@]} --k23si_persistence_endpoint ${PERSISTENCE} --prometheus_port 63001 --memory=1G --partition_request_timeout=6s --k23si_autoflush_deadline 10ms --log_level=Debug k2::transport=Info &
 nodepool_child_pid=$!
 
 # start persistence
-./build/src/k2/cmd/persistence/persistence ${COMMON_ARGS} -c1 --tcp_endpoints ${PERSISTENCE} --prometheus_port 63002 &
+./build/src/k2/cmd/persistence/persistence ${COMMON_ARGS} -c1 --tcp_endpoints ${PERSISTENCE} --prometheus_port 63002 --log_level=Debug k2::transport=Info &
 persistence_child_pid=$!
 
 # start tso
@@ -23,7 +23,7 @@ tso_child_pid=$!
 sleep 1
 
 # start http proxy with increased cpo request timeout as delete collection takes more time
-./build/src/k2/cmd/httpproxy/http_proxy ${COMMON_ARGS} -c1 --tcp_endpoints ${HTTP} --memory=1G --cpo ${CPO} --httpproxy_txn_timeout=100ms --httpproxy_expiry_timer_interval=50ms --cpo_request_timeout=6s&
+./build/src/k2/cmd/httpproxy/http_proxy ${COMMON_ARGS} -c1 --tcp_endpoints ${HTTP} --memory=1G --cpo ${CPO} --httpproxy_txn_timeout=100ms --httpproxy_expiry_timer_interval=50ms --cpo_request_timeout=6s  --log_level=Info k2::httpproxy=Info &
 http_child_pid=$!
 
 function finish {
