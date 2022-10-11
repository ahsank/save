diff --git a/src/gausskernel/storage/access/k2/catalog/sql_catalog_manager.cpp b/src/gausskernel/storage/access/k2/catalog/sql_catalog_manager.cpp
index 8150364b..f774adaa 100644
--- a/src/gausskernel/storage/access/k2/catalog/sql_catalog_manager.cpp
+++ b/src/gausskernel/storage/access/k2/catalog/sql_catalog_manager.cpp
@@ -35,7 +35,7 @@ SqlCatalogManager::~SqlCatalogManager() {
 sh::Status SqlCatalogManager::Start() {
     K2LOG_I(log::catalog, "Starting Catalog Manager...");
     K2ASSERT(log::catalog, !initted_.load(std::memory_order_acquire), "Already started");
-
+    log::catalog.moduleLevel = k2::logging::LogLevel::Debug;
     // load cluster info
     auto [status, clusterInfo] = cluster_info_handler_.GetClusterInfo(cluster_id_);
     if (!status.is2xxOK()) {
