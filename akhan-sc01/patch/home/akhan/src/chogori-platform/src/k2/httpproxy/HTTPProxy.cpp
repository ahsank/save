diff --git a/src/k2/httpproxy/HTTPProxy.cpp b/src/k2/httpproxy/HTTPProxy.cpp
index 061158a..cdd6df4 100644
--- a/src/k2/httpproxy/HTTPProxy.cpp
+++ b/src/k2/httpproxy/HTTPProxy.cpp
@@ -21,6 +21,7 @@ Copyright(c) 2022 Futurewei Cloud
     SOFTWARE.
 */
 
+#include <iomanip>
 #include "HTTPProxy.h"
 
 #include <seastar/core/sleep.hh>
@@ -37,6 +38,69 @@ namespace k2 {
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
+struct HexStr {
+    const std::string& str;
+    HexStr(const std::string& val) : str(val) {}
+};
+
+std::ostream& operator<<(std::ostream& os, const HexStr& hs) {
+    for (size_t i=0; i < hs.str.length(); i++) {
+        if (isprint(hs.str[i])) {
+            os << hs.str[i];
+        } else {
+            os << "\\x" <<  std::setw(2) << std::setfill('0') << std::hex << (hs.str[i] & 0xff);
+        }
+    }
+    return os;
+}
+
+std::string _serialize(shd::SKVRecord& shdrec) {
+    std::ostringstream os;
+    uint32_t serializedCursor = shdrec.storage.serializedCursor;
+    for (uint32_t field = 0; field < serializedCursor; ++field) {
+        shdrec.visitNextField([&os](const auto& fld, auto&& value) {
+            if (value) {
+                os << fld.name << "=";
+                using T = typename std::remove_reference_t<decltype(value)>::value_type;
+                if constexpr (std::is_same_v<T, shd::FieldType>) {
+                    auto kval = static_cast<dto::FieldType>(to_integral(*value));
+                    os << kval;
+                } else if constexpr (std::is_same_v<T, sh::String>) {
+                    os << HexStr(*value);
+                } else {
+                    os << *value;
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
@@ -189,7 +253,10 @@ HTTPProxy::_handleTxnBegin(shd::TxnBeginRequest&& request){
 seastar::future<std::tuple<sh::Status, shd::WriteResponse>>
 HTTPProxy::_handleWrite(K2TxnHandle& txn, shd::WriteRequest&& request, dto::SKVRecord&& k2record) {
     return txn.write(k2record, request.isDelete, static_cast<dto::ExistencePrecondition>(request.precondition))
-        .then([](WriteResult&& result) {
+        .then([ts=txn.mtr().timestamp](WriteResult&& result) {
+            if (!result.status.is2xxOK()) {
+                K2LOG_D(log::httpproxy, "Write error ts={}, status={}", ts, result.status);
+            }
             return MakeHTTPResponse<shd::WriteResponse>(sh::Status{.code = result.status.code, .message = result.status.message}, shd::WriteResponse{});
         });
 }
@@ -226,9 +293,15 @@ HTTPProxy::_handleWrite(shd::WriteRequest&& request) {
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
@@ -257,14 +330,21 @@ HTTPProxy::_handleRead(shd::ReadRequest&& request) {
                 dto::SKVRecord k2record(request.collectionName, k2Schema);
                 shd::SKVRecord shdrecord(request.collectionName, shdSchema, std::move(request.key), true);
                 try {
+                    K2LOG_D(log::httpproxy, "Read detail ts={} read=[{}]", it->second.handle.mtr().timestamp, _serialize(shdrecord));
+
                     _shdRecToK2(shdrecord, k2record);
                 }  catch(shd::DeserializationError& err) {
                     return MakeHTTPResponse<shd::ReadResponse>(sh::Statuses::S400_Bad_Request(err.what()), shd::ReadResponse{});
-                }
+                }  catch(std::exception& err) {
+                K2LOG_E(log::httpproxy, "Exception {}", err);
+                return MakeHTTPResponse<shd::ReadResponse>(sh::Statuses::S400_Bad_Request(err.what()), shd::ReadResponse{});
+                
+            }
 
                 return it->second.handle.read(std::move(k2record))
-                    .then([&request, shdSchema, k2Schema](auto&& result) {
+                    .then([&request, shdSchema, k2Schema, ts=it->second.handle.mtr().timestamp](auto&& result) {
                         if (!result.status.is2xxOK()) {
+                            K2LOG_D(log::httpproxy, "Read error ts={}, status={}", ts, result.status);
                             return MakeHTTPResponse<shd::ReadResponse>(sh::Status{.code = result.status.code, .message = result.status.message}, shd::ReadResponse{});
                         }
                         auto rec = _buildSHDRecord(result.value, request.collectionName, shdSchema);
@@ -281,7 +361,7 @@ HTTPProxy::_handleRead(shd::ReadRequest&& request) {
 
 seastar::future<std::tuple<sh::Status, shd::QueryResponse>>
 HTTPProxy::_handleQuery(shd::QueryRequest&& request) {
-    K2LOG_D(log::httpproxy, "Received query request {}", request);
+    K2LOG_D(log::httpproxy, "Received query request txn={} queryId={}", request.timestamp, request.queryId);
     auto iter = _txns.find(request.timestamp);
     if (iter == _txns.end()) {
         return MakeHTTPResponse<shd::QueryResponse>(Txn_S410_Gone, shd::QueryResponse{});
@@ -303,8 +383,7 @@ HTTPProxy::_handleQuery(shd::QueryRequest&& request) {
     payload.seek(0);
     payload.read(paginationKey);
 
-    queryIter->second.resetPaginationToken(std::move(paginationKey), request.paginationExclusiveKey);
-
+    queryIter->second.resetPaginationToken(std::move(paginationKey), request.paginationExclusiveKey);    
     return iter->second.handle.query(queryIter->second)
     .then([this, request=std::move(request)](QueryResult&& result) {
         if(!result.status.is2xxOK()) {
@@ -391,10 +470,11 @@ HTTPProxy::_handleGetSchema(shd::GetSchemaRequest&& request) {
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
@@ -431,20 +511,28 @@ k2exp::Expression getFilterExpression(shdexp::Expression&& shExpr) {
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
@@ -465,6 +553,7 @@ HTTPProxy::_handleCreateQuery(shd::CreateQueryRequest&& request) {
                 return MakeHTTPResponse<shd::CreateQueryResponse>(Txn_S410_Gone, shd::CreateQueryResponse{});
             } else {
                 updateExpiry(it->second);
+                
                 it->second.queries[queryId] = std::move(result.query);
                 return MakeHTTPResponse<shd::CreateQueryResponse>(
                     sh::Status{.code = result.status.code, .message = result.status.message},
