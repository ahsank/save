/*
MIT License

Copyright(c) 2022 Futurewei Cloud

    Permission is hereby granted,
    free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS",
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER
    LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

// When we mix certain C++ standard lib code and pg code there seems to be a macro conflict that
// will cause compiler errors in libintl.h. Including as the first thing fixes this.
#include <libintl.h>
#include <unordered_map>
#include <assert.h>
#include <atomic>
#include <memory>

#include "access/k2/pg_gate_api.h"
#include "access/k2/pg_memctx.h"
#include "access/k2/pg_ids.h"

#include "k2pg-internal.h"
#include "session.h"
#include "access/k2/pg_session.h"
#include "access/k2/k2_util.h"
#include "storage.h"
#include "access/k2/k2pg_util.h"

#include "utils/elog.h"
#include "utils/errcodes.h"
#include "pg_gate_defaults.h"
#include "pg_gate_thread_local.h"
#include "catalog/sql_catalog_client.h"
#include "catalog/sql_catalog_manager.h"

using namespace k2pg::gate;

namespace {

    class PgGate {
        public:
        // TODO: remove K2PgTypeEntity to map to use type oid to map to k2 type directly
        PgGate(const K2PgTypeEntity *k2PgDataTypeArray, int count, K2PgCallbacks callbacks) {
            // Setup type mapping.
            for (int idx = 0; idx < count; idx++) {
                const K2PgTypeEntity *type_entity = &k2PgDataTypeArray[idx];
                type_map_[type_entity->type_oid] = type_entity;
            }
            catalog_manager_ = std::make_shared<k2pg::catalog::SqlCatalogManager>();
            catalog_manager_->Start();
        };

        ~PgGate() {
              catalog_manager_->Shutdown();
        };

        std::shared_ptr<k2pg::catalog::SqlCatalogClient> GetCatalogClient() {
              return k2pg::pg_session->GetCatalogClient();
        };

        std::shared_ptr<k2pg::catalog::SqlCatalogManager> GetCatalogManager() {
              return catalog_manager_;
        };

        const K2PgTypeEntity *FindTypeEntity(int type_oid) {
              const auto iter = type_map_.find(type_oid);
              if (iter != type_map_.end()) {
                  return iter->second;
              }
              return nullptr;
        };

        private:
        K2PgCallbacks pg_callbacks_;

        // Mapping table of K2PG and PostgreSQL datatypes.
        std::unordered_map<int, const K2PgTypeEntity *> type_map_;

        std::shared_ptr<k2pg::catalog::SqlCatalogManager> catalog_manager_;

    };

    // use anonymous namespace to define a static variable that is not exposed outside of this file
    std::shared_ptr<PgGate> pg_gate;
} // anonymous namespace

void PgGate_InitPgGate(const K2PgTypeEntity *k2PgDataTypeTable, int count, PgCallbacks pg_callbacks) {
    assert(pg_gate == nullptr && "PgGate should only be initialized once");
    elog(INFO, "K2 PgGate open");
    pg_gate = std::make_shared<PgGate>(k2PgDataTypeTable, count, pg_callbacks);
}

void PgGate_DestroyPgGate() {
    if (pg_gate == nullptr) {
      elog(ERROR, "PgGate is destroyed or not initialized");
    } else {
      pg_gate = nullptr;
      elog(INFO, "K2 PgGate destroyed");
    }
}

// Initialize a session to process statements that come from the same client connection.
K2PgStatus PgGate_InitSession(const char *database_name) {
    elog(LOG, "PgGateAPI: PgGate_InitSession %s", database_name);

    k2pg::TXMgr.endTxn(skv::http::dto::EndAction::Abort, true).get();

    std::shared_ptr<k2pg::catalog::SqlCatalogClient> catalog_client = std::make_shared<k2pg::catalog::SqlCatalogClient>(pg_gate->GetCatalogManager());
    k2pg::pg_session = std::make_shared<k2pg::PgSession>(catalog_client, database_name);
    return K2PgStatus::OK;
}

// Initialize K2PgMemCtx.
// - Postgres uses memory context to hold all of its allocated space. Once all associated operations
//   are done, the context is destroyed.
// - There K2PG objects are bound to Postgres operations. All of these objects' allocated
//   memory will be held by K2PgMemCtx, whose handle belongs to Postgres MemoryContext. Once all
//   Postgres operations are done, associated K2PG memory context (K2PgMemCtx) will be
//   destroyed toghether with Postgres memory context.
K2PgMemctx PgGate_CreateMemctx() {
    elog(DEBUG5, "PgGateAPI: PgGate_CreateMemctx");
    // Postgres will create PG Memctx when it first use the Memctx to allocate K2PG object.
    return k2pg::PgMemctx::Create();
}

K2PgStatus PgGate_DestroyMemctx(K2PgMemctx memctx) {
    elog(DEBUG5, "PgGateAPI: PgGate_DestroyMemctx");
    // Postgres will destroy PG Memctx by releasing the pointer.
    return k2pg::PgMemctx::Destroy(memctx);
}

K2PgStatus PgGate_ResetMemctx(K2PgMemctx memctx) {
    elog(DEBUG5, "PgGateAPI: PgGate_ResetMemctx");
    // Postgres reset PG Memctx when clearing a context content without clearing its nested context.
    return k2pg::PgMemctx::Reset(memctx);
}

// Invalidate the sessions table cache.
K2PgStatus PgGate_InvalidateCache() {
    elog(DEBUG5, "PgGateAPI: PgGate_InvalidateCache");
    k2pg::pg_session->InvalidateCache();
    return K2PgStatus::OK;
}

// Check if initdb has been already run.
K2PgStatus PgGate_IsInitDbDone(bool* initdb_done) {
    elog(DEBUG5, "PgGateAPI: PgGate_IsInitDbDone");
    return pg_gate->GetCatalogClient()->IsInitDbDone(initdb_done);
}

// Sets catalog_version to the local tserver's catalog version stored in shared
// memory, or an error if the shared memory has not been initialized (e.g. in initdb).
K2PgStatus PgGate_GetSharedCatalogVersion(uint64_t* catalog_version) {
    elog(DEBUG5, "PgGateAPI: PgGate_GetSharedCatalogVersion");
    return pg_gate->GetCatalogClient()->GetCatalogVersion(catalog_version);
}

//--------------------------------------------------------------------------------------------------
// DDL Statements
//--------------------------------------------------------------------------------------------------

// K2 InitPrimaryCluster
K2PgStatus PgGate_InitPrimaryCluster() {
    elog(DEBUG5, "PgGateAPI: PgGate_InitPrimaryCluster");
    return pg_gate->GetCatalogClient()->InitPrimaryCluster();
}

K2PgStatus PgGate_FinishInitDB() {
    elog(DEBUG5, "PgGateAPI: PgGate_FinishInitDB()");
    return pg_gate->GetCatalogClient()->FinishInitDB();
}

// DATABASE ----------------------------------------------------------------------------------------
// Connect database. Switch the connected database to the given "database_name".
K2PgStatus PgGate_ConnectDatabase(const char *database_name) {
    elog(DEBUG5, "PgGateAPI: PgGate_ConnectDatabase %s", database_name);
    return k2pg::pg_session->ConnectDatabase(database_name);
}

// Create database.
K2PgStatus PgGate_ExecCreateDatabase(const char *database_name,
                                 K2PgOid database_oid,
                                 K2PgOid source_database_oid,
                                 K2PgOid next_oid) {
  elog(LOG, "PgGateAPI: PgGate_ExecCreateDatabase %s, %d, %d, %d",
         database_name, database_oid, source_database_oid, next_oid);
  return pg_gate->GetCatalogClient()->CreateDatabase(database_name,
      k2pg::PgObjectId::GetDatabaseUuid(database_oid),
      database_oid,
      source_database_oid != k2pg::kPgInvalidOid ? k2pg::PgObjectId::GetDatabaseUuid(source_database_oid) : "",
      "" /* creator_role_name */, next_oid);
}

