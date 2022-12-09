diff --git a/src/skvhttpclient/test/ClientIntegrationTest.cpp b/src/skvhttpclient/test/ClientIntegrationTest.cpp
index 265aaf0..2b9c1dc 100644
--- a/src/skvhttpclient/test/ClientIntegrationTest.cpp
+++ b/src/skvhttpclient/test/ClientIntegrationTest.cpp
@@ -59,10 +59,10 @@ void verifyEqual(dto::SKVRecord& first, dto::SKVRecord& second) {
 }
 
 // Compare a query result with reference records
-void verifyEqual(std::vector<dto::SKVRecord>& first, std::vector<dto::SKVRecord>&& second) {
+void verifyEqual(std::vector<dto::SKVRecord>& first, std::vector<dto::SKVRecord*>&& second) {
     K2EXPECT(k2::log::httpclient, first.size(), second.size());
     for (size_t i=0; i < first.size(); i++) {
-        verifyEqual(first[i], second[i]);
+        verifyEqual(first[i], *second[i]);
     }
 }
 
@@ -290,7 +290,7 @@ void testQuery1() {
         auto&& [createStatus, query] = txn.createQuery(start, end).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record1, record2});
+        verifyEqual(records, {&record1, &record2});
     }
     auto record_mid = buildRecord(collectionName, schemaPtr, std::string("A"), std::string("C"), int32_t(33), std::string("data"));
     {
@@ -298,35 +298,35 @@ void testQuery1() {
         auto&& [createStatus, query] = txn.createQuery(record_mid, end).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record2});
+        verifyEqual(records, {&record2});
     }
     {
         // Query ending range key = C, should only return record 1
         auto&& [createStatus, query] = txn.createQuery(start, record_mid).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record1});
+        verifyEqual(records, {&record1});
     }
     {
         // Limit = 1
         auto&& [createStatus, query] = txn.createQuery(start, end, {}, {}, 1).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record1});
+        verifyEqual(records, {&record1});
     }
     {
         // Reverse = true
         auto&& [createStatus, query] = txn.createQuery(start, end, {}, {}, -1, true).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record2, record1});
+        verifyEqual(records, {&record2, &record1});
     }
     {
         // Limit 1 from reverse
         auto&& [createStatus, query] = txn.createQuery(start, end, {}, {}, 1, true).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record2});
+        verifyEqual(records, {&record2});
     }
     {
         // Search by prefix = A
@@ -334,7 +334,7 @@ void testQuery1() {
         auto&& [createStatus, query] = txn.createQuery(prefix, end).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record1, record2});
