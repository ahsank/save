file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(client STATIC ${HEADERS} ${SOURCES})

set_target_properties(client PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_link_libraries (client PRIVATE common dto mpack httplib)

add_executable (skvtester skvtest.cpp)
target_link_libraries (skvtester PRIVATE skv_common skv_dto mpack httplib)

# export the library in the common skvTargets
install(TARGETS client EXPORT skvTargets DESTINATION lib/skv)
install(FILES ${HEADERS} DESTINATION include/skv/client)
# export the cmake config in the build tree for any users who want to use this project from source
export(TARGETS client NAMESPACE skv:: FILE client-config.cmake)

install (TARGETS skvtester DESTINATION bin)
