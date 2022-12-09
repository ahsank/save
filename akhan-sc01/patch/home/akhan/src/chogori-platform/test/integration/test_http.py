diff --git a/test/integration/test_http.py b/test/integration/test_http.py
index ac7eb14..abfc69e 100755
--- a/test/integration/test_http.py
+++ b/test/integration/test_http.py
@@ -409,7 +409,7 @@ class TestHTTP(unittest.TestCase):
         status, query = txn.create_query(test_coll, test_schema.name, start = key)
         self.assertTrue(status.is2xxOK(), msg=status.message)
         status, records = txn.queryAll(query)
-        self.assertTrue(status.is2xxOK())
+        self.assertTrue(status.is2xxOK(), msg=status.message)
         self.assertEqual([r.data for r in records], [record2.data])
 
         # End to partition1=h, should return record1
