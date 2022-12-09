diff --git a/src/skvhttpclient/CMakeLists.txt b/src/skvhttpclient/CMakeLists.txt
index 6ee5b78..5b68d77 100644
--- a/src/skvhttpclient/CMakeLists.txt
+++ b/src/skvhttpclient/CMakeLists.txt
@@ -14,7 +14,7 @@ endif()
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_POSITION_INDEPENDENT_CODE ON)
 add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
-add_compile_options(-O3 -g -Werror -Wall -Wextra)
+add_compile_options(-O0 -g -ggdb -Werror -Wall -Wextra)
 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 
 include_directories(src)
