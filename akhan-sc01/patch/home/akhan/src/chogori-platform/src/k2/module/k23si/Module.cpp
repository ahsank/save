diff --git a/src/k2/module/k23si/Module.cpp b/src/k2/module/k23si/Module.cpp
index 97007c8..322f6d9 100644
--- a/src/k2/module/k23si/Module.cpp
+++ b/src/k2/module/k23si/Module.cpp
@@ -590,6 +590,7 @@ K23SIPartitionModule::handleQuery(dto::K23SIQueryRequest&& request, dto::K23SIQu
 
         // happy case: either committed, or txn is reading its own write
         if (!conflict) {
+            count = 0;
             if (!record->isTombstone) {
                 auto [status, keep] = _doQueryFilter(request, record->value);
                 if (!status.is2xxOK()) {
@@ -644,6 +645,7 @@ K23SIPartitionModule::handleQuery(dto::K23SIQueryRequest&& request, dto::K23SIQu
                         resp=std::move(response), deadline, count](auto&& retryChallenger) mutable {
             if (!retryChallenger.is2xxOK()) {
                 // sitting transaction won. Abort the incoming request
+                K2LOG_E(log::skvsvr, "Txn push failed {}", retryChallenger);
                 return RPCResponse(dto::K23SIStatus::AbortConflict("incumbent txn won in query push"), dto::K23SIQueryResponse{});
             }
             return handleQuery(std::move(request), std::move(resp), deadline, count);
@@ -1231,11 +1233,13 @@ K23SIPartitionModule::_doPush(dto::Key key, dto::Timestamp incumbentId, dto::K23
     K2LOG_D(log::skvsvr, "executing push against txn={}, for mtr={}", *incumbent, challengerMTR);
 
     dto::K23SITxnPushRequest request{};
+    auto its = incumbent->mtr.timestamp;
+    auto cts = challengerMTR.timestamp;
     request.collectionName = incumbent->trhCollection;
     request.incumbentMTR = incumbent->mtr;
     request.key = incumbent->trh; // this is the routing key - should be the TRH key
     request.challengerMTR = std::move(challengerMTR);
-    return seastar::do_with(std::move(request), std::move(key), [this, deadline, &incumbent] (auto& request, auto& key) {
+    return seastar::do_with(std::move(request), std::move(key), [this, deadline, &incumbent, its, cts] (auto& request, auto& key) {
         auto fut = seastar::make_ready_future<std::tuple<Status, dto::K23SITxnPushResponse>>();
         if (incumbent->isAborted()) {
             fut = fut.then([] (auto&&) {
@@ -1246,7 +1250,9 @@ K23SIPartitionModule::_doPush(dto::Key key, dto::Timestamp incumbentId, dto::K23
         }
         else if (incumbent->isCommitted()) {
             // Challenger should retry if they are newer than the committed value
-            fut = fut.then([] (auto&&) {
+            fut = fut.then([its, cts] (auto&&) {
+                    K2LOG_I(log::skvsvr, "incumbent txn={} won, for challenger mtr={}", its, cts);
+
                 return RPCResponse(dto::K23SIStatus::OK("incumbent won in push since incumbent was already committed"),
                               dto::K23SITxnPushResponse{.incumbentFinalization = dto::EndAction::Commit,
                                                         .allowChallengerRetry = true});
@@ -1258,10 +1264,11 @@ K23SIPartitionModule::_doPush(dto::Key key, dto::Timestamp incumbentId, dto::K23
                 return _cpo.partitionRequest<dto::K23SITxnPushRequest, dto::K23SITxnPushResponse, dto::Verbs::K23SI_TXN_PUSH>(deadline, request);
             });
         }
-        return fut.then([this, &key, &request](auto&& responsePair) {
+        return fut.then([this, &key, &request, its, cts](auto&& responsePair) {
             auto& [status, response] = responsePair;
             K2LOG_D(log::skvsvr, "Push request completed with status={} and response={}", status, response);
             if (!status.is2xxOK()) {
+                K2LOG_I(log::skvsvr, "Push conflict incumbent txn={}, for challenger mtr={}", its, cts);
                 K2LOG_E(log::skvsvr, "txn push failed: {}", status);
                 return seastar::make_ready_future<Status>(std::move(status));
             }