+        verifyEqual(records, {&record1, &record2});
     }
     {
         // Query using filter
@@ -342,7 +342,7 @@ void testQuery1() {
         auto&& [createStatus, query] = txn.createQuery(start, end, std::move(filter)).get();
         K2EXPECT(k2::log::httpclient, createStatus.is2xxOK(), true);
         auto records = queryAll(txn, collectionName, schemaPtr, query);
-        verifyEqual(records, {record2});
+        verifyEqual(records, {&record2});
     }
     {
         // Query using projection
@@ -377,9 +377,9 @@ void testPartialUpdate() {
       auto&& [writeStatus] = txn.write(record).get();
       K2EXPECT(k2::log::httpclient, writeStatus.is2xxOK(), true);
     }
-    auto key = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"));
     {
-        auto&& [readStatus, readRecord] = txn.read(key).get();
+        auto key = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"));
+        auto&& [readStatus, readRecord] = txn.read(std::move(key)).get();
         K2EXPECT(k2::log::httpclient, readStatus.is2xxOK(), true);
         verifyEqual(readRecord, record);
     }
@@ -388,7 +388,8 @@ void testPartialUpdate() {
         std::vector<uint32_t> fields = {3}; // Update data field only
         auto&& [writeStatus] = txn.partialUpdate(record1, fields).get();
         K2EXPECT(k2::log::httpclient, writeStatus.is2xxOK(), true);
-        auto&& [readStatus, readRecord] = txn.read(key).get();
+        auto key = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"));        
+        auto&& [readStatus, readRecord] = txn.read(std::move(key)).get();
         K2EXPECT(k2::log::httpclient, readStatus.is2xxOK(), true);
         auto compareRecord = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"), 33, std::string("Test1_Update"));
         verifyEqual(readRecord, compareRecord);
@@ -397,11 +398,10 @@ void testPartialUpdate() {
         K2LOG_I(k2::log::httpclient, "Full write, should succeed");
         // Full write updating balance and data, should succeed
         auto record1 = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"), 35, std::string("Test1_Update1"));
-        auto key = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"));
-
         auto&& [writeStatus] = txn.write(record1).get();
         K2EXPECT(k2::log::httpclient, writeStatus.is2xxOK(), true);
-        auto&& [readStatus, readRecord] = txn.read(key).get();
+        auto key = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"));        
+        auto&& [readStatus, readRecord] = txn.read(std::move(key)).get();
         K2EXPECT(k2::log::httpclient, readStatus.is2xxOK(), true);
         verifyEqual(readRecord, record1);
         {
@@ -410,13 +410,13 @@ void testPartialUpdate() {
             auto&& [writeStatus] = txn.write(record1_1, false, dto::ExistencePrecondition::NotExists).get();
             K2EXPECT(k2::log::httpclient, writeStatus.is2xxOK(),false);
             K2EXPECT(k2::log::httpclient, writeStatus.code, 412);
-            auto&& [readStatus, readRecord1] = txn.read(key).get();
+            auto key = buildRecord(collectionName, schemaPtr, std::string("A1"), std::string("B1"));        
+            auto&& [readStatus, readRecord1] = txn.read(std::move(key)).get();
             K2EXPECT(k2::log::httpclient, readStatus.is2xxOK(), true);
             verifyEqual(readRecord1, record1);
         }
     }
 
-    auto key1 = buildRecord(collectionName, schemaPtr, std::string("A2"), std::string("B2"));
     {
         K2LOG_I(k2::log::httpclient, "Write record with no value");
         // Write a record with no value for Data field, should fail
@@ -429,7 +429,8 @@ void testPartialUpdate() {
         auto record1 = buildRecord(collectionName, schemaPtr, std::string("A2"), std::string("B2"), 32, std::optional<std::string>());
         auto&& [writeStatus] = txn.write(record1).get();
         K2EXPECT(k2::log::httpclient, writeStatus.is2xxOK(), true);
-        auto&& [readStatus, readRecord] = txn.read(key1).get();
+        auto key1 = buildRecord(collectionName, schemaPtr, std::string("A2"), std::string("B2"));
+        auto&& [readStatus, readRecord] = txn.read(std::move(key1)).get();
         K2EXPECT(k2::log::httpclient, readStatus.is2xxOK(), true);
         verifyEqual(readRecord, record1);
     }
@@ -439,7 +440,8 @@ void testPartialUpdate() {
         std::vector<uint32_t> fields = {3}; // Update data field only
         auto&& [writeStatus] = txn.partialUpdate(record1, fields).get();
         K2EXPECT(k2::log::httpclient, writeStatus.is2xxOK(), true);
-        auto&& [readStatus, readRecord] = txn.read(key1).get();
+        auto key1 = buildRecord(collectionName, schemaPtr, std::string("A2"), std::string("B2"));
+        auto&& [readStatus, readRecord] = txn.read(std::move(key1)).get();
         K2EXPECT(k2::log::httpclient, readStatus.is2xxOK(), true);
         auto compareRecord = buildRecord(collectionName, schemaPtr, std::string("A2"), std::string("B2"), 32, std::string("Test2_Update"));
         verifyEqual(readRecord, compareRecord);
