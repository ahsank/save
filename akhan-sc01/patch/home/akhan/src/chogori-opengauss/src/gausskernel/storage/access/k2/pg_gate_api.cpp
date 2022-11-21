diff --git a/src/gausskernel/storage/access/k2/pg_gate_api.cpp b/src/gausskernel/storage/access/k2/pg_gate_api.cpp
index 66ed6b55..463fa211 100644
--- a/src/gausskernel/storage/access/k2/pg_gate_api.cpp
+++ b/src/gausskernel/storage/access/k2/pg_gate_api.cpp
@@ -115,13 +115,13 @@ K2PgStatus PgGate_InitSession(const char *database_name) {
 //   Postgres operations are done, associated K2PG memory context (K2PgMemCtx) will be
 //   destroyed together with Postgres memory context.
 K2PgMemctx PgGate_CreateMemctx() {
-    elog(LOG, "PgGateAPI: PgGate_CreateMemctx");
+    elog(DEBUG5, "PgGateAPI: PgGate_CreateMemctx");
     // Postgres will create PG Memctx when it first use the Memctx to allocate K2PG object.
     return k2pg::PgMemctx::Create();
 }
 
 K2PgStatus PgGate_DestroyMemctx(K2PgMemctx memctx) {
-    elog(LOG, "PgGateAPI: PgGate_DestroyMemctx");
+    elog(DEBUG5, "PgGateAPI: PgGate_DestroyMemctx");
     // Postgres will destroy PG Memctx by releasing the pointer.
     return k2pg::PgMemctx::Destroy(memctx);
 }
@@ -383,7 +383,7 @@ K2PgStatus PgGate_ExecDropTable(K2PgOid database_oid,
 K2PgStatus PgGate_GetTableDesc(K2PgOid database_oid,
                             K2PgOid table_oid,
                             K2PgTableDesc *handle) {
-    elog(LOG, "PgGateAPI: PgGate_GetTableDesc %d, %d", database_oid, table_oid);
+    elog(DEBUG5, "PgGateAPI: PgGate_GetTableDesc %d, %d", database_oid, table_oid);
     *handle = k2pg::pg_session->LoadTable(database_oid, table_oid).get();
     if (*handle == nullptr) {
         return K2PgStatus {
@@ -455,7 +455,7 @@ K2PgStatus PgGate_ExecCreateIndex(const char *database_name,
         .attr_name = "k2pgidxbasectid",
         .attr_num = k2pg::to_underlying(k2pg::PgSystemAttrNum::kPgIdxBaseTupleId),
         .type_oid = BYTEAOID,
-        .is_key = false,
+        .is_key = !is_unique_index,
         .is_desc = false,
         .is_nulls_first = columns[0].is_nulls_first // honor the settings in other keys
     };
@@ -568,7 +568,7 @@ K2PgStatus PgGate_DmlFetch(K2PgScanHandle* handle, int32_t nattrs, uint64_t *val
 // This function returns the tuple id (k2pgctid) of a Postgres tuple.
 K2PgStatus PgGate_DmlBuildPgTupleId(Oid db_oid, Oid table_oid, const std::vector<K2PgAttributeDef>& attrs,
                                     uint64_t *k2pgctid){
-    elog(LOG, "PgGateAPI: PgGate_DmlBuildPgTupleId db: %d, table %d, attr size: %lu", db_oid, table_oid, attrs.size());
+    elog(DEBUG5, "PgGateAPI: PgGate_DmlBuildPgTupleId db: %d, table %d, attr size: %lu", db_oid, table_oid, attrs.size());
 
     skv::http::dto::SKVRecord fullRecord;
     std::shared_ptr<k2pg::PgTableDesc> pg_table = k2pg::pg_session->LoadTable(db_oid, table_oid);
@@ -612,7 +612,7 @@ K2PgStatus PgGate_ExecInsert(K2PgOid database_oid,
                              bool increment_catalog,
                              std::vector<K2PgAttributeDef>& columns,
                              Datum* k2pgtupleid) {
-    elog(LOG, "PgGateAPI: PgGate_ExecInsert %d, %d", database_oid, table_oid);
+    elog(DEBUG5, "PgGateAPI: PgGate_ExecInsert %d, %d", database_oid, table_oid);
     auto catalog = pg_gate->GetCatalogClient();
 
     skv::http::dto::SKVRecord record;
@@ -681,7 +681,7 @@ K2PgStatus PgGate_ExecUpdate(K2PgOid database_oid,
                              bool increment_catalog,
                              int* rows_affected,
                              const std::vector<K2PgAttributeDef>& columns) {
-    elog(LOG, "PgGateAPI: PgGate_ExecUpdate %u, %u", database_oid, table_oid);
+    elog(DEBUG5, "PgGateAPI: PgGate_ExecUpdate %u, %u", database_oid, table_oid);
 
     auto catalog = pg_gate->GetCatalogClient();
     if (rows_affected) {
@@ -821,7 +821,7 @@ K2PgStatus PgGate_NewSelect(K2PgOid database_oid,
                          K2PgOid table_oid,
                          K2PgSelectIndexParams idxp,
                          K2PgScanHandle **handle){
-    elog(LOG, "PgGateAPI: PgGate_NewSelect %d, %d", database_oid, table_oid);
+    elog(DEBUG5, "PgGateAPI: PgGate_NewSelect %d, %d", database_oid, table_oid);
     *handle = new K2PgScanHandle();
     GetCurrentK2Memctx()->Cache([ptr=*handle] () { delete ptr;});
     (*handle)->indexParams = std::move(idxp);
