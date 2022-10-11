diff --git a/src/gausskernel/storage/access/k2/session.cpp b/src/gausskernel/storage/access/k2/session.cpp
index da049398..89d7c89b 100644
--- a/src/gausskernel/storage/access/k2/session.cpp
+++ b/src/gausskernel/storage/access/k2/session.cpp
@@ -80,6 +80,7 @@ void TxnManager::_init() {
         // We don't really handle nested transactions separately - all ops are just bundled in the parent
         // if we did, register this callback to handle nested txns:
         // RegisterSubXactCallback(K2SubxactCallback, NULL);
+        log::k2pg.moduleLevel = k2::logging::LogLevel::Debug;
         _initialized = true;
         auto clientConfig = _config.sub("client");
         std::string host = clientConfig.get<std::string>("host", "localhost");
