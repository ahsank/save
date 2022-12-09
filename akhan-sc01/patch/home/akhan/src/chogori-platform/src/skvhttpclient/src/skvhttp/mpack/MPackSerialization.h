diff --git a/src/skvhttpclient/src/skvhttp/mpack/MPackSerialization.h b/src/skvhttpclient/src/skvhttp/mpack/MPackSerialization.h
index af73c68..4e937e4 100644
--- a/src/skvhttpclient/src/skvhttp/mpack/MPackSerialization.h
+++ b/src/skvhttpclient/src/skvhttp/mpack/MPackSerialization.h
@@ -317,8 +317,10 @@ private:
         if (sizeof(Decimal128::__decfloat128) != sz) {
             return false;
         }
-
-        value.__setval(*((Decimal128::__decfloat128*)data));
+        Decimal128::__decfloat128 tmp = 0;
+        // Use memcpy to avoid any alignment issue
+        memcpy((void*)&tmp, data, sz);
+        value.__setval(tmp);
         return true;
     }
 
@@ -337,7 +339,7 @@ private:
     }
 
 private:
-    mpack_node_t _node;
+    mpack_node_t _node{};
     Binary& _source;
 };
 
@@ -347,6 +349,26 @@ public:
     MPackReader(const Binary& bin): _binary(bin){
         mpack_tree_init_data(&_tree, _binary.data(), _binary.size());  // initialize a parser + parse a tree
     }
+    ~MPackReader(){
+        mpack_tree_destroy(&_tree);
+    }
+
+    MPackReader& operator=(const MPackReader& o) =delete;
+
+    MPackReader(const MPackReader& o) = delete;
+
+    MPackReader(MPackReader&& o) {
+        _tree = std::move(o._tree);
+        _binary = std::move(o._binary);
+    }
+
+    MPackReader& operator=(MPackReader&& o) {
+        mpack_tree_destroy(&_tree);
+        _tree = std::move(o._tree);
+        _binary = std::move(o._binary);
+        return *this;
+    }
+
     template<typename T>
     bool read(T& obj) {
         // read an entire node tree as a single object.
@@ -357,7 +379,7 @@ public:
             return false;
         }
         MPackNodeReader reader(node, _binary);
-        return reader.read(obj);
+        return  reader.read(obj);
     }
 
 private:
@@ -532,6 +554,34 @@ public:
     MPackWriter() {
         mpack_writer_init_growable(&_writer, &_data, &_size);
     }
+
+    ~MPackWriter() {
+        mpack_writer_destroy(&_writer);
+        MPACK_FREE(_data);
+    }
+
+
+    MPackWriter& operator=(const MPackWriter& o) =delete;
+
+    MPackWriter(const MPackWriter& o) = delete;
+
+    MPackWriter(MPackWriter&& o) {
+        _data = o._data;
+        _size = o._size;
+        o._data = NULL;
+        o._size = 0;
+        _writer = std::move(o._writer);
+    }
+
+    MPackWriter& operator=(MPackWriter&& o) {
+        _data = o._data;
+        _size = o._size;
+        o._data = NULL;
+        o._size = 0;
+        _writer = std::move(o._writer);
+        return *this;
+    }
+
     template <typename T>
     void write(T&& obj) {
         MPackNodeWriter writer(_writer);
@@ -551,8 +601,8 @@ public:
         return true;
     }
 private:
-    char* _data;
-    size_t _size;
-    mpack_writer_t _writer;
+    char* _data = NULL;
+    size_t _size = 0;
+    mpack_writer_t _writer {};
 };
 }