// Drop database.
K2PgStatus PgGate_ExecDropDatabase(const char *database_name,
                                   K2PgOid database_oid) {
    elog(DEBUG5, "PgGateAPI: PgGate_ExecDropDatabase %s, %d", database_name, database_oid);
    return pg_gate->GetCatalogClient()->DeleteDatabase(database_name,
        k2pg::PgObjectId::GetDatabaseUuid(database_oid));
}

// Alter database.
K2PgStatus PgGate_NewAlterDatabase(const char *database_name,
                               K2PgOid database_oid,
                               K2PgStatement *handle) {
  elog(DEBUG5, "PgGateAPI: PgGate_NewAlterDatabase %s, %d", database_name, database_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AlterDatabaseRenameDatabase(K2PgStatement handle, const char *new_name) {
  elog(DEBUG5, "PgGateAPI: PgGate_AlterDatabaseRenameDatabase %s", new_name);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_ExecAlterDatabase(K2PgStatement handle) {
  elog(DEBUG5, "PgGateAPI: PgGate_ExecAlterDatabase");
  return K2PgStatus::NotSupported;
}

// Reserve oids.
K2PgStatus PgGate_ReserveOids(K2PgOid database_oid,
                           K2PgOid next_oid,
                           uint32_t count,
                           K2PgOid *begin_oid,
                           K2PgOid *end_oid) {
    elog(DEBUG5, "PgGateAPI: PgGate_ReserveOids %d, %d, %d", database_oid, next_oid, count);
    return pg_gate->GetCatalogClient()->ReservePgOids(database_oid, next_oid, count, begin_oid, end_oid);
}

K2PgStatus PgGate_GetCatalogMasterVersion(uint64_t *version) {
    elog(DEBUG5, "PgGateAPI: PgGate_GetCatalogMasterVersion");
    return pg_gate->GetCatalogClient()->GetCatalogVersion(version);
}

void PgGate_InvalidateTableCache(
    const K2PgOid database_oid,
    const K2PgOid table_oid) {
    elog(DEBUG5, "PgGateAPI: PgGate_InvalidateTableCache %d, %d", database_oid, table_oid);
    const k2pg::PgObjectId table_object_id(database_oid, table_oid);
    k2pg::pg_session->InvalidateTableCache(table_object_id);
}

K2PgStatus PgGate_InvalidateTableCacheByTableId(const char *table_uuid) {
    elog(DEBUG5, "PgGateAPI: PgGate_InvalidateTableCacheByTableId %s", table_uuid);
    if (table_uuid == NULL) {
        K2PgStatus status {
            .pg_code = ERRCODE_FDW_ERROR,
            .k2_code = 400,
            .msg = "Invalid argument",
            .detail = "table_uuid is null"
        };

        return status;
    }
    std::string table_uuid_str = table_uuid;
    const k2pg::PgObjectId table_object_id(table_uuid_str);
    k2pg::pg_session->InvalidateTableCache(table_object_id);
    return K2PgStatus::OK;
}

// Make ColumnSchema from column information
k2pg::ColumnSchema makeColumn(const std::string& col_name, int order, K2SqlDataType k2pg_type, bool is_key, bool is_desc, bool is_nulls_first) {
    using SortingType = k2pg::ColumnSchema::SortingType;
    SortingType sorting_type = SortingType::kNotSpecified;
    if (is_key) {
        if (is_desc) {
            sorting_type = is_nulls_first ? SortingType::kDescending : SortingType::kDescendingNullsLast;
        } else {
            sorting_type = is_nulls_first ? SortingType::kAscending : SortingType::kAscendingNullsLast;
        }
    }
    std::shared_ptr<k2pg::SQLType> data_type = k2pg::SQLType::Create(static_cast<DataType>( k2pg_type));
    bool is_nullable = !is_key;
    return k2pg::ColumnSchema(col_name, data_type, is_nullable, is_key, order, order, sorting_type);
}

std::tuple<k2pg::Status, bool, k2pg::Schema> makeSChema(const std::string& schema_name, const std::vector<K2PGColumnDef>& columns, bool add_primary_key = false) {
    std::vector<k2pg::ColumnSchema> k2pgcols;
    std::vector<k2pg::ColumnId> colIds;
    int num_key_columns = 0;
    const bool is_pg_catalog_table = (schema_name == "pg_catalog") || (schema_name == "information_schema");

    // Add internal primary key column to a Postgres table without a user-specified primary key.
    if (add_primary_key) {
        // For regular user table, k2pgrowid should be a hash key because k2pgrowid is a random uuid.
        k2pgcols.push_back(makeColumn("k2pgrowid",
                static_cast<int32_t>(k2pg::PgSystemAttrNum::kPgRowId),
                static_cast<DataType>( K2SQL_DATA_TYPE_BINARY),
                true /* is_key */, false /* is_desc */, false /* is_nulls_first */));
    }
    // Add key columns at the beginning
    for (auto& col : columns) {
        if (!col.is_key) {
            continue;
        }
        num_key_columns++;
        k2pgcols.push_back(makeColumn(col.attr_name, col.attr_num, col.attr_type->k2pg_type, col.is_key, col.is_desc, col.is_nulls_first));
    }
    // Add data columns
    for (auto& col : columns) {
        if (col.is_key) {
            continue;
        }

        k2pgcols.push_back(makeColumn(col.attr_name, col.attr_num, col.attr_type->k2pg_type, col.is_key, col.is_desc, col.is_nulls_first));
    }
    // Get column ids
    for (size_t i=0; i < k2pgcols.size(); i++) {
        colIds.push_back(k2pg::ColumnId(i));
    }
    k2pg::Schema schema;
    auto status = schema.Reset(k2pgcols, colIds, num_key_columns);
    return std::make_tuple(std::move(status), is_pg_catalog_table, std::move(schema));
}

// TABLE -------------------------------------------------------------------------------------------

// Create and drop table "database_name.schema_name.table_name()".
// - When "schema_name" is NULL, the table "database_name.table_name" is created.
// - When "database_name" is NULL, the table "connected_database_name.table_name" is created.
K2PgStatus PgGate_ExecCreateTable(const char *database_name,
                              const char *schema_name,
                              const char *table_name,
                              K2PgOid database_oid,
                              K2PgOid table_oid,
                              bool if_not_exist,
                              bool add_primary_key,
                              const std::vector<K2PGColumnDef>& columns) {
    elog(DEBUG5, "PgGateAPI: PgGate_NewCreateTable %s, %s, %s", database_name, schema_name, table_name);
    auto [status, is_pg_catalog_table, schema] = makeSChema(schema_name, columns);
    if (!status.IsOK()) {
        return status;
    }
    const k2pg::PgObjectId table_object_id(database_oid, table_oid);
    return pg_gate->GetCatalogClient()->CreateTable(database_name, table_name, table_object_id, schema, is_pg_catalog_table, false /* is_shared_table */, if_not_exist);
}

K2PgStatus PgGate_NewAlterTable(K2PgOid database_oid,
                             K2PgOid table_oid,
                             K2PgStatement *handle){
  elog(DEBUG5, "PgGateAPI: PgGate_NewAlterTable %d, %d", database_oid, table_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AlterTableAddColumn(K2PgStatement handle, const char *name, int order,
                                   const K2PgTypeEntity *attr_type, bool is_not_null){
  elog(DEBUG5, "PgGateAPI: PgGate_AlterTableAddColumn %s", name);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AlterTableRenameColumn(K2PgStatement handle, const char *oldname,
                                      const char *newname){
  elog(DEBUG5, "PgGateAPI: PgGate_AlterTableRenameColumn %s, %s", oldname, newname);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AlterTableDropColumn(K2PgStatement handle, const char *name){
  elog(DEBUG5, "PgGateAPI: PgGate_AlterTableDropColumn %s", name);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AlterTableRenameTable(K2PgStatement handle, const char *db_name,
                                     const char *newname){
  elog(DEBUG5, "PgGateAPI: PgGate_AlterTableRenameTable %s, %s", db_name, newname);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_ExecAlterTable(K2PgStatement handle){
  elog(DEBUG5, "PgGateAPI: PgGate_ExecAlterTable");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_NewDropTable(K2PgOid database_oid,
                            K2PgOid table_oid,
                            bool if_exist,
                            K2PgStatement *handle) {
  elog(DEBUG5, "PgGateAPI: PgGate_NewDropTable %d, %d", database_oid, table_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_ExecDropTable(K2PgStatement handle){
  elog(DEBUG5, "PgGateAPI: PgGate_ExecDropTable");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_GetTableDesc(K2PgOid database_oid,
                            K2PgOid table_oid,
                            K2PgTableDesc *handle) {
  elog(DEBUG5, "PgGateAPI: PgGate_GetTableDesc %d, %d", database_oid, table_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_GetColumnInfo(K2PgTableDesc table_desc,
                             int16_t attr_number,
                             bool *is_primary,
                             bool *is_hash) {
  elog(DEBUG5, "PgGateAPI: PgGate_GetTableDesc %d", attr_number);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_GetTableProperties(K2PgTableDesc table_desc,
                                  K2PgTableProperties *properties){
  elog(DEBUG5, "PgGateAPI: PgGate_GetTableProperties");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_SetIsSysCatalogVersionChange(K2PgStatement handle){
  elog(DEBUG5, "PgGateAPI: PgGate_SetIsSysCatalogVersionChange");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_SetCatalogCacheVersion(K2PgStatement handle, uint64_t catalog_cache_version){
  elog(DEBUG5, "PgGateAPI: PgGate_SetCatalogCacheVersion %ld", catalog_cache_version);
  return K2PgStatus::NotSupported;
}

// INDEX -------------------------------------------------------------------------------------------

// Create and drop index "database_name.schema_name.index_name()".
// - When "schema_name" is NULL, the index "database_name.index_name" is created.
// - When "database_name" is NULL, the index "connected_database_name.index_name" is created.
K2PgStatus PgGate_ExecCreateIndex(const char *database_name,
                              const char *schema_name,
                              const char *index_name,
                              K2PgOid database_oid,
                              K2PgOid index_oid,
                              K2PgOid table_oid,
                              bool is_unique_index,
                              const bool skip_index_backfill,
                              bool if_not_exist,
                              const std::vector<K2PGColumnDef>& columns){
    elog(DEBUG5, "PgGateAPI: PgGate_NewCreateIndex %s, %s, %s", database_name, schema_name, index_name);
    auto [status, is_pg_catalog_table, schema] = makeSChema(schema_name, columns);
    if (!status.IsOK()) {
        return status;
    }
    const k2pg::PgObjectId index_object_id(database_oid, index_oid);
    const k2pg::PgObjectId base_table_object_id(database_oid, table_oid);
    return pg_gate->GetCatalogClient()->CreateIndexTable(database_name, index_name, index_object_id, base_table_object_id, schema, is_unique_index, skip_index_backfill, is_pg_catalog_table, false /* is_shared_table */, if_not_exist);
}

K2PgStatus PgGate_NewDropIndex(K2PgOid database_oid,
                            K2PgOid index_oid,
                            bool if_exist,
                            K2PgStatement *handle){
  elog(DEBUG5, "PgGateAPI: PgGate_NewDropIndex %d, %d", database_oid, index_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_ExecDropIndex(K2PgStatement handle){
  elog(DEBUG5, "PgGateAPI: PgGate_ExecDropIndex");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_WaitUntilIndexPermissionsAtLeast(
    const K2PgOid database_oid,
    const K2PgOid table_oid,
    const K2PgOid index_oid,
    const uint32_t target_index_permissions,
    uint32_t *actual_index_permissions) {
  elog(DEBUG5, "PgGateAPI: PgGate_WaitUntilIndexPermissionsAtLeast %d, %d, %d", database_oid, table_oid, index_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AsyncUpdateIndexPermissions(
    const K2PgOid database_oid,
    const K2PgOid indexed_table_oid){
  elog(DEBUG5, "PgGateAPI: PgGate_AsyncUpdateIndexPermissions %d, %d", database_oid,  indexed_table_oid);
  return K2PgStatus::NotSupported;
}

//--------------------------------------------------------------------------------------------------
// DML statements (select, insert, update, delete, truncate)
//--------------------------------------------------------------------------------------------------

// This function is for specifying the selected or returned expressions.
// - SELECT target_expr1, target_expr2, ...
// - INSERT / UPDATE / DELETE ... RETURNING target_expr1, target_expr2, ...
K2PgStatus PgGate_DmlAppendTarget(K2PgStatement handle, K2PgExpr target){
  elog(DEBUG5, "PgGateAPI: PgGate_DmlAppendTarget");
  return K2PgStatus::NotSupported;
}

// Binding Columns: Bind column with a value (expression) in a statement.
// + This API is used to identify the rows you want to operate on. If binding columns are not
//   there, that means you want to operate on all rows (full scan). You can view this as a
//   a definitions of an initial rowset or an optimization over full-scan.
//
// + There are some restrictions on when BindColumn() can be used.
//   Case 1: INSERT INTO tab(x) VALUES(x_expr)
//   - BindColumn() can be used for BOTH primary-key and regular columns.
//   - This bind-column function is used to bind "x" with "x_expr", and "x_expr" that can contain
//     bind-variables (placeholders) and constants whose values can be updated for each execution
//     of the same allocated statement.
//
//   Case 2: SELECT / UPDATE / DELETE <WHERE key = "key_expr">
//   - BindColumn() can only be used for primary-key columns.
//   - This bind-column function is used to bind the primary column "key" with "key_expr" that can
//     contain bind-variables (placeholders) and constants whose values can be updated for each
//     execution of the same allocated statement.
//
// NOTE ON KEY BINDING
// - For Sequential Scan, the target columns of the bind are those in the main table.
// - For Primary Scan, the target columns of the bind are those in the main table.
// - For Index Scan, the target columns of the bind are those in the index table.
//   The index-scan will use the bind to find base-k2pgctid which is then use to read data from
//   the main-table, and therefore the bind-arguments are not associated with columns in main table.
K2PgStatus PgGate_DmlBindColumn(K2PgStatement handle, int attr_num, K2PgExpr attr_value){
  elog(DEBUG5, "PgGateAPI: PgGate_DmlBindColumn %d", attr_num);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_DmlBindRangeConds(K2PgStatement handle, K2PgExpr range_conds) {
  elog(DEBUG5, "PgGateAPI: PgGate_DmlBindRangeConds");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_DmlBindWhereConds(K2PgStatement handle, K2PgExpr where_conds) {
  elog(DEBUG5, "PgGateAPI: PgGate_DmlBindWhereConds");
  return K2PgStatus::NotSupported;
}

// Binding Tables: Bind the whole table in a statement.  Do not use with BindColumn.
K2PgStatus PgGate_DmlBindTable(K2PgStatement handle){
  elog(DEBUG5, "PgGateAPI: PgGate_DmlBindTable");
  return K2PgStatus::NotSupported;
}

// API for SET clause.
K2PgStatus PgGate_DmlAssignColumn(K2PgStatement handle,
                               int attr_num,
                               K2PgExpr attr_value){
  elog(DEBUG5, "PgGateAPI: PgGate_DmlAssignColumn %d", attr_num);
  return K2PgStatus::NotSupported;
}

// This function is to fetch the targets in PgGate_DmlAppendTarget() from the rows that were defined
// by PgGate_DmlBindColumn().
K2PgStatus PgGate_DmlFetch(K2PgScanHandle* handle, int32_t natts, uint64_t *values, bool *isnulls,
                        K2PgSysColumns *syscols, bool *has_data){
  elog(DEBUG5, "PgGateAPI: PgGate_DmlFetch %d", natts);
  return K2PgStatus::NotSupported;
}

// Utility method that checks stmt type and calls either exec insert, update, or delete internally.
K2PgStatus PgGate_DmlExecWriteOp(K2PgStatement handle, int32_t *rows_affected_count){
  elog(DEBUG5, "PgGateAPI: PgGate_DmlExecWriteOp");
  return K2PgStatus::NotSupported;
}

// This function returns the tuple id (k2pgctid) of a Postgres tuple.
K2PgStatus PgGate_DmlBuildPgTupleId(Oid db_oid, Oid table_id, const std::vector<K2PgAttributeDef>& attrs,
                                    uint64_t *k2pgctid){
    elog(DEBUG5, "PgGateAPI: PgGate_DmlBuildPgTupleId %lu", attrs.size());

    skv::http::dto::SKVRecord record;
    K2PgStatus status = makeSKVRecordFromK2PgAttributes(db_oid, table_id, attrs, record);
    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    }


    // TODO can we remove some of the copies being done?
    skv::http::MPackWriter _writer;
    skv::http::Binary serializedStorage;
    _writer.write(record.getStorage());
    bool flushResult = _writer.flush(serializedStorage);
    if (!flushResult) {
        K2PgStatus err {
            .pg_code = ERRCODE_INTERNAL_ERROR,
            .k2_code = 0,
            .msg = "Serialization error in _writer flush",
            .detail = ""
        };
        return err;
    }

    // This comes from cstring_to_text_with_len which is used to create a proper datum
    // that is prepended with the data length. Doing it by hand here to avoid the extra copy
    char *datum = (char*)palloc(serializedStorage.size() + VARHDRSZ);
    SET_VARSIZE(datum, serializedStorage.size() + VARHDRSZ);
    memcpy(VARDATA(datum), serializedStorage.data(), serializedStorage.size());
    *k2pgctid = PointerGetDatum(datum);

    return K2PgStatus::OK;
}

// INSERT ------------------------------------------------------------------------------------------

K2PgStatus PgGate_ExecInsert(K2PgOid database_oid,
                             K2PgOid table_oid,
                             bool upsert,
                             bool increment_catalog,
                             const std::vector<K2PgAttributeDef>& columns) {
    elog(DEBUG5, "PgGateAPI: PgGate_ExecInsert %d, %d", database_oid, table_oid);
    auto catalog = pg_gate->GetCatalogClient();

    skv::http::dto::SKVRecord record;
    K2PgStatus status = makeSKVRecordFromK2PgAttributes(database_oid, table_oid, columns, record);

    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    }

    auto [k2status] = k2pg::TXMgr.write(record, false, upsert ? skv::http::dto::ExistencePrecondition::None : skv::http::dto::ExistencePrecondition::NotExists).get();
    status = k2pg::K2StatusToK2PgStatus(std::move(k2status));
    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    }

    if (increment_catalog) {
        status = catalog->IncrementCatalogVersion();
    }

    return status;
}

// UPDATE ------------------------------------------------------------------------------------------
K2PgStatus PgGate_ExecUpdate(K2PgOid database_oid,
                             K2PgOid table_oid,
                             bool increment_catalog,
                             int* rows_affected,
                             const std::vector<K2PgAttributeDef>& columns) {
    elog(DEBUG5, "PgGateAPI: PgGate_ExecUpdate %u, %u", database_oid, table_oid);

    auto catalog = pg_gate->GetCatalogClient();
    *rows_affected = 0;
    std::unique_ptr<skv::http::dto::SKVRecordBuilder> builder;

    // Get a builder with the keys serialzed. called function handles tupleId attribute if needed
    K2PgStatus status = makeSKVBuilderWithKeysSerialized(database_oid, table_oid, columns, builder);
    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    }

    // Iterate through the passed attributes to determine which fields should be marked for update
    std::unordered_map<int, K2PgConstant> attr_map;
    std::vector<uint32_t> fieldsForUpdate;
    std::shared_ptr<k2pg::PgTableDesc> pg_table = k2pg::pg_session->LoadTable(database_oid, table_oid);
    for (const auto& column : columns) {
        k2pg::PgColumn *pg_column = pg_table->FindColumn(column.attr_num);
        if (pg_column == NULL) {
            K2PgStatus status {
                .pg_code = ERRCODE_INTERNAL_ERROR,
                .k2_code = 404,
                .msg = "Cannot find column with attr_num",
                .detail = "Load table failed"
            };
            return status;
        }
        // we have two extra fields, i.e., table_id and index_id, in skv key
        fieldsForUpdate.push_back(pg_column->index() + 2);
        attr_map[pg_column->index() + 2] = column.value;
    }

    // Serialize remaining non-key fields
    try {
        size_t offset = builder->getSchema()->partitionKeyFields.size();
        for (size_t i = offset; i < builder->getSchema()->fields.size(); ++i) {
            auto it = attr_map.find(i);
            if (it == attr_map.end()) {
                builder->serializeNull();
            } else {
                serializePGConstToK2SKV(*builder, it->second);
            }
        }
    }
    catch (const std::exception& err) {
        K2PgStatus status {
            .pg_code = ERRCODE_INTERNAL_ERROR,
            .k2_code = 0,
            .msg = "Serialization error in serializePgAttributesToSKV",
            .detail = err.what()
        };

        return status;
    }

    // Send the partialUpdate request to SKV
    skv::http::dto::SKVRecord record = builder->build();
    auto [k2status] = k2pg::TXMgr.partialUpdate(record, std::move(fieldsForUpdate)).get();
    status = k2pg::K2StatusToK2PgStatus(std::move(k2status));
    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    } else {
        *rows_affected = 1;
    }

    if (increment_catalog) {
        K2PgStatus catalog_status = catalog->IncrementCatalogVersion();
        if (!catalog_status.IsOK()) {
            return catalog_status;
        }
    }

    return K2PgStatus::OK;
}

// DELETE ------------------------------------------------------------------------------------------
K2PgStatus PgGate_ExecDelete(K2PgOid database_oid,
                             K2PgOid table_oid,
                             bool increment_catalog,
                             int* rows_affected,
                             const std::vector<K2PgAttributeDef>& columns) {
    elog(DEBUG5, "PgGateAPI: PgGate_ExecDelete %d, %d", database_oid, table_oid);

    auto catalog = pg_gate->GetCatalogClient();
    *rows_affected = 0;
    std::unique_ptr<skv::http::dto::SKVRecordBuilder> builder;

    // Get a builder with the keys serialzed. called function handles tupleId attribute if needed
    K2PgStatus status = makeSKVBuilderWithKeysSerialized(database_oid, table_oid, columns, builder);
    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    }

    // Serialize remaining non-key fields as null to create a valid SKVRecord
    try {
        size_t offset = builder->getSchema()->partitionKeyFields.size();
        for (size_t i = offset; i < builder->getSchema()->fields.size(); ++i) {
            builder->serializeNull();
        }
    }
    catch (const std::exception& err) {
        K2PgStatus status {
            .pg_code = ERRCODE_INTERNAL_ERROR,
            .k2_code = 0,
            .msg = "Serialization error in serializePgAttributesToSKV",
            .detail = err.what()
        };

        return status;
    }

    // Send the delete request to SKV
    skv::http::dto::SKVRecord record = builder->build();
    auto [k2status] = k2pg::TXMgr.write(record, true, skv::http::dto::ExistencePrecondition::Exists).get();
    status = k2pg::K2StatusToK2PgStatus(std::move(k2status));
    if (status.pg_code != ERRCODE_SUCCESSFUL_COMPLETION) {
        return status;
    } else {
        *rows_affected = 1;
    }

    if (increment_catalog) {
        K2PgStatus catalog_status = catalog->IncrementCatalogVersion();
        if (!catalog_status.IsOK()) {
            return catalog_status;
        }
    }

    return K2PgStatus::OK;
}

// SELECT ------------------------------------------------------------------------------------------
K2PgStatus PgGate_NewSelect(K2PgOid database_oid,
                         K2PgOid table_oid,
                         const K2PgSelectIndexParams& index_params,
                         K2PgScanHandle **handle){
  elog(DEBUG5, "PgGateAPI: PgGate_NewSelect %d, %d", database_oid, table_oid);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_ExecSelect(K2PgScanHandle *handle, const std::vector<K2PgConstraintDef>& constraints, const std::vector<int>& targets_attrnum,
                             bool whole_table_scan, bool forward_scan, const K2PgSelectLimitParams& limit_params) {
  elog(DEBUG5, "PgGateAPI: PgGate_ExecSelect");
  return K2PgStatus::NotSupported;
}

// Transaction control -----------------------------------------------------------------------------

K2PgStatus PgGate_BeginTransaction(){
  elog(DEBUG5, "PgGateAPI: PgGate_BeginTransaction");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_RestartTransaction(){
  elog(DEBUG5, "PgGateAPI: PgGate_RestartTransaction");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_CommitTransaction(){
  elog(DEBUG5, "PgGateAPI: PgGate_CommitTransaction");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_AbortTransaction(){
  elog(DEBUG5, "PgGateAPI: PgGate_AbortTransaction");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_SetTransactionIsolationLevel(int isolation){
  elog(DEBUG5, "PgGateAPI: PgGate_SetTransactionIsolationLevel %d", isolation);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_SetTransactionReadOnly(bool read_only){
  elog(DEBUG5, "PgGateAPI: PgGate_SetTransactionReadOnly %d", read_only);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_SetTransactionDeferrable(bool deferrable){
  elog(DEBUG5, "PgGateAPI: PgGate_SetTransactionReadOnly %d", deferrable);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_EnterSeparateDdlTxnMode(){
  elog(DEBUG5, "PgGateAPI: PgGate_EnterSeparateDdlTxnMode");
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_ExitSeparateDdlTxnMode(bool success){
  elog(DEBUG5, "PgGateAPI: PgGate_ExitSeparateDdlTxnMode");
  return K2PgStatus::NotSupported;
}

//--------------------------------------------------------------------------------------------------
// Expressions.

// Column references.
K2PgStatus PgGate_NewColumnRef(K2PgStatement stmt, int attr_num, const K2PgTypeEntity *type_entity,
                            const K2PgTypeAttrs *type_attrs, K2PgExpr *expr_handle){
  elog(DEBUG5, "PgGateAPI: PgGate_NewColumnRef %d", attr_num);
  return K2PgStatus::NotSupported;
}

// Constant expressions.
K2PgStatus PgGate_NewConstant(K2PgStatement stmt, const K2PgTypeEntity *type_entity,
                           uint64_t datum, bool is_null, K2PgExpr *expr_handle){
  elog(DEBUG5, "PgGateAPI: PgGate_NewConstant %ld, %d", datum, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_NewConstantOp(K2PgStatement stmt, const K2PgTypeEntity *type_entity,
                           uint64_t datum, bool is_null, K2PgExpr *expr_handle, bool is_gt){
  elog(DEBUG5, "PgGateAPI: PgGate_NewConstantOp %lu, %d, %d", datum, is_null, is_gt);
  return K2PgStatus::NotSupported;
}

// The following update functions only work for constants.
// Overwriting the constant expression with new value.
K2PgStatus PgGate_UpdateConstInt2(K2PgExpr expr, int16_t value, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstInt2 %d, %d", value, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_UpdateConstInt4(K2PgExpr expr, int32_t value, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstInt4 %d, %d", value, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_UpdateConstInt8(K2PgExpr expr, int64_t value, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstInt8 %ld, %d", value, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_UpdateConstFloat4(K2PgExpr expr, float value, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstFloat4 %f, %d", value, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_UpdateConstFloat8(K2PgExpr expr, double value, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstFloat8 %f, %d", value, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_UpdateConstText(K2PgExpr expr, const char *value, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstText %s, %d", value, is_null);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_UpdateConstChar(K2PgExpr expr, const char *value, int64_t bytes, bool is_null){
  elog(DEBUG5, "PgGateAPI: PgGate_UpdateConstChar %s, %ld, %d", value, bytes, is_null);
  return K2PgStatus::NotSupported;
}

// Expressions with operators "=", "+", "between", "in", ...
K2PgStatus PgGate_NewOperator(K2PgStatement stmt, const char *opname,
                           const K2PgTypeEntity *type_entity,
                           K2PgExpr *op_handle){
  elog(DEBUG5, "PgGateAPI: PgGate_NewOperator %s", opname);
  return K2PgStatus::NotSupported;
}

K2PgStatus PgGate_OperatorAppendArg(K2PgExpr op_handle, K2PgExpr arg){
  elog(DEBUG5, "PgGateAPI: PgGate_OperatorAppendArg");
  return K2PgStatus::NotSupported;
}

// Referential Integrity Check Caching.
// Check if foreign key reference exists in cache.
bool PgGate_ForeignKeyReferenceExists(K2PgOid table_oid, const char* k2pgctid, int64_t k2pgctid_size) {
  elog(DEBUG5, "PgGateAPI: PgGate_ForeignKeyReferenceExists %d, %s, %ld", table_oid, k2pgctid, k2pgctid_size);
  return false;
}

// Add an entry to foreign key reference cache.
K2PgStatus PgGate_CacheForeignKeyReference(K2PgOid table_oid, const char* k2pgctid, int64_t k2pgctid_size){
  elog(DEBUG5, "PgGateAPI: PgGate_CacheForeignKeyReference %d, %s, %ld", table_oid, k2pgctid, k2pgctid_size);
  return K2PgStatus::NotSupported;
}

// Delete an entry from foreign key reference cache.
K2PgStatus PgGate_DeleteFromForeignKeyReferenceCache(K2PgOid table_oid, uint64_t k2pgctid){
  elog(DEBUG5, "PgGateAPI: PgGate_DeleteFromForeignKeyReferenceCache %d, %lu", table_oid, k2pgctid);
  return K2PgStatus::NotSupported;
}

void PgGate_ClearForeignKeyReferenceCache() {
  elog(DEBUG5, "PgGateAPI: PgGate_ClearForeignKeyReferenceCache");
}

bool PgGate_IsInitDbModeEnvVarSet() {
  elog(DEBUG5, "PgGateAPI: PgGate_IsInitDbModeEnvVarSet");
  return false;
}

// This is called by initdb. Used to customize some behavior.
void PgGate_InitFlags() {
  elog(DEBUG5, "PgGateAPI: PgGate_InitFlags");
}

// Retrieves value of psql_max_read_restart_attempts gflag
int32_t PgGate_GetMaxReadRestartAttempts() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetMaxReadRestartAttempts");
  return default_max_read_restart_attempts;
}

// Retrieves value of psql_output_buffer_size gflag
int32_t PgGate_GetOutputBufferSize() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetOutputBufferSize");
  return default_output_buffer_size;
}

// Retrieve value of psql_disable_index_backfill gflag.
bool PgGate_GetDisableIndexBackfill() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetDisableIndexBackfill");
  return default_disable_index_backfill;
}

bool PgGate_IsK2PgEnabled() {
    return pg_gate != nullptr;
}

// Sets the specified timeout in the rpc service.
void PgGate_SetTimeout(int timeout_ms, void* extra) {
  elog(DEBUG5, "PgGateAPI: PgGate_SetTimeout %d", timeout_ms);
  if (timeout_ms <= 0) {
    return;
  }
  timeout_ms = std::min(timeout_ms, default_client_read_write_timeout_ms);
}

//--------------------------------------------------------------------------------------------------
// Thread-Local variables.

void* PgGate_GetThreadLocalCurrentMemoryContext() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetThreadLocalCurrentMemoryContext");
  return PgGetThreadLocalCurrentMemoryContext();
}

void* PgGate_SetThreadLocalCurrentMemoryContext(void *memctx) {
  elog(DEBUG5, "PgGateAPI: PgGate_SetThreadLocalCurrentMemoryContext");
  return PgSetThreadLocalCurrentMemoryContext(memctx);
}

void PgGate_ResetCurrentMemCtxThreadLocalVars() {
  elog(DEBUG5, "PgGateAPI: PgGate_ResetCurrentMemCtxThreadLocalVars");
  PgResetCurrentMemCtxThreadLocalVars();
}

void* PgGate_GetThreadLocalStrTokPtr() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetThreadLocalStrTokPtr");
  return PgGetThreadLocalStrTokPtr();
}

void PgGate_SetThreadLocalStrTokPtr(char *new_pg_strtok_ptr) {
  elog(DEBUG5, "PgGateAPI: PgGate_SetThreadLocalStrTokPtr %s", new_pg_strtok_ptr);
  PgSetThreadLocalStrTokPtr(new_pg_strtok_ptr);
}

void* PgGate_SetThreadLocalJumpBuffer(void* new_buffer) {
  elog(DEBUG5, "PgGateAPI: PgGate_SetThreadLocalJumpBuffer");
  return PgSetThreadLocalJumpBuffer(new_buffer);
}

void* PgGate_GetThreadLocalJumpBuffer() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetThreadLocalJumpBuffer");
  return PgGetThreadLocalJumpBuffer();
}

void PgGate_SetThreadLocalErrMsg(const void* new_msg) {
  elog(DEBUG5, "PgGateAPI: PgGate_SetThreadLocalErrMsg %s", static_cast<const char*>(new_msg));
  PgSetThreadLocalErrMsg(new_msg);
}

const void* PgGate_GetThreadLocalErrMsg() {
  elog(DEBUG5, "PgGateAPI: PgGate_GetThreadLocalErrMsg");
  return PgGetThreadLocalErrMsg();
}

const K2PgTypeEntity *K2PgFindTypeEntity(int type_oid) {
    elog(DEBUG5, "PgGateAPI: K2PgFindTypeEntity %d", type_oid);
    return pg_gate->FindTypeEntity(type_oid);
}

K2PgDataType K2PgGetType(const K2PgTypeEntity *type_entity) {
  elog(DEBUG5, "PgGateAPI: K2PgGetType");
  if (type_entity) {
    return type_entity->k2pg_type;
  }
  return K2SQL_DATA_TYPE_UNKNOWN_DATA;
}

bool K2PgAllowForPrimaryKey(const K2PgTypeEntity *type_entity) {
  elog(DEBUG5, "PgGateAPI: K2PgAllowForPrimaryKey");
  if (type_entity) {
    return type_entity->allow_for_primary_key;
  }
  return false;
}

void K2PgAssignTransactionPriorityLowerBound(double newval, void* extra) {
  elog(DEBUG5, "PgGateAPI: K2PgAssignTransactionPriorityLowerBound %f", newval);
}

void K2PgAssignTransactionPriorityUpperBound(double newval, void* extra) {
  elog(DEBUG5, "PgGateAPI: K2PgAssignTransactionPriorityUpperBound %f", newval);
}

// the following APIs are called by pg_dump.c only
// TODO: check if we really need to implement them

K2PgStatus PgGate_InitPgGateBackend() {
  return K2PgStatus::OK;
}

void PgGate_ShutdownPgGateBackend() {
}
