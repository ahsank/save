diff --git a/src/k2/httpproxy/HTTPProxy.h b/src/k2/httpproxy/HTTPProxy.h
index 039c10e..641964d 100644
--- a/src/k2/httpproxy/HTTPProxy.h
+++ b/src/k2/httpproxy/HTTPProxy.h
@@ -93,7 +93,7 @@ private:
 
     std::shared_ptr<shd::Schema> _getSchemaFromCache(const sh::String& cname, std::shared_ptr<dto::Schema> schema);
 
-    void _shdStorageToK2Record(const sh::String& collectionName, shd::SKVRecord::Storage&& key, dto::SKVRecord& k2record);
+    shd::SKVRecord _shdStorageToK2Record(const sh::String& collectionName, shd::SKVRecord::Storage&& key, dto::SKVRecord& k2record);
 
     void _registerAPI();
     void _registerMetrics();
