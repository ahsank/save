diff --git a/src/k2/httpproxy/HTTPProxy.cpp b/src/k2/httpproxy/HTTPProxy.cpp
index 061158a..891cba7 100644
--- a/src/k2/httpproxy/HTTPProxy.cpp
+++ b/src/k2/httpproxy/HTTPProxy.cpp
@@ -37,6 +37,52 @@ namespace k2 {
 static const inline auto Txn_S410_Gone = sh::Statuses::S410_Gone("transaction does not exist");
 static const inline auto Query_S410_Gone = sh::Statuses::S410_Gone("Query does not exist");
 
+// Doesn't work
+template <typename T>
+void _serialiationVisitor(std::optional<T>&& value, String& field, std::ostream& os) {
+    if (value) {
+          if constexpr (std::is_same_v<T, dto::FieldType>) {
+            auto ft = static_cast<shd::FieldType>(to_integral(*value));
+            os << field << '=' << (int) ft;             
+          } else {
+              os << field << '=' << *value;
+          }
+          os << ", ";
+    }
+}
+
+// Doesn't work
+std::string _serialize(dto::SKVRecord& k2rec) {
+    std::ostringstream os;
+    FOR_EACH_RECORD_FIELD(k2rec, _serialiationVisitor, os);
+    k2rec.seekField(0);
+    return os.str();
+}
+
+std::string _serialize(shd::SKVRecord& shdrec) {
+    std::ostringstream os;
+    uint32_t serializedCursor = shdrec.storage.serializedCursor;
+    for (uint32_t field = 0; field < serializedCursor; ++field) {
+        shdrec.visitNextField([&os](const auto& fld, auto&& value) {
+            if (value) {
+                using T = typename std::remove_reference_t<decltype(value)>::value_type;
+                if constexpr (std::is_same_v<T, shd::FieldType>) {
+                    auto kval = static_cast<dto::FieldType>(to_integral(*value));
+                    os << fld.name << '=' << kval;
+                } else if constexpr (std::is_same_v<T, sh::String>) {
+                    os << fld.name << '=' << *value;
+                } else {
+                    os << fld.name << '=' << *value;
+                }
+                os << ", ";
+            }
+        });
+    }
+
+    shdrec.seekField(0);
+    return os.str();
+}
+
 void _shdRecToK2(shd::SKVRecord& shdrec, dto::SKVRecord& k2rec) {
     uint32_t serializedCursor = shdrec.storage.serializedCursor;
     for (uint32_t field = 0; field < serializedCursor; ++field) {
@@ -226,9 +272,15 @@ HTTPProxy::_handleWrite(shd::WriteRequest&& request) {
                 return MakeHTTPResponse<shd::WriteResponse>(sh::Statuses::S400_Bad_Request("All fields must be specified in the record"), shd::WriteResponse{});
             }
             try {
+                K2LOG_D(log::httpproxy, "Write detail ts={} start=[{}] delete={}", it->second.handle.mtr().timestamp, _serialize(shdrecord), request.isDelete);
+
                 _shdRecToK2(shdrecord, k2record);
             } catch(shd::DeserializationError& err) {
                 return MakeHTTPResponse<shd::WriteResponse>(sh::Statuses::S400_Bad_Request(err.what()), shd::WriteResponse{});
+            } catch(std::exception& err) {
+                K2LOG_E(log::httpproxy, "Exception {}", err);
+                return MakeHTTPResponse<shd::WriteResponse>(sh::Statuses::S400_Bad_Request(err.what()), shd::WriteResponse{});
+                
             }
 
             return isPartialUpdate ?
@@ -303,8 +355,7 @@ HTTPProxy::_handleQuery(shd::QueryRequest&& request) {
     payload.seek(0);
     payload.read(paginationKey);
 
-    queryIter->second.resetPaginationToken(std::move(paginationKey), request.paginationExclusiveKey);
-
+    queryIter->second.resetPaginationToken(std::move(paginationKey), request.paginationExclusiveKey);    
     return iter->second.handle.query(queryIter->second)
     .then([this, request=std::move(request)](QueryResult&& result) {
         if(!result.status.is2xxOK()) {
@@ -391,10 +442,11 @@ HTTPProxy::_handleGetSchema(shd::GetSchemaRequest&& request) {
     });
 }
 
-void HTTPProxy::_shdStorageToK2Record(const sh::String& collectionName, shd::SKVRecord::Storage&& key, dto::SKVRecord& k2record) {
+shd::SKVRecord HTTPProxy::_shdStorageToK2Record(const sh::String& collectionName, shd::SKVRecord::Storage&& key, dto::SKVRecord& k2record) {
     auto shdSchema = _getSchemaFromCache(collectionName, k2record.schema);
     shd::SKVRecord shdrecord(collectionName, shdSchema, std::move(key), true);
     _shdRecToK2(shdrecord, k2record);
+    return shdrecord;
 }
 
 namespace k2exp = dto::expression;
@@ -431,20 +483,28 @@ k2exp::Expression getFilterExpression(shdexp::Expression&& shExpr) {
 seastar::future<std::tuple<sh::Status, shd::CreateQueryResponse>>
 HTTPProxy::_handleCreateQuery(shd::CreateQueryRequest&& request) {
     K2LOG_D(log::httpproxy, "Received create query request {}", request);
+    
     auto it = _txns.find(request.timestamp);
     if (it == _txns.end()) {
         return MakeHTTPResponse<shd::CreateQueryResponse>(Txn_S410_Gone, shd::CreateQueryResponse{});
     }
+    auto ts = it->second.handle.mtr().timestamp;
     updateExpiry(it->second);
     return _client.createQuery(request.collectionName, request.schemaName)
-        .then([this, req=std::move(request)] (auto&& result) mutable {
+        .then([this, req=std::move(request), ts] (auto&& result) mutable {
             if(!result.status.is2xxOK()) {
                 return MakeHTTPResponse<shd::CreateQueryResponse>(sh::Status{.code = result.status.code, .message = result.status.message}, shd::CreateQueryResponse{});
             }
             try {
-                _shdStorageToK2Record(req.collectionName, std::move(req.key), result.query.startScanRecord);
-                _shdStorageToK2Record(req.collectionName, std::move(req.endKey), result.query.endScanRecord);
-
+                auto startKey = _shdStorageToK2Record(req.collectionName, std::move(req.key), result.query.startScanRecord);
+                auto endKey = _shdStorageToK2Record(req.collectionName, std::move(req.endKey), result.query.endScanRecord);
+                try {
+                    K2LOG_D(log::httpproxy, "Query detail ts={} start=[{}], end=[{}]", ts, _serialize(startKey), _serialize(endKey));
+                } catch(std::exception& err) {
+                    K2LOG_E(log::httpproxy, "Exception {}", err);
+                    return MakeHTTPResponse<shd::CreateQueryResponse>(sh::Statuses::S400_Bad_Request(err.what()), shd::CreateQueryResponse{});
+                }
+                
                 result.query.setLimit(req.recordLimit);
                 result.query.setIncludeVersionMismatch(req.includeVersionMismatch);
                 result.query.setReverseDirection(req.reverseDirection);
@@ -465,6 +525,7 @@ HTTPProxy::_handleCreateQuery(shd::CreateQueryRequest&& request) {
                 return MakeHTTPResponse<shd::CreateQueryResponse>(Txn_S410_Gone, shd::CreateQueryResponse{});
             } else {
                 updateExpiry(it->second);
+                
                 it->second.queries[queryId] = std::move(result.query);
                 return MakeHTTPResponse<shd::CreateQueryResponse>(
                     sh::Status{.code = result.status.code, .message = result.status.message},
